#ifndef NETWORKSYSTEM_H_INCLUDED
#define NETWORKSYSTEM_H_INCLUDED

#include <thread>
#include <condition_variable>
#include <mutex>
#include "Sigma.h"
#include "systems/network/IOPoller.h"
#include "AtomicQueue.hpp"
#include "ThreadPool.h"
#include "systems/network/Protocol.h"
#include "systems/network/AuthenticationHandler.h"
#include "netport/include/net/network.h"
#include "composites/NetworkNode.h"

namespace network {
	class TCPConnection;
};

namespace Sigma {
	class IOPoller;
	class GetSaltTaskRequest;

	using namespace network;

	extern template bool FrameObject::Verify_Authenticity<false>();

	namespace network_packet_handler {
		extern template void INetworkPacketHandler::Process<TEST,TEST, false>();
		extern template void INetworkPacketHandler::Process<TEST,TEST, true>();
	}

	struct Frame_req {
		Frame_req(int fd, vmac_pair* vmac_ptr = nullptr) : reassembled_frame(std::make_shared<FrameObject>(fd, vmac_ptr)),
			length_requested(sizeof(Frame_hdr)), length_got(0) {};
		std::shared_ptr<FrameObject> reassembled_frame;
		uint32_t length_requested;
		uint32_t length_got;
		void (*return_queue)(void);
	};

	namespace network_packet_handler {
		class INetworkPacketHandler;
	}

	class NetworkSystem {
	public:
		friend class network_packet_handler::INetworkPacketHandler;
		friend class Authentication;
		friend class KeyExchangePacket;

		DLL_EXPORT NetworkSystem() {};
		DLL_EXPORT virtual ~NetworkSystem() {};


        /** \brief Start the server to accept players
         *
         * \param ip const char* the address to listen
         *
         */
		DLL_EXPORT bool Start(const char *ip, unsigned short port);
		DLL_EXPORT bool Connect(const char *ip, unsigned short port, const std::string& login);

		DLL_EXPORT void SendMessage(unsigned char major, unsigned char minor, FrameObject& packet);
		DLL_EXPORT void SendUnauthenticatedMessage(unsigned char major, unsigned char minor, FrameObject& packet);

		void SetHasher(std::unique_ptr<cryptography::VMAC_StreamHasher>&& hasher) { this->hasher = std::move(hasher); };
		cryptography::VMAC_StreamHasher* Hasher() { return hasher.get(); };

		void SetAuthState(uint32_t state) { auth_state = state; };
		uint32_t AuthState() const { return auth_state; };

		template<bool isClient = false>
		static AtomicMap<int,char>* GetAuthStateMap() { return &auth_state_map; };

		AtomicQueue<std::shared_ptr<Frame_req>>* GetAuthenticatedRawFrameReqQueue() { return &auth_rawframe_req; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetAuthenticatedReassembledFrameQueue() { return &auth_frame_req; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetAuthenticatedCheckedFrameQueue() { return &auth_checked_frame_req; };
		AtomicQueue<std::shared_ptr<Frame_req>>* GetPublicRawFrameReqQueue() { return &pub_rawframe_req; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetPublicReassembledFrameQueue() { return &pub_frame_req; };

		template<bool isClient>
		DLL_EXPORT void SetTCPHandler() {

			authentication.Initialize<isClient>(this);

			start = chain_t({
				// Poll the socket and select the next chain to run
				[&](){
					std::vector<struct kevent> evList(32);
					LOG_DEBUG << "Polling...";
					auto nev = Poller()->Poll(evList);
					if (nev < 0) {
						LOG_ERROR << "Error when polling event";
					}
					LOG_DEBUG << "got " << nev << " kqueue events";
					for (auto i=0; i<nev; i++) {
						auto fd = evList[i].ident;
						if (evList[i].flags & EV_EOF) {
							LOG_DEBUG << "disconnect " << fd;
							CloseConnection(reinterpret_cast<vmac_pair*>(evList[i].udata)->first, fd);
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
							if (! NetworkSystem::GetAuthStateMap<isClient>()->Count(fd) || NetworkSystem::GetAuthStateMap<isClient>()->At(fd) != AUTH_SHARE_KEY) {
								// Data received, not authenticated
								// Request the frame header
								// queue the task to reassemble the frame
								LOG_DEBUG << "got unauthenticated frame";
								NetworkSystem::GetPublicRawFrameReqQueue()->Push(std::make_shared<Frame_req>(fd));
								ThreadPool::Queue(std::make_shared<TaskReq<chain_t>>(unauthenticated_recv_data));
							}
							else {
								// We stop watching the connection until we got all the frame
								LOG_DEBUG << "got authenticated frame";
								// Data received from authenticated client
								NetworkSystem::GetAuthenticatedRawFrameReqQueue()->Push(std::make_shared<Frame_req>(fd, reinterpret_cast<vmac_pair*>(evList[i].udata)));
								ThreadPool::Queue(std::make_shared<TaskReq<chain_t>>(authenticated_recv_data));
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
				std::bind(&NetworkSystem::ReassembleFrame, this, &pub_rawframe_req, &pub_frame_req),
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
							CloseConnection(req->GetId(), req->fd);
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
											network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_INIT, isClient>();
											break;
										}
									case AUTH_SEND_SALT:
										{
											network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_SEND_SALT>()->Push(req);
											network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_SEND_SALT, isClient>();
											break;
										}
									case AUTH_KEY_EXCHANGE:
										{
											network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_KEY_EXCHANGE>()->Push(req);
											network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_EXCHANGE, isClient>();
											break;
										}
									case AUTH_KEY_REPLY:
										{
											network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_KEY_REPLY>()->Push(req);
											network_packet_handler::INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_REPLY, isClient>();
											break;
										}
									default:
										{
											LOG_DEBUG << "invalid minor code, closing";
											CloseConnection(req->GetId(), req->fd);
										}
								}
								break;
							}
						default:
							{
								LOG_DEBUG << "invalid major code, closing";
								CloseConnection(req->GetId(), req->fd);
							}
						}
						poller.Watch<isClient>(req->fd);
					}
					return STOP;
				}
			});

			authenticated_recv_data = chain_t({
				// Get the length
				std::bind(&NetworkSystem::ReassembleFrame, this, &auth_rawframe_req, &auth_frame_req),
				[&](){
					std::shared_ptr<FrameObject> req;
					if(! NetworkSystem::GetAuthenticatedReassembledFrameQueue()->Pop(req)) {
						return STOP;
					}
					if(req->Verify_Authenticity<isClient>()) {
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
											network_packet_handler::INetworkPacketHandler::Process<TEST,TEST, isClient>();
											break;
										}
									default:
										{
											LOG_ERROR << "TEST: closing";
											CloseConnection(req->GetId(), req->fd);
										}
								}
								break;
							}

						default:
							{
								CloseConnection(req->GetId(), req->fd);
							}
						}
						poller.Watch<isClient>(req->fd);
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

		IOPoller* Poller() { return &poller; };

		void CloseClientConnection();
		void CloseConnection(id_t id, int fd);

		int ReassembleFrame(AtomicQueue<std::shared_ptr<Frame_req>>* input, AtomicQueue<std::shared_ptr<FrameObject>>* output);

		int ssocket;											// the listening socket
		IOPoller poller;
		Authentication authentication;
		static AtomicMap<int,char> auth_state_map;						// state of the connections

		chain_t start;
		chain_t unauthenticated_recv_data;
		chain_t authenticated_recv_data;

		static ThreadPool thread_pool;
		AtomicQueue<std::shared_ptr<Frame_req>> auth_rawframe_req;				// raw frame request, to be authenticated
		AtomicQueue<std::shared_ptr<FrameObject>> auth_frame_req;				// reassembled frame request, to be authenticated
		AtomicQueue<std::shared_ptr<FrameObject>> auth_checked_frame_req;		// reassembled frame request, authenticated
		AtomicQueue<std::shared_ptr<Frame_req>> pub_rawframe_req;				// raw frame requests
		AtomicQueue<std::shared_ptr<FrameObject>> pub_frame_req;				// reassembled frame request

		// members for client only
		static AtomicMap<int,char> auth_state_client;						// state of the connections
		std::unique_ptr<cryptography::VMAC_StreamHasher> hasher;
		uint32_t auth_state;
		network::TCPConnection cnx;
		std::condition_variable is_connected;
		std::mutex connecting;
	};
}

#endif // NETWORKSYSTEM_H_INCLUDED
