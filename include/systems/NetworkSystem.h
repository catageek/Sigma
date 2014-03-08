#ifndef NETWORKSYSTEM_H_INCLUDED
#define NETWORKSYSTEM_H_INCLUDED

#include "Sigma.h"
#include "systems/IOWorker.h"
#include "systems/IOPoller.h"

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
	};
}

#endif // NETWORKSYSTEM_H_INCLUDED
