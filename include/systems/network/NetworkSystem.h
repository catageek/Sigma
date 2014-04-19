#ifndef NETWORKSYSTEM_H_INCLUDED
#define NETWORKSYSTEM_H_INCLUDED

#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <algorithm>
#include "Sigma.h"
#include "systems/network/Network.h"
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

	namespace network_packet_handler {
		extern template void INetworkPacketHandler<SERVER>::Process<NET_MSG,AUTH_INIT>();
		extern template void INetworkPacketHandler<CLIENT>::Process<NET_MSG,AUTH_SEND_SALT>();
		extern template void INetworkPacketHandler<SERVER>::Process<NET_MSG,AUTH_KEY_EXCHANGE>();
		extern template void INetworkPacketHandler<CLIENT>::Process<NET_MSG,AUTH_KEY_REPLY>();
		extern template void INetworkPacketHandler<SERVER>::Process<TEST,TEST>();
		extern template void INetworkPacketHandler<CLIENT>::Process<TEST,TEST>();
	}

	namespace network_packet_handler {
		template<TagType T> class INetworkPacketHandler;
	}

	class NetworkSystem {
	public:
		template<TagType T> friend void Authentication::SetHasher(std::unique_ptr<std::function<void(unsigned char*,const unsigned char*,size_t)>>&& hasher);
		template<TagType T> friend void Authentication::SetVerifier(std::unique_ptr<std::function<bool(const unsigned char*,const unsigned char*,size_t)>>&& verifier);
		template<TagType T> friend void Authentication::SetAuthState(uint32_t state);
		friend int Authentication::SendSalt() const;
		friend void Authentication::CheckKeyExchange(const std::list<std::shared_ptr<FrameObject>>& req_list);
		friend void Authentication::CreateSecureKey(const std::list<std::shared_ptr<FrameObject>>& req_list);
		friend void FrameObject::SendMessage(unsigned char major, unsigned char minor);
		friend void FrameObject::SendMessage(id_t, unsigned char major, unsigned char minor);

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

    	template<TagType T>
		DLL_EXPORT void SetTCPHandler() {

			authentication.Initialize<T>(this);

			start = chain_t({
				// Poll the socket and select the next chain to run
				[&](){
//					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					std::vector<struct kevent> evList(32);
//					LOG_DEBUG << "(" << sched_getcpu() << ") Polling...";
					auto nev = Poller()->Poll(evList);
					if (nev < 0) {
						LOG_ERROR << "(" << sched_getcpu() << ") Error when polling event";
					}
//					LOG_DEBUG << "(" << sched_getcpu() << ") got " << nev << " kqueue events";
					bool a = false, b = false;
					for (auto i=0; i<nev; i++) {
						auto fd = evList[i].ident;
						if (evList[i].flags & EV_EOF) {
							LOG_DEBUG << "(" << sched_getcpu() << ") disconnect " << fd << " with " << evList[i].fflags << " code, " << evList[i].data << " bytes left.";
							if(evList[i].udata) {
								CloseConnection(fd, reinterpret_cast<ConnectionData*>(evList[i].udata));
							}
							else {
								RemoveConnection(fd);
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
							poller.Create(c.Handle(), reinterpret_cast<void*>(new ConnectionData(AUTH_INIT)));
							LOG_DEBUG << "connect " << c.Handle();
	//						c.Send("welcome!\n");

						}
						else if (evList[i].flags & EVFILT_READ) {
							// Data received
							auto cx_data = reinterpret_cast<ConnectionData*>(evList[i].udata);
							if (! cx_data || ! cx_data->TryLockConnection()) {
								continue;
							}
							if (! cx_data->CompareAuthState(AUTH_SHARE_KEY)) {
								// Data received, not authenticated
								// Request the frame header
								// queue the task to reassemble the frame
//								LOG_DEBUG << "(" << sched_getcpu() << ") got unauthenticated frame of " << evList[i].data << " bytes";
								auto max_size = std::min(evList[i].data, 128L);
								GetPublicRawFrameReqQueue()->Push(make_unique<Frame_req>(fd, max_size, cx_data));
								a = true;
							}
							else {
								// We stop watching the connection until we got all the frame
//								LOG_DEBUG << "(" << sched_getcpu() << ") got authenticated frame of " << evList[i].data << " bytes";
								// Data received from authenticated client
								auto max_size = std::min(evList[i].data, 1460L);
								GetAuthenticatedRawFrameReqQueue()->Push(make_unique<Frame_req>(fd, max_size, cx_data));
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
				std::bind(&NetworkSystem::ReassembleFrame<T, NONE>, this, &pub_rawframe_req, &pub_frame_req),
                std::bind(&NetworkSystem::UnauthenticatedDispatch<T>, this)
			});

			authenticated_recv_data = chain_t({
				// Get the length
				std::bind(&NetworkSystem::ReassembleFrame<T>, this, &auth_rawframe_req, &auth_checked_frame_req),
                std::bind(&NetworkSystem::AuthenticatedDispatch<T>, this)
			});
		}
		void SetServerPublicKey(std::unique_ptr<std::vector<unsigned char>>&& key) { serverPublicKey = std::move(key); };

	private:
        /** \brief The listener waits for connections and pass new connections to the IncomingConnection
         *
         * \param url const char* the address to listen
         *
         */
		std::unique_ptr<network::TCPConnection> Listener(const char *ip, unsigned short port);

		void SetHasher(std::unique_ptr<std::function<void(unsigned char*,const unsigned char*,size_t)>>&& hasher) { this->hasher = std::move(hasher); };

		std::function<void(unsigned char*,const unsigned char*,size_t)>* Hasher() { return hasher.get(); };

		void SetVerifier(std::unique_ptr<std::function<bool(const unsigned char*,const unsigned char*,size_t)>>&& verifier) { this->verifier = std::move(verifier); };

		std::function<bool(const unsigned char*,const unsigned char*,size_t)>* Verifier() { return verifier.get(); };

		void SetAuthState(uint32_t state) const { auth_state.store(state); };
		uint32_t AuthState() const { return auth_state.load(); };

		const AtomicQueue<std::unique_ptr<Frame_req>>* const GetAuthenticatedRawFrameReqQueue() const { return &auth_rawframe_req; };
		const AtomicQueue<std::unique_ptr<FrameObject>>* const GetAuthenticatedCheckedFrameQueue() const { return &auth_checked_frame_req; };
		const AtomicQueue<std::unique_ptr<Frame_req>>* const GetPublicRawFrameReqQueue() const { return &pub_rawframe_req; };
		const AtomicQueue<std::unique_ptr<FrameObject>>* const GetPublicReassembledFrameQueue() const { return &pub_frame_req; };

		const IOPoller* const Poller() { return &poller; };

		// specialization in network.cpp
    	template<TagType T>
		void CloseConnection(const FrameObject* frame) const;

		// specialization in network.cpp
    	template<TagType T>
        int UnauthenticatedDispatch() const;

		// specialization in network.cpp
    	template<TagType T>
        int AuthenticatedDispatch() const;

		void RemoveClientConnection() const;
		void CloseConnection(const int fd, const ConnectionData* cx_data) const;
		void RemoveConnection(const int fd) const;

		const std::vector<unsigned char>* ServerPublicKey() const { return serverPublicKey.get(); };

		int ssocket;											// the listening socket
		mutable IOPoller poller;
		const Authentication authentication;

		chain_t start;
		chain_t unauthenticated_recv_data;
		chain_t authenticated_recv_data;

		const AtomicQueue<std::unique_ptr<Frame_req>> auth_rawframe_req;				// raw frame request, to be authenticated
		const AtomicQueue<std::unique_ptr<FrameObject>> auth_checked_frame_req;		// reassembled frame request, authenticated
		const AtomicQueue<std::unique_ptr<Frame_req>> pub_rawframe_req;				// raw frame requests
		const AtomicQueue<std::unique_ptr<FrameObject>> pub_frame_req;				// reassembled frame request

        std::unique_ptr<std::vector<unsigned char>> serverPublicKey;
		std::unique_ptr<std::function<bool(const unsigned char*,const unsigned char*,size_t)>> verifier;
		// members for client only
		std::unique_ptr<std::function<void(unsigned char*,const unsigned char*,size_t)>> hasher;
		mutable std::atomic_uint_least32_t auth_state;
		network::TCPConnection cnx;
		mutable std::condition_variable is_connected;
		std::mutex connecting;

		template<TagType T, TagType checkAuth = T>
		int ReassembleFrame(const AtomicQueue<std::unique_ptr<Frame_req>>* const input, const AtomicQueue<std::unique_ptr<FrameObject>>* const output) const {
			auto req_list = input->Poll();
			if (req_list.empty()) {
				return STOP;
			}
			auto ret = CONTINUE;
//			LOG_DEBUG << "(" << sched_getcpu() << ") got " << req_list->size() << " Reassemble events";
			for (auto& req : req_list) {
				auto target_size = req->length_requested;
//				LOG_DEBUG << "(" << sched_getcpu() << ")  Reassembling " << target_size << " bytes";
				auto frame = req->reassembled_frames_list.back().get();
				char* buffer = reinterpret_cast<char*>(frame->FullFrame());

				auto current_size = req->length_got;
				int len;

				len = TCPConnection(req->fd, NETA_IPv4, SCS_CONNECTED).Recv(reinterpret_cast<char*>(buffer) + current_size, target_size - current_size);
				if(len < 0) {
					LOG_ERROR << "(" << sched_getcpu() << ") Could not read data";
					continue;
				}

				if (len > 65535) {
					LOG_DEBUG << "(" << sched_getcpu() << ") Packet length exceeding 65535 bytes. closing";
					CloseConnection<T>(frame);
					continue;
				}
				current_size += len;
				req->length_got = current_size;

				if (current_size == sizeof(Frame_hdr) && target_size == sizeof(Frame_hdr)) {
					// We now have the length
					auto length = frame->FullFrame()->length;
					target_size = frame->FullFrame()->length + sizeof(Frame_hdr);
//					LOG_DEBUG << "(" << sched_getcpu() << ")  Completing message to " << target_size << " bytes in a frame of " << req->length_total << " bytes";
					req->length_requested = target_size;
					frame->Resize<NONE>(length - sizeof(msg_hdr));
					buffer = reinterpret_cast<char*>(frame->FullFrame());
					len = TCPConnection(req->fd, NETA_IPv4, SCS_CONNECTED).Recv(reinterpret_cast<char*>(buffer) + current_size, target_size - current_size);
					if (len < 0) {
						LOG_ERROR << "(" << sched_getcpu() << ") Could not read data";
						continue;
					}
					current_size += len;
					req->length_got = current_size;
				}

				if (current_size < target_size) {
					// missing bytes
					if (req->HasExpired()) {
						// Frame reassembly is stopped after 3 seconds if message is uncomplete
						LOG_DEBUG << "(" << sched_getcpu() << ") Dropping all frames and closing (timeout)";
						CloseConnection<T>(frame);
						continue;
					}
					// we put again the request in the queue
//					LOG_DEBUG << "(" << sched_getcpu() << ") got only " << current_size << " bytes, waiting " << target_size << " bytes";
					input->Push(std::move(req));
					if (ret == CONTINUE) {
						ret = REPEAT;
					}
				}
				else {
					if ( current_size < req->length_total ) {
						// message retrieved, but there are still bytes to read
//						LOG_DEBUG << "(" << sched_getcpu() << ") Packet of " << current_size << " put in frame queue.";
						req->length_total -= current_size;
						req->length_got = 0;
						req->length_requested = sizeof(Frame_hdr);
//						LOG_DEBUG << "(" << sched_getcpu() << ") Get another packet #" << req->reassembled_frames_list.size() << " from fd #" << frame->fd << " frome same frame.";

						req->reassembled_frames_list.emplace_back(new FrameObject(req->fd, req->CxData()));
						// reset the timestamp to now
						req->UpdateTimestamp();
						// requeue the frame request for next message
						input->Push(std::move(req));
						ret = REPEAT;
					}
					else {
						// request completed
						// check integrity
						if (! req->CheckVMAC<checkAuth>()) {
							LOG_DEBUG << "(" << sched_getcpu() << ") Dropping all frames and closing (VMAC check failed)";
							CloseConnection<T>(frame);
							continue;
						}
						// We allow again events on this socket
						req->CxData()->ReleaseConnection();
						poller.Watch(req->fd);

//						LOG_DEBUG << "(" << sched_getcpu() << ") Moving " << req->reassembled_frames_list.size() << " messages with a total of " << req->length_got << " bytes to queue";
						// Put the messages in the queue for next step
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
