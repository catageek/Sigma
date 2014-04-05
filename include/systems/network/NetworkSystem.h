#ifndef NETWORKSYSTEM_H_INCLUDED
#define NETWORKSYSTEM_H_INCLUDED

#include <thread>
#include <condition_variable>
#include <mutex>
#include "Sigma.h"
#include "systems/network/IOPoller.h"
#include "AtomicQueue.hpp"
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
		Frame_req(int fd, size_t length_total, vmac_pair* vmac_ptr = nullptr) : reassembled_frame(std::make_shared<FrameObject>(fd, vmac_ptr)),
			length_total(length_total), length_requested(sizeof(Frame_hdr)), length_got(0) {};

		// delete copy constructor and assignment for zero-copy guarantee
		Frame_req(Frame_req&) = delete;
		Frame_req& operator=(Frame_req&) = delete;

		std::shared_ptr<FrameObject> reassembled_frame;
		uint32_t length_requested;
		uint32_t length_got;
		size_t length_total;
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
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					std::vector<struct kevent> evList(32);
					LOG_DEBUG << "Polling...";
					auto nev = Poller()->Poll(evList);
					if (nev < 0) {
						LOG_ERROR << "Error when polling event";
					}
					LOG_DEBUG << "got " << nev << " kqueue events";
					bool a = false, b = false;
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
								NetworkSystem::GetPublicRawFrameReqQueue()->Push(std::make_shared<Frame_req>(fd, evList[i].data));
								a = true;
							}
							else {
								// We stop watching the connection until we got all the frame
								LOG_DEBUG << "got authenticated frame";
								// Data received from authenticated client
								NetworkSystem::GetAuthenticatedRawFrameReqQueue()->Push(std::make_shared<Frame_req>(fd, evList[i].data, reinterpret_cast<vmac_pair*>(evList[i].udata)));
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
						return REPEAT;
					}
					return STOP;
				}
			});

			unauthenticated_recv_data = chain_t({
				// Get the length
				std::bind(&NetworkSystem::ReassembleFrame<isClient>, this, &pub_rawframe_req, &pub_frame_req),
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
				std::bind(&NetworkSystem::ReassembleFrame<isClient>, this, &auth_rawframe_req, &auth_frame_req),
				[&](){
					auto req_list = NetworkSystem::GetAuthenticatedReassembledFrameQueue()->Poll();
					if (! req_list) {
						return STOP;
					}
					for(std::shared_ptr<FrameObject>& req : *req_list) {
						if(req->Verify_Authenticity<isClient>()) {
							NetworkSystem::GetAuthenticatedCheckedFrameQueue()->Push(std::move(req));
						}
						else {
							LOG_DEBUG << "Dropping 1 frame (VMAC check failed)";
						}
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
											network_packet_handler::INetworkPacketHandler::GetQueue<TEST,TEST>()->Push(std::move(req));
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

		int ssocket;											// the listening socket
		IOPoller poller;
		Authentication authentication;
		static AtomicMap<int,char> auth_state_map;						// state of the connections

		chain_t start;
		chain_t unauthenticated_recv_data;
		chain_t authenticated_recv_data;

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

		template<bool isClient>
		int ReassembleFrame(AtomicQueue<std::shared_ptr<Frame_req>>* input, AtomicQueue<std::shared_ptr<FrameObject>>* output) {
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
				input->Push(std::move(req));
				return REPEAT;
			}
			else {
				LOG_DEBUG << "Packet of " << current_size << " put in dispatch queue.";
				output->Push(std::move(frame));
				if( current_size < req->length_total ) {
					req->length_total -= current_size;
					LOG_DEBUG << "Get another packet from fd #" << req->reassembled_frame->fd << " frome same frame.";
					input->Push(std::make_shared<Frame_req>(req->reassembled_frame->fd, req->length_total, req->reassembled_frame->GetVMACVerifier()));
					return REPEAT;
				}
				else {
					poller.Watch<isClient>(req->reassembled_frame->fd);
				}
			}
			return CONTINUE;
		}
	};
}

#endif // NETWORKSYSTEM_H_INCLUDED
