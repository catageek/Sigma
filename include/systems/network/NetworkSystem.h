#ifndef NETWORKSYSTEM_H_INCLUDED
#define NETWORKSYSTEM_H_INCLUDED

#include "Sigma.h"
#include "systems/network/IOPoller.h"
#include "AtomicQueue.hpp"
#include "ThreadPool.h"
#include "systems/network/Protocol.h"
#include "systems/network/AuthenticationHandler.h"

namespace network {
	class TCPConnection;
};

namespace Sigma {
	class IOPoller;
	class GetSaltTaskRequest;

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

		DLL_EXPORT NetworkSystem();
		DLL_EXPORT virtual ~NetworkSystem() {};


        /** \brief Start the server to accept players
         *
         * \param ip const char* the address to listen
         *
         */
		DLL_EXPORT bool Start(const char *ip, unsigned short port);

		static ThreadPool* GetThreadPool() { return &thread_pool; };

		static Authentication& GetAuthenticationComponent() { return authentication; };

		AtomicQueue<std::shared_ptr<Frame_req>>* GetAuthenticatedRawFrameReqQueue() { return &auth_rawframe_req; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetAuthenticatedReassembledFrameQueue() { return &auth_frame_req; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetAuthenticatedCheckedFrameQueue() { return &auth_checked_frame_req; };
		AtomicQueue<std::shared_ptr<Frame_req>>* GetPublicRawFrameReqQueue() { return &pub_rawframe_req; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetPublicReassembledFrameQueue() { return &pub_frame_req; };

	private:
        /** \brief The listener waits for connections and pass new connections to the IncomingConnection
         *
         * \param url const char* the address to listen
         *
         */
		std::unique_ptr<network::TCPConnection> Listener(const char *ip, unsigned short port);

		static IOPoller* Poller() { return &poller; };

		void SetPipeline();

		static void CloseConnection(int fd);

		static int ReassembleFrame(AtomicQueue<std::shared_ptr<Frame_req>>* input, AtomicQueue<std::shared_ptr<FrameObject>>* output, ThreadPool* threadpool);

		int ssocket;											// the listening socket
		static IOPoller poller;
		static Authentication authentication;

		chain_t start;
		chain_t unauthenticated_recv_data;
		chain_t authenticated_recv_data;

		static ThreadPool thread_pool;
		AtomicQueue<std::shared_ptr<Frame_req>> auth_rawframe_req;				// raw frame request, to be authenticated
		AtomicQueue<std::shared_ptr<FrameObject>> auth_frame_req;				// reassembled frame request, to be authenticated
		AtomicQueue<std::shared_ptr<FrameObject>> auth_checked_frame_req;		// reassembled frame request, authenticated
		AtomicQueue<std::shared_ptr<Frame_req>> pub_rawframe_req;				// raw frame requests
		AtomicQueue<std::shared_ptr<FrameObject>> pub_frame_req;				// reassembled frame request
	};
}

#endif // NETWORKSYSTEM_H_INCLUDED
