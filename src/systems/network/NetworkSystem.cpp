#include "systems/network/NetworkSystem.h"

#include <thread>
#include <chrono>
#include <sys/event.h>
#include "netport/include/net/network.h"
#include "systems/network/Protocol.h"
#include "systems/network/ThreadPool.h"
#include "systems/network/Authentication.h"
// Hom many seconds to wait the first bytes
#define WAIT_DATA 3

#define OPEN_CHAIN chaint_t({
#define OPEN_LAMBDA [&]() -> int (
#define CLOSE_LAMBDA )
#define CLOSE_CHAIN })

using namespace network;

namespace Sigma {
	NetworkSystem::NetworkSystem() : poller(), thread_pool(5) {};

	bool NetworkSystem::Start(const char *ip, unsigned short port) {
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
		auto tr = new TaskReq(start, 0);
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
		NetworkSystem::GetPendingSet()->Erase(fd);
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
						NetworkSystem::GetPendingSet()->Insert(c.Handle());
					}
					else if (evList[i].flags & EVFILT_READ) {
						// Data received
						if (NetworkSystem::GetPendingSet()->Count(fd)) {
							// Data received, not authenticated
							// Build a fram request
							auto fr = std::make_shared<Frame_req>(fd, sizeof(msg_hdr));
							// We stop watching the connection until we got all the frame
							poller.Unwatch(fd);
							// queue the task to reassemble the header
							NetworkSystem::GetPublicRawFrameReqQueue()->Push(fr);
							GetThreadPool()->Queue(new TaskReq(unauthenticated_reassemble, 0));
						}
						else {
							// Data received from authenticated client
							NetworkSystem::GetDataRecvQueue()->Push(fd);
						}
					}
				}
				// Queue a task to wait another event
				GetThreadPool()->Queue(new TaskReq(start, 0));
				return STOP;
			}
		});

		unauthenticated_reassemble = chain_t({
			// Reassemble the frames
			[&](){
				auto req_list = NetworkSystem::GetPublicRawFrameReqQueue()->Poll();
				LOG_DEBUG << "got " << req_list->size() << " PublicRawFrameReq events";
				for (auto& req : *req_list) {
					auto current_size = req->reassembled_frame->length;
					auto target_size = req->length_requested;
					auto buffer = req->reassembled_frame->data;
					if (! buffer) {
						req->reassembled_frame->data = std::make_shared<std::vector<char>>(target_size);
						buffer = req->reassembled_frame->data;
					}
					auto len = TCPConnection(req->reassembled_frame->fd, NETA_IPv4, SCS_CONNECTED).Recv(buffer->data() + current_size, target_size - current_size);
					LOG_DEBUG << "got " << len << " bytes in reassemble_frame for fd = " << req->reassembled_frame->fd << ", target = " << target_size << " current = " << current_size;
					current_size += len;
					if (current_size < target_size) {
						// we put again the request in the queue
						req->reassembled_frame->length = current_size;
						NetworkSystem::GetPublicRawFrameReqQueue()->Push(req);
						GetThreadPool()->Queue(new TaskReq(unauthenticated_reassemble, 0));
					}
					else {
						NetworkSystem::GetPublicReassembledFrameQueue()->Push(req->reassembled_frame);
					}
				}
				return NEXT;
			},
			// Read the header
			[&](){
				auto req_list = NetworkSystem::GetPublicReassembledFrameQueue()->Poll();
				LOG_DEBUG << "got " << req_list->size() << " PublicReassembleReq events";
				for (auto& req : *req_list) {
					msg_hdr* header = reinterpret_cast<msg_hdr*>(req->data->data());
					auto major = header->type_major;
					if (IS_RESTRICTED(major)) {
						LOG_DEBUG << "restricted";
						CloseConnection(req->fd);
						break;
					}
					switch(major) {
					case NET_MSG:
						if (header->type_minor == AUTH_REQUEST) {
							NetworkSystem::GetAuthRequestQueue()->Push(req->fd);
							GetThreadPool()->Queue(new TaskReq(request_authentication, 0));
						}
						break;
					default:
						CloseConnection(req->fd);
					}
				}
				return NEXT;
			},
		});

		request_authentication = chain_t({
			// Get authentication initial packet
			[&](){
				auto req_list = NetworkSystem::GetAuthRequestQueue()->Poll();
				LOG_DEBUG << "got " << req_list->size() << " AuthReq events";
				for (auto& req : *req_list) {
					auto packet = std::make_shared<AuthInitPacket>();
					auto len = TCPConnection(req, NETA_IPv4, SCS_CONNECTED).Recv(reinterpret_cast<char*>(packet.get()), sizeof(*packet));
					LOG_DEBUG << "received " << len << " bytes";
					auto task_request = std::make_shared<ChallengePrepareTaskRequest>(req, std::move(packet));
					NetworkSystem::GetChallengeRequestQueue()->Push(std::move(task_request));
				}
				return NEXT;
			},
			// Send challenge
			[&](){
				while(1) {
					auto req_list = NetworkSystem::GetChallengeRequestQueue()->Poll();
					LOG_DEBUG << "got " << req_list->size() << " ChallReq events";
					if (req_list->empty()) {
						break;
					}
					for (auto& req : *req_list) {
						auto challenge = req->packet->GetChallenge();
						if (challenge) {
							msg_hdr header;
							header.length = sizeof(AuthChallReqPacket);
							header.type_major = 1;
							header.type_minor = 2;
							// send challenge
							TCPConnection(req->fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(&header), sizeof(msg_hdr));
							TCPConnection(req->fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(challenge.get()), sizeof(AuthChallReqPacket));
							LOG_DEBUG << "Send challenge.";
							// Wait reply
							poller.Watch(req->fd);
						}
						else {
							// close connection
							// TODO: send error message
							//TCPConnection(req->fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(challenge.get()), sizeof(*challenge));
							LOG_DEBUG << "User unknown: " << req->packet->login;
							CloseConnection(req->fd);
						}
					}
				}
				return STOP;
			}
		});
	}
}
