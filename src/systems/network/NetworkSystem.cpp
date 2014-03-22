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
	NetworkSystem::NetworkSystem() : poller(), thread_pool(5) {};

	bool NetworkSystem::Start(const char *ip, unsigned short port) {
		Authentication::GetCryptoEngine()->InitializeDH();
		SetPipeline();

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
		NetworkSystem::GetStatefulMap()->Erase(fd);
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
						GetStatefulMap()->Insert(c.Handle(), AUTH_INIT);
					}
					else if (evList[i].flags & EVFILT_READ) {
						// Data received
						if (! GetStatefulMap()->Count(fd) || GetStatefulMap()->At(fd) != AUTHENTICATED) {
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
			std::bind(&NetworkSystem::ReassembleFrame, &pub_rawframe_req, &pub_frame_req, &thread_pool, &unauthenticated_recv_data, 0),
/*			// Read the length and get all the message
			[&](){
				auto req_list = NetworkSystem::GetPublicReassembledFrameQueue()->Poll();
				LOG_DEBUG << "got " << req_list->size() << " Get msg length events";
				for (auto& req : *req_list) {
					Frame_hdr* fheader = req->Length();
					if(fheader->length >= 512) {
						LOG_ERROR << "Length is more than 512 bytes for a public message. Closing connection";
						CloseConnection(req->fd);
					}
					LOG_DEBUG << "Reading length of " << fheader->length;
					// Update the length to read
					req->requested_length = fheader->length + sizeof(Frame_hdr);
					// We stop watching the connection until we got all the frame
					poller.Unwatch(fd);
					// Reading the rest of the packet
					NetworkSystem::GetPublicRawFrameReqQueue()->Push(std::move(req));
				}
				return NEXT;
			},
			// Get the messages
			std::bind(&NetworkSystem::ReassembleFrame, &pub_rawframe_req, &pub_frame_req, &thread_pool, &unauthenticated_recv_data, 2),
*/			// Read the messages and dispatch
			[&](){
				auto req_list = NetworkSystem::GetPublicReassembledFrameQueue()->Poll();
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
										NetworkSystem::GetAuthRequestQueue()->Push(req);
										GetThreadPool()->Queue(std::make_shared<TaskReq<block_t>>(std::bind(&NetworkSystem::RetrieveSalt, this, req, NetworkSystem::GetSaltRetrievedQueue())));
										break;
									}
								case DH_EXCHANGE:
									{
										if(! GetStatefulMap()->Count(req->fd) || GetStatefulMap()->At(req->fd) != DH_EXCHANGE) {
											// This is not the packet expected
											LOG_DEBUG << "Unexpected packet received with length = " << req->Length()->length;
											CloseConnection(req->fd);
										}
										NetworkSystem::GetDHKeyExchangeQueue()->Push(req);
										GetThreadPool()->Queue(std::make_shared<TaskReq<chain_t>>(shared_secret_key, 0));
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
				return NEXT;
			}
		});

		request_authentication = chain_t({
			// Send salt of the user
			[&](){
				auto req_list = NetworkSystem::GetSaltRetrievedQueue()->Poll();
				LOG_DEBUG << "got " << req_list->size() << " SaltRetrieved events";
				for (auto& req : *req_list) {

					auto reply = req->Length();
					if (reply) {
						// send salt
						SendMessage(req->fd, 1, 2, req);
						GetStatefulMap()->At(req->fd) = DH_EXCHANGE;
						LOG_DEBUG << "value = " << (int)(GetStatefulMap()->At(req->fd));
						LOG_DEBUG << "Send salt : " << std::string(reinterpret_cast<const char*>(req->Body()), 8);
						// Wait reply
						poller.Watch(req->fd);
					}
					else {
						// close connection
						// TODO: send error message
						//TCPConnection(req->fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(challenge.get()), sizeof(*challenge));
						LOG_DEBUG << "User unknown";
						CloseConnection(req->fd);
					}
				}
				return STOP;
			}

		});

		shared_secret_key = chain_t({
			[&](){
				auto req_list = NetworkSystem::GetDHKeyExchangeQueue()->Poll();
				LOG_DEBUG << "got " << req_list->size() << " DH Key events";
				for (auto& req : *req_list) {
					DHKeyExchangePacket* packet = reinterpret_cast<DHKeyExchangePacket*>(req->Body());
					if(packet->ComputeSharedKey()) {
						LOG_DEBUG << "VMAC check passed";
						continue;
					}
					else {
						LOG_DEBUG << "VMAC check failed";
					}
				}
				return STOP;
			}
		});
	}

	int NetworkSystem::RetrieveSalt(std::shared_ptr<FrameObject> frame, AtomicQueue<std::shared_ptr<FrameObject>>* output) {
		auto reply = std::make_shared<FrameObject>(frame->fd);
		reply->CreateBodySpace(sizeof(SendSaltPacket));
		// TODO : salt is hardcoded !!!
		// AuthInitPacket* packet = reinterpret_cast<AuthInitPacket*>(frame->Body());
		// GetSaltFromsomewhere(reply->Body(), packet->login);
		std::memcpy(reply->Body(), std::string("abcdefgh").data(), 8);
		output->Push(std::move(reply));
		GetThreadPool()->Queue(std::make_shared<TaskReq<chain_t>>(request_authentication, 0));
		return STOP;
	}

	int NetworkSystem::ReassembleFrame(AtomicQueue<std::shared_ptr<Frame_req>>* input, AtomicQueue<std::shared_ptr<FrameObject>>* output, ThreadPool* thread_pool, const chain_t* rerun, size_t index) {
		auto req_list = input->Poll();
		LOG_DEBUG << "got " << req_list->size() << " Reassemble frame requests";
		for (auto& req : *req_list) {
			auto target_size = req->length_requested;
			auto frame = req->reassembled_frame;
			char* buffer = reinterpret_cast<char*>(frame->Length());

			if (! buffer ) {
				frame->CreateFrameSpace(target_size);
				buffer = reinterpret_cast<char*>(frame->Length());
			}

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
				frame->CreateBodySpace(target_size);
				buffer = reinterpret_cast<char*>(frame->Length());
				frame->Length()->length = length;
			}

			if (current_size < target_size) {
				// we put again the request in the queue
				input->Push(req);
				thread_pool->Queue(std::make_shared<TaskReq<chain_t>>(*rerun, index));
			}
			else {
				LOG_DEBUG << "Packet of " << current_size << " put in dispatch queue.";
				output->Push(frame);
			}
		}
		return NEXT;
	}

	inline void NetworkSystem::RequestPublicFrameAsync(int fd, size_t len) {
	}

	void NetworkSystem::SendMessage(int fd, unsigned char major, unsigned char minor, char* body, uint32_t len) {
		Frame frame;
		frame.mheader.type_major = major;
		frame.mheader.type_minor = minor;
		frame.fheader.length = len + sizeof(msg_hdr);
		LOG_DEBUG << "Sending frame of length " << frame.fheader.length;
		auto cx = TCPConnection(fd, NETA_IPv4, SCS_CONNECTED);
		cx.Send(reinterpret_cast<char*>(&frame), sizeof(Frame));
		cx.Send(body, len);
	}

	void NetworkSystem::SendMessage(int fd, unsigned char major, unsigned char minor, const std::shared_ptr<FrameObject>& frame) {
		auto header = frame->Header();
		header->type_major = major;
		header->type_minor = minor;
		auto frame_h = frame->Header();
		frame->Length()->length = frame->PacketSize() - sizeof(Frame_hdr);
		LOG_DEBUG << "Sending frame of length " << frame->Length()->length;
		LOG_DEBUG << "Sending length with length " << sizeof(Frame_hdr);
		LOG_DEBUG << "Sending header with length " << sizeof(msg_hdr);
		LOG_DEBUG << "Sending body of length " << frame->Length()->length - sizeof(msg_hdr);
		LOG_DEBUG << "Sending packet of length " << frame->PacketSize();
		if (TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(frame->Length()), frame->PacketSize()) <= 0) {
			LOG_ERROR << "could not send data" ;
		};
	}
}
