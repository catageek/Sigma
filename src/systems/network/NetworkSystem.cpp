#include "systems/network/NetworkSystem.h"

#include <thread>
#include <chrono>
#include <cstring>
#include <sys/event.h>
#include "netport/include/net/network.h"
#include <typeinfo>
#include "composites/NetworkNode.h"

using namespace network;

namespace Sigma {
	NetworkSystem::NetworkSystem() {};

	ThreadPool NetworkSystem::thread_pool(5);
	IOPoller NetworkSystem::poller;
	Authentication NetworkSystem::authentication;

	bool NetworkSystem::Start(const char *ip, unsigned short port) {
		authentication.Initialize();
		authentication.GetCryptoEngine()->InitializeDH();

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
		poller.CreatePermanent(ssocket);
		auto tr = std::make_shared<TaskReq<chain_t>>(start);
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
		poller.Delete(fd);
		TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Close();
		NetworkSystem::GetAuthenticationComponent().GetAuthStateMap()->Erase(fd);
	}

	void NetworkSystem::SetPipeline() {
		start = chain_t({
			// Poll the socket and select the next chain to run
			[&](){
				std::vector<struct kevent> evList(32);
				auto nev = poller.Poll(evList);
				if (nev < 0) {
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
						c.SetNonBlocking(true);
						poller.Create(c.Handle());
						LOG_DEBUG << "connect " << c.Handle();
//						c.Send("welcome!\n");
						NetworkSystem::GetAuthenticationComponent().GetAuthStateMap()->Insert(c.Handle(), AUTH_INIT);
					}
					else if (evList[i].flags & EVFILT_READ) {
						// Data received
						if (! NetworkSystem::GetAuthenticationComponent().GetAuthStateMap()->Count(fd) || NetworkSystem::GetAuthenticationComponent().GetAuthStateMap()->At(fd) != AUTH_SHARE_KEY) {
							// Data received, not authenticated
							// Request the frame header
							// queue the task to reassemble the frame
							NetworkSystem::GetPublicRawFrameReqQueue()->Push(std::make_shared<Frame_req>(fd));
							GetThreadPool()->Queue(std::make_shared<TaskReq<chain_t>>(unauthenticated_recv_data));
						}
						else {
							// We stop watching the connection until we got all the frame
							LOG_DEBUG << "got authenticated frame";
							// Data received from authenticated client
							NetworkSystem::GetAuthenticatedRawFrameReqQueue()->Push(std::make_shared<Frame_req>(fd, reinterpret_cast<vmac_pair*>(evList[i].udata)));
							GetThreadPool()->Queue(std::make_shared<TaskReq<chain_t>>(authenticated_recv_data));
						}
					}
				}
				// Queue a task to wait another event
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				return REPEAT;
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
										network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_INIT>();
										break;
									}
								case AUTH_KEY_EXCHANGE:
									{
										network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_KEY_EXCHANGE>()->Push(req);
										network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_EXCHANGE>();
										break;
									}
								default:
									{
										LOG_DEBUG << "invalid minor code, closing";
										CloseConnection(req->fd);
									}
							}
							break;
						}
					default:
						{
							LOG_DEBUG << "invalid major code, closing";
							CloseConnection(req->fd);
						}
					}
				}
				return STOP;
			}
		});

		authenticated_recv_data = chain_t({
			// Get the length
			std::bind(&NetworkSystem::ReassembleFrame, &auth_rawframe_req, &auth_frame_req, &thread_pool),
			[&](){
				std::shared_ptr<FrameObject> req;
				if(! NetworkSystem::GetAuthenticatedReassembledFrameQueue()->Pop(req)) {
					return STOP;
				}
				if(req->Verify_VMAC_tag()) {
					NetworkSystem::GetAuthenticatedCheckedFrameQueue()->Push(req);
				}
				else {
					LOG_DEBUG << "Dropping 1 frame (VMAC check failed)";
				}
				return CONTINUE;
			},
			[&](){
				auto req_list = NetworkSystem::GetAuthenticatedCheckedFrameQueue()->Poll();
				if (! req_list) {
					return STOP;
				}
				LOG_DEBUG << "got " << req_list->size() << " AuthenticatedCheckedDispatchReq events";
				for (auto& req : *req_list) {
					msg_hdr* header = req->Header();
					auto major = header->type_major;
					switch(major) {
						// select handlers
					case TEST:
						{
							auto minor = header->type_minor;
							switch(minor) {
								case TEST:
									{
										network_packet_handler::INetworkPacketHandler::GetQueue<TEST,TEST>()->Push(req);
										network_packet_handler::INetworkPacketHandler::Process<TEST,TEST>();
										break;
									}
								default:
									{
										CloseConnection(req->fd);
									}
							}
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
		std::shared_ptr<Frame_req> req;
		if(! input->Pop(req)) {
			return STOP;
		}
		auto target_size = req->length_requested;
		auto frame = req->reassembled_frame;
		char* buffer = reinterpret_cast<char*>(frame->FullFrame());

		auto current_size = req->length_got;

		auto len = TCPConnection(frame->fd, NETA_IPv4, SCS_CONNECTED).Recv(reinterpret_cast<char*>(buffer) + current_size, target_size - current_size);
		current_size += len;
		req->length_got = current_size;

		if(current_size == sizeof(Frame_hdr) && target_size == sizeof(Frame_hdr)) {
			// We now have the length
			auto length = frame->FullFrame()->length;
			target_size = frame->FullFrame()->length + sizeof(Frame_hdr);
			req->length_requested = target_size;
			frame->Resize<false>(length - sizeof(msg_hdr));
			buffer = reinterpret_cast<char*>(frame->FullFrame());
			auto len = TCPConnection(frame->fd, NETA_IPv4, SCS_CONNECTED).Recv(reinterpret_cast<char*>(buffer) + current_size, target_size - current_size);
			current_size += len;
			req->length_got = current_size;
		}

		if (current_size < target_size) {
			// we put again the request in the queue
			LOG_DEBUG << "got only " << current_size << " bytes, waiting " << target_size << " bytes";
			input->Push(req);
			return REPEAT;
		}
		else {
			LOG_DEBUG << "Packet of " << current_size << " put in dispatch queue.";
			poller.Watch(frame->fd);
			output->Push(frame);
		}
		return CONTINUE;
	}
}
