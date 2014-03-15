#ifndef NETWORKSYSTEM_H_INCLUDED
#define NETWORKSYSTEM_H_INCLUDED

#include "Sigma.h"
#include "systems/network/IOPoller.h"
#include "systems/network/AtomicQueue.hpp"
#include "systems/network/AtomicSet.hpp"
#include "systems/network/ThreadPool.h"
#include "systems/network/Protocol.h"

namespace network {
	class TCPConnection;
};

namespace Sigma {
	class IOPoller;
	class AuthenticationPacket;

	struct Frame_req {
		Frame_req(int fd, uint32_t length) : reassembled_frame(std::make_shared<Frame>(fd)), length_requested(length) {};
		std::shared_ptr<Frame> reassembled_frame;
		uint32_t length_requested;
		void (*return_queue)(void);
	};


	class NetworkSystem {
	public:
		DLL_EXPORT NetworkSystem();
		DLL_EXPORT virtual ~NetworkSystem() {};


        /** \brief Start the server to accept players
         *
         * \param ip const char* the address to listen
         *
         */
		DLL_EXPORT bool Start(const char *ip, unsigned short port);

		static AtomicQueue<block_t*>* GetNextTargetChain() { return &next_chain; };

		static ThreadPool* GetThreadPool() { return &thread_pool; };

		static AtomicSet<int>* GetPendingSet() { return &pending; };
		static AtomicQueue<int>* GetPublicReqQueue() { return &public_req; };
		static AtomicQueue<int>* GetAuthRequestQueue() { return &authentication_req; };
		static AtomicQueue<int>* GetDataRecvQueue() { return &data_received; };
		static AtomicQueue<std::shared_ptr<Frame_req>>* GetPublicRawFrameReqQueue() { return &pub_rawframe_req; };
		static AtomicQueue<std::shared_ptr<Frame>>* GetPublicReassembledFrameQueue() { return &pub_frame_req; };

	private:
        /** \brief The listener waits for connections and pass new connections to the IncomingConnection
         *
         * \param url const char* the address to listen
         *
         */
		std::unique_ptr<network::TCPConnection> Listener(const char *ip, unsigned short port);

		void SetPipeline();

		bool Authenticate(const AuthenticationPacket* packet);
		void CloseConnection(int fd);

		int ssocket;											// the listening socket
		IOPoller poller;

		chain_t start;
		chain_t unauthenticated_reassemble;

		static AtomicQueue<block_t*> next_chain;

		static ThreadPool thread_pool;
		static AtomicSet<int> pending;				// Connections accepted, no data received
		static AtomicQueue<int> public_req;				// Data received, not authenticated
		static AtomicQueue<int> authentication_req;		// Authentication request
		static AtomicQueue<int> data_received;			// Data received, authenticated
		static AtomicQueue<std::shared_ptr<Frame_req>> pub_rawframe_req;		// raw frame requests
		static AtomicQueue<std::shared_ptr<Frame>> pub_frame_req;			// reassembled frame request
	};
}

#endif // NETWORKSYSTEM_H_INCLUDED
