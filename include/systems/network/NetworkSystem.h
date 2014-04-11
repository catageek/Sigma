#ifndef NETWORKSYSTEM_H_INCLUDED
#define NETWORKSYSTEM_H_INCLUDED

#include <thread>
#include <condition_variable>
#include <mutex>
#include <algorithm>
#include "Sigma.h"
#include "systems/network/IOPoller.h"
#include "AtomicQueue.hpp"
#include "systems/network/Protocol.h"
#include "systems/network/AuthenticationHandler.h"
#include "netport/include/net/network.h"
#include "systems/network/FrameRequest.h"

namespace network {
	class TCPConnection;
};

namespace Sigma {
	class IOPoller;
	class GetSaltTaskRequest;

	using namespace network;

	extern template bool FrameObject::Verify_Authenticity<false>();
	extern template	bool Frame_req::CheckVMAC<false, false>() const;
	extern template bool Frame_req::CheckVMAC<true, false>() const;

	namespace network_packet_handler {
		extern template void INetworkPacketHandler::Process<NET_MSG,AUTH_INIT, false>();
		extern template void INetworkPacketHandler::Process<NET_MSG,AUTH_SEND_SALT, true>();
		extern template void INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_EXCHANGE, false>();
		extern template void INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_REPLY, true>();
		extern template void INetworkPacketHandler::Process<TEST,TEST, false>();
		extern template void INetworkPacketHandler::Process<TEST,TEST, true>();
	}

	namespace network_packet_handler {
		class INetworkPacketHandler;
	}

	class NetworkSystem {
	public:
		friend void Authentication::SetHasher(std::unique_ptr<cryptography::VMAC_StreamHasher>&& hasher);
		friend void Authentication::SetAuthState(uint32_t state);
		friend int Authentication::SendSalt() const;
		friend void Authentication::CheckKeyExchange(const std::list<std::shared_ptr<FrameObject>>& req_list);
		friend void Authentication::CreateSecureKey(const std::list<std::shared_ptr<FrameObject>>& req_list);
		friend void FrameObject::SendMessage(unsigned char major, unsigned char minor);

//		extern template	bool CheckVMAC<true,false>(const std::list<std::unique_ptr<FrameObject>>& frames) const;
//		extern template	bool CheckVMAC<false,false>(const std::list<std::unique_ptr<FrameObject>>& frames) const;

		DLL_EXPORT NetworkSystem() throw (std::runtime_error);
		DLL_EXPORT virtual ~NetworkSystem() {};


        /** \brief Start the server to accept players
         *
         * \param ip const char* the address to listen
         *
         */
		DLL_EXPORT bool Server_Start(const char *ip, unsigned short port);
		DLL_EXPORT bool Connect(const char *ip, unsigned short port, const std::string& login, const std::string& password);

		template<bool isClient>
		DLL_EXPORT void SetTCPHandler() {

			authentication.Initialize<isClient>(this);

			start = chain_t({
				// Poll the socket and select the next chain to run
				[&](){
//					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					std::vector<struct kevent> evList(32);
					LOG_DEBUG << "(" << sched_getcpu() << ") Polling...";
					auto nev = Poller()->Poll(evList);
					if (nev < 0) {
						LOG_ERROR << "(" << sched_getcpu() << ") Error when polling event";
					}
					LOG_DEBUG << "(" << sched_getcpu() << ") got " << nev << " kqueue events";
					bool a = false, b = false;
					for (auto i=0; i<nev; i++) {
						auto fd = evList[i].ident;
						if (evList[i].flags & EV_EOF) {
							LOG_DEBUG << "(" << sched_getcpu() << ") disconnect " << fd << " with " << evList[i].fflags << " code";
							if(evList[i].udata) {
								CloseConnection(fd, reinterpret_cast<vmac_pair*>(evList[i].udata)->first);
							}
							else {
								CloseConnection(fd);
							}
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
							NetworkSystem::GetAuthStateMap<isClient>()->Insert(c.Handle(), AUTH_INIT);
						}
						else if (evList[i].flags & EVFILT_READ) {
							// Data received
							if (! NetworkSystem::GetAuthStateMap<isClient>()->Compare(fd, AUTH_SHARE_KEY)) {
								// Data received, not authenticated
								// Request the frame header
								// queue the task to reassemble the frame
								LOG_DEBUG << "(" << sched_getcpu() << ") got unauthenticated frame of " << evList[i].data << " bytes";
								auto max_size = std::min(evList[i].data, 128L);
								GetPublicRawFrameReqQueue()->Push(make_unique<Frame_req>(fd, max_size));
								a = true;
							}
							else {
								// We stop watching the connection until we got all the frame
								LOG_DEBUG << "(" << sched_getcpu() << ") got authenticated frame of " << evList[i].data << " bytes";
								// Data received from authenticated client
								auto max_size = std::min(evList[i].data, 1460L);
								GetAuthenticatedRawFrameReqQueue()->Push(make_unique<Frame_req>(fd, max_size, reinterpret_cast<vmac_pair*>(evList[i].udata)));
								b = true;
							}
						}
					}
					if(a) {
						ThreadPool::Queue(std::make_shared<TaskReq<chain_t>>(unauthenticated_recv_data));
					}
					if(b) {
						ThreadPool::Queue(std::make_shared<TaskReq<chain_t>>(start));
						ThreadPool::Execute(std::make_shared<TaskReq<chain_t>>(authenticated_recv_data));
					}
					else {
						// Queue a task to wait another event
						return REQUEUE;
					}
					return STOP;
				}
			});

			unauthenticated_recv_data = chain_t({
				// Get the length
				std::bind(&NetworkSystem::ReassembleFrame<isClient, false>, this, &pub_rawframe_req, &pub_frame_req),
				[&](){
					auto req_list = GetPublicReassembledFrameQueue()->Poll();
					if (! req_list) {
						return STOP;
					}
					LOG_DEBUG << "(" << sched_getcpu() << ") got " << req_list->size() << " PublicDispatchReq events";
					for (auto& req : *req_list) {
						msg_hdr* header = req->Header();
						auto major = header->type_major;
						if (IS_RESTRICTED(major)) {
							LOG_DEBUG << "restricted";
							CloseConnection(req->fd, req->GetId());
							break;
						}
						switch(major) {
						case NET_MSG:
							{
								auto minor = header->type_minor;
								switch(minor) {
									case AUTH_INIT:
										{
											network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_INIT>()->Push(std::move(req));
											break;
										}
									case AUTH_SEND_SALT:
										{
											network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_SEND_SALT>()->Push(std::move(req));
											break;
										}
									case AUTH_KEY_EXCHANGE:
										{
											network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_KEY_EXCHANGE>()->Push(std::move(req));
											break;
										}
									case AUTH_KEY_REPLY:
										{
											network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_KEY_REPLY>()->Push(std::move(req));
											break;
										}
									default:
										{
											LOG_DEBUG << "(" << sched_getcpu() << ") invalid minor code, closing";
											CloseConnection(req->fd, req->GetId());
										}
								}
								break;
							}
						default:
							{
								LOG_DEBUG << "(" << sched_getcpu() << ") invalid major code, closing";
								CloseConnection(req->fd, req->GetId());
							}
						}
					}
					if(! network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG, AUTH_INIT>()->Empty()) {
						network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_INIT, isClient>();
					}
					if(! network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG, AUTH_SEND_SALT>()->Empty()) {
						network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_SEND_SALT, isClient>();
					}
					if(! network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG, AUTH_KEY_EXCHANGE>()->Empty()) {
						network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_EXCHANGE, isClient>();
					}
					if(! network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG, AUTH_KEY_REPLY>()->Empty()) {
						network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_REPLY, isClient>();
					}
					return STOP;
				}
			});

			authenticated_recv_data = chain_t({
				// Get the length
				std::bind(&NetworkSystem::ReassembleFrame<isClient, true>, this, &auth_rawframe_req, &auth_checked_frame_req),
				[&](){
					auto req_list = NetworkSystem::GetAuthenticatedCheckedFrameQueue()->Poll();
					if (! req_list) {
						return STOP;
					}
					LOG_DEBUG << "(" << sched_getcpu() << ") got " << req_list->size() << " AuthenticatedCheckedDispatchReq events";
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
											network_packet_handler::INetworkPacketHandler::GetQueue<TEST,TEST>()->Push(std::move(req));
											network_packet_handler::INetworkPacketHandler::Process<TEST,TEST, isClient>();
											break;
										}
									default:
										{
											LOG_ERROR << "(" << sched_getcpu() << ") TEST: closing";
											CloseConnection(req->fd, req->GetId());
										}
								}
								break;
							}

						default:
							{
								LOG_ERROR << "(" << sched_getcpu() << ") Authenticated switch: closing";
								CloseConnection(req->fd, req->GetId());
							}
						}
					}
					return STOP;
				}
			});
		}

	private:
        /** \brief The listener waits for connections and pass new connections to the IncomingConnection
         *
         * \param url const char* the address to listen
         *
         */
		std::unique_ptr<network::TCPConnection> Listener(const char *ip, unsigned short port);

		void SetHasher(std::unique_ptr<cryptography::VMAC_StreamHasher>&& hasher) { this->hasher = std::move(hasher); };
		cryptography::VMAC_StreamHasher* Hasher() { return hasher.get(); };

		void SetAuthState(uint32_t state) { auth_state.store(state); };
		uint32_t AuthState() const { return auth_state.load(); };

		template<bool isClient = false>
		static const AtomicMap<int,char>* const GetAuthStateMap() { return &auth_state_map; };

		const AtomicQueue<std::unique_ptr<Frame_req>>* const GetAuthenticatedRawFrameReqQueue() { return &auth_rawframe_req; };
		const AtomicQueue<std::unique_ptr<FrameObject>>* const GetAuthenticatedCheckedFrameQueue() { return &auth_checked_frame_req; };
		const AtomicQueue<std::unique_ptr<Frame_req>>* const GetPublicRawFrameReqQueue() const { return &pub_rawframe_req; };
		const AtomicQueue<std::unique_ptr<FrameObject>>* const GetPublicReassembledFrameQueue() { return &pub_frame_req; };

		const IOPoller* const Poller() { return &poller; };

		void CloseClientConnection();
		void CloseConnection(int fd, id_t id) const;
		void CloseConnection(int fd) const;

		int ssocket;											// the listening socket
		mutable IOPoller poller;
		const Authentication authentication;
		static AtomicMap<int,char> auth_state_map;						// state of the connections
		mutable std::mutex net_mutex;

		chain_t start;
		chain_t unauthenticated_recv_data;
		chain_t authenticated_recv_data;

		const AtomicQueue<std::unique_ptr<Frame_req>> auth_rawframe_req;				// raw frame request, to be authenticated
		const AtomicQueue<std::unique_ptr<FrameObject>> auth_checked_frame_req;		// reassembled frame request, authenticated
		const AtomicQueue<std::unique_ptr<Frame_req>> pub_rawframe_req;				// raw frame requests
		const AtomicQueue<std::unique_ptr<FrameObject>> pub_frame_req;				// reassembled frame request

		// members for client only
		static AtomicMap<int,char> auth_state_client;						// state of the connections
		std::unique_ptr<cryptography::VMAC_StreamHasher> hasher;
		std::atomic_uint_least32_t auth_state;
		network::TCPConnection cnx;
		std::condition_variable is_connected;
		std::mutex connecting;

		template<bool isClient, bool hasVMAC>
		int ReassembleFrame(const AtomicQueue<std::unique_ptr<Frame_req>>* const input, const AtomicQueue<std::unique_ptr<FrameObject>>* const output) const {
			auto req_list = input->Poll();
			if (! req_list) {
				return STOP;
			}
			auto ret = CONTINUE;
			LOG_DEBUG << "(" << sched_getcpu() << ") got " << req_list->size() << " Reassemble events";
			for (auto& req : *req_list) {
				auto target_size = req->length_requested;
				LOG_DEBUG << "(" << sched_getcpu() << ")  Reassembling " << target_size << " bytes";
				auto frame = req->reassembled_frames_list.back().get();
				char* buffer = reinterpret_cast<char*>(frame->FullFrame());

				auto current_size = req->length_got;
				int len;

				// lock mutex: Only 1 thread from here
				net_mutex.lock();
				len = TCPConnection(frame->fd, NETA_IPv4, SCS_CONNECTED).Recv(reinterpret_cast<char*>(buffer) + current_size, target_size - current_size);
				if(len < 0) {
					LOG_ERROR << "(" << sched_getcpu() << ") Could not read data";
					continue;
				}
				current_size += len;
				req->length_got = current_size;

				if(current_size == sizeof(Frame_hdr) && target_size == sizeof(Frame_hdr)) {
					// We now have the length
					auto length = frame->FullFrame()->length;
					target_size = frame->FullFrame()->length + sizeof(Frame_hdr);
					LOG_DEBUG << "(" << sched_getcpu() << ")  Completing message for a total of " << target_size << " bytes";
					req->length_requested = target_size;
					frame->Resize<false>(length - sizeof(msg_hdr));
					buffer = reinterpret_cast<char*>(frame->FullFrame());
					len = TCPConnection(frame->fd, NETA_IPv4, SCS_CONNECTED).Recv(reinterpret_cast<char*>(buffer) + current_size, target_size - current_size);
					if(len < 0) {
						LOG_ERROR << "(" << sched_getcpu() << ") Could not read data";
						// release mutex
						net_mutex.unlock();
						continue;
					}
					current_size += len;
					req->length_got = current_size;
				}

				if (current_size < target_size) {
					// release mutex
					net_mutex.unlock();
					// we put again the request in the queue
					LOG_DEBUG << "(" << sched_getcpu() << ") got only " << current_size << " bytes, waiting " << target_size << " bytes";
					input->Push(std::move(req));
					if(ret == CONTINUE) {
						ret = REPEAT;
					}
				}
				else {
					if( current_size < req->length_total ) {
						// release mutex
						net_mutex.unlock();
						LOG_DEBUG << "(" << sched_getcpu() << ") Packet of " << current_size << " put in frame queue.";
						req->length_total -= current_size;
						req->length_got = 0;
						req->length_requested = sizeof(Frame_hdr);
						LOG_DEBUG << "(" << sched_getcpu() << ") Get another packet #" << req->reassembled_frames_list.size() << " from fd #" << frame->fd << " frome same frame.";

						req->reassembled_frames_list.emplace_back(new FrameObject(frame->fd, frame->GetVMACVerifier()));
						input->Push(std::move(req));
						ret = REPEAT;
					}
					else {
						if (! req->CheckVMAC<isClient, hasVMAC>()) {
							// release mutex
							net_mutex.unlock();
							LOG_DEBUG << "(" << sched_getcpu() << ") Dropping all frames and closing (VMAC check failed)";
							CloseConnection(frame->fd);
							continue;
						}
						// release mutex
						net_mutex.unlock();
						poller.Watch<isClient>(frame->fd);

						LOG_DEBUG << "(" << sched_getcpu() << ") Moving " << req->reassembled_frames_list.size() << " messages with a total of " << req->length_got << " bytes to queue";
						output->PushList(std::move(req->reassembled_frames_list));
						if(ret == REPEAT) {
							ret = SPLIT;
						}
					}
				}
			}
			return ret;
		}
	};
}

#endif // NETWORKSYSTEM_H_INCLUDED
