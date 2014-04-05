#include "systems/network/NetworkSystem.h"

#include <chrono>
#include <cstring>
#include <sys/event.h>
#include "netport/include/net/network.h"
#include <typeinfo>
#include "composites/NetworkNode.h"

using namespace network;

namespace Sigma {
	AtomicMap<int,char> NetworkSystem::auth_state_map;						// state of the connections
	AtomicMap<int,char> NetworkSystem::auth_state_client;						// state of the connections

	template<>
	AtomicMap<int,char>* NetworkSystem::GetAuthStateMap<true>() { return &auth_state_client; };

	bool NetworkSystem::Server_Start(const char *ip, unsigned short port) {
//		authentication.GetCryptoEngine()->InitializeDH();

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
		//std::thread t(&IOWorker::watch_start, &this->worker, std::move(s));
		//t.detach();
		ssocket = s->Handle();
		poller.CreatePermanent(ssocket);
		auto tr = std::make_shared<TaskReq<chain_t>>(start);
		ThreadPool::Queue(tr);
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

	bool NetworkSystem::Connect(const char *ip, unsigned short port, const std::string& login) {
		if (! poller.Initialize()) {
			LOG_ERROR << "Could not initialize kqueue !";
			return false;
		}

		auto tr = std::make_shared<TaskReq<chain_t>>(start);
		ThreadPool::Queue(tr);

		LOG_DEBUG << "Connecting...";
		cnx = TCPConnection();
		if (! cnx.Init(NETA_IPv4)) {
			LOG_ERROR << "Could not open socket !";
			return false;
		}

		NetworkAddress address;
		address.IP4(ip);
		address.Port(port);
		if (! cnx.Connect(address)) {
			LOG_ERROR << "Could not connect to server !";
			return false;
		}
		poller.CreatePermanent(cnx.Handle());

		FrameObject packet{};
		std::strncpy(packet.Content<AuthInitPacket, char>(), login.c_str(), LOGIN_FIELD_SIZE - 1);
		packet.SendMessageNoVMAC(cnx.Handle(), NET_MSG, AUTH_INIT);
		SetAuthState(AUTH_INIT);
		std::unique_lock<std::mutex> locker(connecting);
		is_connected.wait_for(locker, std::chrono::seconds(5), [&]() { return (AuthState() == AUTH_SHARE_KEY || AuthState() == AUTH_NONE); });
		if (AuthState() == AUTH_SHARE_KEY) {
			return true;
		}
		CloseClientConnection();
		return false;
	}

	void NetworkSystem::CloseClientConnection() {
		auto fd = cnx.Handle();
		//poller.Delete(fd);
		cnx.Close();
		NetworkSystem::GetAuthStateMap<true>()->Erase(fd);
		SetAuthState(AUTH_NONE);
		is_connected.notify_all();
	}

	void NetworkSystem::CloseConnection(const id_t id, const int fd) {
		poller.Delete(fd);
		TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Close();
		NetworkSystem::GetAuthStateMap()->Erase(fd);
		NetworkNode::RemoveEntity(id);
		VMAC_Checker::RemoveEntity(id);
	}
}
