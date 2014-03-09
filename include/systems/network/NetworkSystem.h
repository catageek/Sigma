#ifndef NETWORKSYSTEM_H_INCLUDED
#define NETWORKSYSTEM_H_INCLUDED

#include "Sigma.h"
#include "systems/network/IOWorker.h"
#include "systems/network/IOPoller.h"
#include "systems/network/AtomicQueue.hpp"

namespace network {
	class TCPConnection;
};

namespace Sigma {
	class IOPoller;
	class IOWorker;

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

		static AtomicQueue<int>* GetPendingQueue() { return &pending; };
		static AtomicQueue<int>* GetAuthRequestQueue() { return &authentication_req; };
		static AtomicQueue<int>* GetDataRecvQueue() { return &data_received; };

	private:
        /** \brief The listener waits for connections and pass new connections to the IncomingConnection
         *
         * \param url const char* the address to listen
         *
         */
		std::unique_ptr<network::TCPConnection> Listener(const char *ip, unsigned short port);

        /** \brief Receive incoming connections and check that they are valid (i.e there is an identified player behind)
         *
         * This function is run as a thread
         *
         */
		void ConnectionHandler(network::TCPConnection client);

		IOPoller poller;
		IOWorker worker;

		static AtomicQueue<int> pending;			// Connections accepted, no data received
		static AtomicQueue<int> authentication_req;	// Data received, not authenticated
		static AtomicQueue<int> data_received;		// Data received, authenticated
	};
}

#endif // NETWORKSYSTEM_H_INCLUDED
