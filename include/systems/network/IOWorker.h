#ifndef IOWORKER_H_INCLUDED
#define IOWORKER_H_INCLUDED

#include <memory>
#include "netport/include/net/network.h"

namespace Sigma {
	class IOPoller;

	class IOWorker {
	public:
		IOWorker(IOPoller& p) : poller(&p) {};
		virtual ~IOWorker() {};

		IOWorker(IOWorker&& w) : poller(std::move(w.poller)), ssocket(std::move(w.ssocket)) {};

        /** \brief Event Dispatcher Loop
		 *
		 * IOWorkers must return here
         *
         */
		void Work();

        /** \brief Function to begin the Event Dispatcher loop
         *
         * \param server_port std::unique_ptr<network::TCPConnection>&&
         * \return void
         *
         */
		void watch_start(std::unique_ptr<network::TCPConnection>&& server_port);

	private:
		std::unique_ptr<network::TCPConnection> ssocket;	// the listening socket
		IOPoller* const poller;									// the kqueue poller

	};
}

#endif // IOWORKER_H_INCLUDED
