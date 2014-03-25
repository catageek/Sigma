#include "systems/network/NetworkSystem.h"

#include <thread>
#include <chrono>
#include <cstring>
#include <sys/event.h>
#include "netport/include/net/network.h"
#include "systems/network/Protocol.h"
#include "systems/network/ThreadPool.h"
#include "systems/network/Authentication.h"
#include <typeinfo>

using namespace network;

namespace Sigma {
	NetworkSystem::NetworkSystem() {};

	ThreadPool NetworkSystem::thread_pool(5);
	IOPoller NetworkSystem::poller;

	bool NetworkSystem::Start(const char *ip, unsigned short port) {
		Authentication::GetCryptoEngine()->InitializeDH();
		SetPipeline();
		thread_pool.Initialize();

		if (! poller.Initialize()) {
			LOG_ERROR << "Could not initialize kqueue !";
			return false;
		}
		// start to listen
		auto s = Listener(ip, port);
		if (! s) {
			LOG_ERROR << "Failed to start NetworkSystem.";
			return false;
		}
		// run the IOWorker thread
		//std::thread t(&IOWorker::watch_start, &this->worker, std::move(s));
		//t.detach();
		ssocket = s->Handle();
		poller.Watch(ssocket);
		auto tr = std::make_shared<TaskReq<chain_t>>(start, 0);
		GetThreadPool()->Queue(tr);
		return true;
	}

	std::unique_ptr<TCPConnection> NetworkSystem::Listener(const char *ip, unsigned short port) {
		auto server = std::unique_ptr<TCPConnection>(new TCPConnection());
		if (! server->Init(NETA_IPv4)) {
			LOG_ERROR << "Could not open socket !";
			return std::unique_ptr<TCPConnection>();
		}

		NetworkAddress address;
		address.IP4(ip);
		address.Port(port);
		if (! server->Bind(address)) {
			LOG_ERROR << "Could not bind address !";
			return std::unique_ptr<TCPConnection>();
		}

		if (! server->Listen(5)) {
			LOG_ERROR << "Could not listen on port !";
			return std::unique_ptr<TCPConnection>();
		}
		return std::move(server);
	}

	void NetworkSystem::CloseConnection(int fd) {
		poller.Unwatch(fd);
		TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Close();
		Authentication::GetAuthStateMap()->Erase(fd);
	}

	void NetworkSystem::SetPipeline() {
		start = chain_t({
			// Poll the socket and select the next chain to run
			[&](){
				std::vector<struct kevent> evList(32);
				auto nev = poller.Poll(evList, nullptr);
				if (nev < 1) {
					LOG_ERROR << "Error when polling event";
				}
				LOG_DEBUG << "got " << nev << " kqueue events";
				for (auto i=0; i<nev; i++) {
					auto fd = evList[i].ident;
					if (evList[i].flags & EV_EOF) {
						LOG_DEBUG << "disconnect " << fd;
						CloseConnection(fd);
						continue;
					}
					if (evList[i].flags & EV_ERROR) {   /* report any error */
						LOG_ERROR << "EV_ERROR: " << evList[i].data;
						continue;
					}

					if (fd == ssocket) {
						TCPConnection c;
						if (! TCPConnection(ssocket, NETA_IPv4, SCS_LISTEN).Accept(&c)) {
							LOG_ERROR << "Could not accept connection, handle is " << ssocket;
							continue;
						}
						poller.Watch(c.Handle());
						LOG_DEBUG << "connect " << c.Handle();
//						c.Send("welcome!\n");
						Authentication::GetAuthStateMap()->Insert(c.Handle(), AUTH_INIT);
					}
					else if (evList[i].flags & EVFILT_READ) {
						// Data received
						if (! Authentication::GetAuthStateMap()->Count(fd) || Authentication::GetAuthStateMap()->At(fd) != AUTHENTICATED) {
							// Data received, not authenticated
							// Request the frame header
							// We stop watching the connection until we got all the frame
							poller.Unwatch(fd);
							// queue the task to reassemble the the frame
							NetworkSystem::GetPublicRawFrameReqQueue()->Push(std::make_shared<Frame_req>(fd));
							GetThreadPool()->Queue(std::make_shared<TaskReq<chain_t>>(unauthenticated_recv_data, 0));
						}
						else {
							// Data received from authenticated client
							NetworkSystem::GetDataRecvQueue()->Push(fd);
						}
					}
				}
				// Queue a task to wait another event
				GetThreadPool()->Queue(std::make_shared<TaskReq<chain_t>>(start, 0));
				return STOP;
			}
		});

		unauthenticated_recv_data = chain_t({
			// Get the length
			std::bind(&NetworkSystem::ReassembleFrame, &pub_rawframe_req, &pub_frame_req, &thread_pool),
			[&](){
				auto req_list = NetworkSystem::GetPublicReassembledFrameQueue()->Poll();
				if (! req_list) {
					return STOP;
				}
				LOG_DEBUG << "got " << req_list->size() << " PublicDispatchReq events";
				for (auto& req : *req_list) {
					msg_hdr* header = req->Header();
					auto major = header->type_major;
					if (IS_RESTRICTED(major)) {
						LOG_DEBUG << "restricted";
						CloseConnection(req->fd);
						break;
					}
					switch(major) {
					case NET_MSG:
						{
							auto minor = header->type_minor;
							switch(minor) {
								case AUTH_INIT:
									{
										network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_INIT>()->Push(req);
										auto handler = new chain_t({
											block_t(&network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_INIT>),
											block_t(&network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_SEND_SALT>)
										});
										GetThreadPool()->Queue(std::make_shared<TaskReq<chain_t>>(std::shared_ptr<chain_t>(handler), 0));
										break;
									}
								case AUTH_KEY_EXCHANGE:
									{
										network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_KEY_EXCHANGE>()->Push(req);
										GetThreadPool()->Queue(std::make_shared<TaskReq<block_t>>(block_t(&network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_EXCHANGE>)));
										break;
									}
								default:
									{
										CloseConnection(req->fd);
									}
							}
							break;
						}
					default:
						{
							CloseConnection(req->fd);
						}
					}
				}
				return STOP;
			}
		});
	}

	int NetworkSystem::ReassembleFrame(AtomicQueue<std::shared_ptr<Frame_req>>* input, AtomicQueue<std::shared_ptr<FrameObject>>* output, ThreadPool* thread_pool) {
		auto req_list = input->Poll();
		if (!req_list) {
			return STOP;
		}
		LOG_DEBUG << "got " << req_list->size() << " Reassemble frame requests";
		for (auto& req : *req_list) {
			auto target_size = req->length_requested;
			auto frame = req->reassembled_frame;
			char* buffer = reinterpret_cast<char*>(frame->Length());

			auto current_size = req->length_got;

			auto len = TCPConnection(frame->fd, NETA_IPv4, SCS_CONNECTED).Recv(reinterpret_cast<char*>(buffer) + current_size, target_size - current_size);
			LOG_DEBUG << "got " << len << " bytes in reassemble_frame for fd = " << frame->fd << ", expected = " << target_size << " got = " << current_size;
			current_size += len;
			req->length_got = current_size;

			if(current_size == sizeof(Frame_hdr) && target_size == sizeof(Frame_hdr)) {
				// We now have the length
				auto length = frame->Length()->length;
				target_size = frame->Length()->length + sizeof(Frame_hdr);
				req->length_requested = target_size;
				frame->Resize(length);
				buffer = reinterpret_cast<char*>(frame->Length());
			}

			if (current_size < target_size) {
				// we put again the request in the queue
				input->Push(req);
			}
			else {
				LOG_DEBUG << "Packet of " << current_size << " put in dispatch queue.";
				output->Push(frame);
			}
		}
		return SPLIT;
	}
}
