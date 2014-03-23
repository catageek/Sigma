#ifndef NETWORKSYSTEM_H_INCLUDED
#define NETWORKSYSTEM_H_INCLUDED

#include "Sigma.h"
#include "systems/network/IOPoller.h"
#include "systems/network/AtomicQueue.hpp"
#include "systems/network/AtomicMap.hpp"
#include "systems/network/ThreadPool.h"
#include "systems/network/Protocol.h"

namespace network {
	class TCPConnection;
};

namespace Sigma {
	class IOPoller;
	class GetSaltTaskRequest;

	struct Frame_req {
		Frame_req(int fd) : reassembled_frame(std::make_shared<FrameObject>(fd)),
			length_requested(sizeof(Frame_hdr)), length_got(0) {};
		std::shared_ptr<FrameObject> reassembled_frame;
		uint32_t length_requested;
		uint32_t length_got;
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

		ThreadPool* GetThreadPool() { return &thread_pool; };

		AtomicMap<int,char>* GetAuthStateMap() { return &auth_state; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetAuthRequestQueue() { return &authentication_req; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetSaltRequestQueue() { return &salt_req; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetSaltRetrievedQueue() { return &salt_ret; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetKeyReceivedQueue() { return &key_received; };
		AtomicQueue<int>* GetDataRecvQueue() { return &data_received; };
		AtomicQueue<std::shared_ptr<Frame_req>>* GetPublicRawFrameReqQueue() { return &pub_rawframe_req; };
		AtomicQueue<std::shared_ptr<FrameObject>>* GetPublicReassembledFrameQueue() { return &pub_frame_req; };

	private:
        /** \brief The listener waits for connections and pass new connections to the IncomingConnection
         *
         * \param url const char* the address to listen
         *
         */
		std::unique_ptr<network::TCPConnection> Listener(const char *ip, unsigned short port);

		void SetPipeline();

		void CloseConnection(int fd);

		static int ReassembleFrame(AtomicQueue<std::shared_ptr<Frame_req>>* input, AtomicQueue<std::shared_ptr<FrameObject>>* output, ThreadPool* threadpool, const chain_t* rerun, size_t index);
		int RetrieveSalt(std::shared_ptr<FrameObject> frame, AtomicQueue<std::shared_ptr<FrameObject>>* output);

		int ssocket;											// the listening socket
		IOPoller poller;

		chain_t start;
		chain_t unauthenticated_recv_data;
		chain_t request_authentication;
		chain_t shared_secret_key;

		ThreadPool thread_pool;
		AtomicMap<int,char> auth_state;													// stateful connections, i.e packets are to be routed to a specific chain
		AtomicQueue<std::shared_ptr<FrameObject>> authentication_req;	// Authentication request
		AtomicQueue<std::shared_ptr<FrameObject>> salt_req;				// salt request
		AtomicQueue<std::shared_ptr<FrameObject>> salt_ret;				// salt retrieved
		AtomicQueue<std::shared_ptr<FrameObject>> key_received;											// Received DH key
		AtomicQueue<int> data_received;			// Data received, authenticated
		AtomicQueue<std::shared_ptr<Frame_req>> pub_rawframe_req;		// raw frame requests
		AtomicQueue<std::shared_ptr<FrameObject>> pub_frame_req;			// reassembled frame request
	};
}

#endif // NETWORKSYSTEM_H_INCLUDED
