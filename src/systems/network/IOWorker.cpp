#include <vector>
#include "systems/network/IOWorker.h"
#include "Log.h"
#include "sys/event.h"
#include "systems/network/IOPoller.h"
#include "systems/network/AuthWorker.h"

using namespace network;

namespace Sigma {
	void IOWorker::watch_start(std::unique_ptr<TCPConnection>&& server_port) {
		poller->Watch(server_port->Handle());
		ssocket = std::move(server_port);
		watch();
	}

	void IOWorker::watch() {
		std::vector<struct kevent> evList(32);
		while (1) {
			auto nev = poller->Poll(evList, nullptr);
			if (nev < 1) {
				LOG_ERROR << "Error when polling event";
			}
			LOG_DEBUG << "got " << nev << " events";
			for (auto i=0; i<nev; i++) {
				if (evList[i].flags & EV_EOF) {
					auto fd = evList[i].ident;
					LOG_DEBUG << "disconnect " << fd;
					poller->Unwatch(fd);
					TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Close();
					continue;
				}
				if (evList[i].flags & EV_ERROR) {   /* report any error */
					LOG_ERROR << "EV_ERROR: " << evList[i].data;
					continue;
				}

				if (evList[i].ident == ssocket->Handle()) {
					TCPConnection c;
					if (! ssocket->Accept(&c)) {
						LOG_ERROR << "Could not accept connection, handle is " << ssocket->Handle();
						continue;
					}
					poller->Watch(c.Handle());
					LOG_DEBUG << "connect " << c.Handle();
					c.Send("welcome!\n");
				}
				else if (evList[i].flags & EVFILT_READ) {
					AuthenticationPacket packet;
					auto len = TCPConnection(evList[i].ident, NETA_IPv4, SCS_CONNECTED).Recv(packet.buffer, 32);
					LOG_DEBUG << "received " << len << " bytes";
					auto rep = AuthWorker().Authenticate(&packet);
					LOG_DEBUG << "authenticator reply: " << rep;
				}
			}
		}
	}
}
