#include <vector>
#include "systems/network/IOWorker.h"
#include "Log.h"
#include "sys/event.h"
#include "systems/network/IOPoller.h"
#include "systems/network/AuthWorker.h"
#include "systems/network/NetworkSystem.h"

using namespace network;

namespace Sigma {
	void IOWorker::watch_start(std::unique_ptr<TCPConnection>&& server_port) {
		poller->Watch(server_port->Handle());
		ssocket = std::move(server_port);
		Work();
	}

	void IOWorker::Work() {
		std::vector<struct kevent> evList(32);
		while (1) {
			auto nev = poller->Poll(evList, nullptr);
			if (nev < 1) {
				LOG_ERROR << "Error when polling event";
			}
			LOG_DEBUG << "got " << nev << " events";
			for (auto i=0; i<nev; i++) {
				auto fd = evList[i].ident;
				if (evList[i].flags & EV_EOF) {
					LOG_DEBUG << "disconnect " << fd;
					poller->Unwatch(fd);
					TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Close();
					continue;
				}
				if (evList[i].flags & EV_ERROR) {   /* report any error */
					LOG_ERROR << "EV_ERROR: " << evList[i].data;
					continue;
				}

				if (fd == ssocket->Handle()) {
					TCPConnection c;
					if (! ssocket->Accept(&c)) {
						LOG_ERROR << "Could not accept connection, handle is " << ssocket->Handle();
						continue;
					}
					poller->Watch(c.Handle());
					LOG_DEBUG << "connect " << c.Handle();
					c.Send("welcome!\n");
					NetworkSystem::GetPendingQueue()->Add(c.Handle());
				}
				else if (evList[i].flags & EVFILT_READ) {
					// Data received
					if (NetworkSystem::GetPendingQueue()->Exist(fd)) {
						// Data received, not authenticated
						NetworkSystem::GetAuthRequestQueue()->Add(fd);
					}
					else {
						// Data received from authenticated client
						NetworkSystem::GetDataRecvQueue()->Add(fd);
					}
				}
				AuthWorker().Work();
			}
		}
	}
}
