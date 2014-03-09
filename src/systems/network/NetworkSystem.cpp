#include "systems/network/NetworkSystem.h"

#include <thread>
#include <chrono>
#include <sys/event.h>
#include "netport/include/net/network.h"
// Hom many seconds to wait the first bytes
#define WAIT_DATA 3

using namespace network;

namespace Sigma {
	NetworkSystem::NetworkSystem() : poller(), worker(IOWorker(poller)) {};

	AtomicQueue<int> NetworkSystem::pending;
	AtomicQueue<int> NetworkSystem::authentication_req;	// Data received, not authenticated
	AtomicQueue<int> NetworkSystem::data_received;		// Data received, authenticated

	bool NetworkSystem::Start(const char *ip, unsigned short port) {
		if (! poller.Initialize()) {
			LOG_ERROR << "Could not initialize kqueue !";
			return false;
		}
		// start to listen
		auto s = Listener(ip, port);
		if (! s) {
			LOG_ERROR << "Failed to start NetworkSystem.";
			return false;
		}
		// run the IOWorker thread
		std::thread t(&IOWorker::watch_start, &this->worker, std::move(s));
		t.detach();
		return true;
	}

	std::unique_ptr<TCPConnection> NetworkSystem::Listener(const char *ip, unsigned short port) {
		auto server = std::unique_ptr<TCPConnection>(new TCPConnection());
		if (! server->Init(NETA_IPv4)) {
			LOG_ERROR << "Could not open socket !";
			return std::unique_ptr<TCPConnection>();
		}

		NetworkAddress address;
		address.IP4(ip);
		address.Port(port);
		if (! server->Bind(address)) {
			LOG_ERROR << "Could not bind address !";
			return std::unique_ptr<TCPConnection>();
		}

		if (! server->Listen(5)) {
			LOG_ERROR << "Could not listen on port !";
			return std::unique_ptr<TCPConnection>();
		}
		return std::move(server);
	}

	void NetworkSystem::ConnectionHandler(TCPConnection client) {
		client.SetNonBlocking(true);
		LOG_DEBUG << "Handling connection from " << client.GetRemote().ToString();
		std::string s, auth_data;
		int i;
		char len = 0;
		// We get a byte giving the length of the string to wait, in a limited time
		for (auto d = 0; d < WAIT_DATA; ++d, std::this_thread::sleep_for(std::chrono::seconds(1))) {
			i = client.Recv(s, 10);
			if (i == 0) { continue; }
			if (i > 0) {
				// we got data
				for (auto it = s.cbegin(); it != s.cend(); ++it) {
					if (len == 0) {
						len = *it;
						if (len <= 0 || len > 32) {
							// we drop the connection
							client.Close();
							return;
						}
						continue;
					}
					auth_data += *it;
				}
				if (auth_data.size() == len) {
					break;
				}
			}
		}
		if (len == 0 || auth_data.size() != len) {
			// We drop the connection after the timeout
			client.Close();
			return;
		}
		LOG_DEBUG << "Got data : " << auth_data;
		// TODO: authenticate user
		// TODO: get a dynamic ID
		// hardcoded ID for the moment
		id_t id = 1;

		int kq;
		struct kevent evSet;

		kq = kqueue();

		EV_SET(&evSet, client.Handle(), EVFILT_READ, EV_ADD, 0, 0, NULL);
		if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1)
			LOG_ERROR << "kevent error";
	}
}
