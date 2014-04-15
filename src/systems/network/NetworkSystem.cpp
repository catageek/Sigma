#include "systems/network/NetworkSystem.h"

#include <chrono>
#include <cstring>
#include <sys/event.h>
#include "netport/include/net/network.h"
#include <typeinfo>
#include "composites/NetworkNode.h"

using namespace network;

namespace Sigma {
	extern template void NetworkSystem::CloseConnection<true>(const FrameObject* frame) const;
	extern template	void NetworkSystem::CloseConnection<false>(const FrameObject* frame) const;

	NetworkSystem::NetworkSystem() throw (std::runtime_error) {
		if (! poller.Initialize()) {
			throw std::runtime_error("Could not initialize kqueue !");
		}
	};

	bool NetworkSystem::Server_Start(const char *ip, unsigned short port) {
//		authentication.GetCryptoEngine()->InitializeDH();

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

	bool NetworkSystem::Connect(const char *ip, unsigned short port, const std::string& login, const std::string& password) {
		auto tr = std::make_shared<TaskReq<chain_t>>(start);
		ThreadPool::Queue(tr);

		authentication.SetPassword(password);

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
		poller.Create(cnx.Handle(), reinterpret_cast<void*>(new ConnectionData(AUTH_INIT)));

		FrameObject packet{};
		std::strncpy(packet.Content<AuthInitPacket, char>(), login.c_str(), LOGIN_FIELD_SIZE - 1);
		packet.SendMessageNoVMAC(cnx.Handle(), NET_MSG, AUTH_INIT);
		SetAuthState(AUTH_INIT);
		std::unique_lock<std::mutex> locker(connecting);
		while (AuthState() != AUTH_SHARE_KEY && AuthState() != AUTH_NONE) {
			is_connected.wait_for(locker, std::chrono::seconds(5), [&]() { return (AuthState() == AUTH_SHARE_KEY || AuthState() == AUTH_NONE); });
		}
		if (AuthState() == AUTH_SHARE_KEY) {
			return true;
		}
		return false;
	}

	void NetworkSystem::RemoveClientConnection() const {
		auto fd = cnx.Handle();
		poller.Delete(fd);
		// TODO : make NetPort thread-safe
//		cnx.Close();
		TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Close();
		SetAuthState(AUTH_NONE);
		is_connected.notify_all();
	}

	void NetworkSystem::RemoveConnection(const int fd) const {
		poller.Delete(fd);
		TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Close();
		SetAuthState(AUTH_NONE);
		is_connected.notify_all();
	}

	void NetworkSystem::CloseConnection(const int fd, const ConnectionData* cx_data) const {
		RemoveConnection(fd);
		NetworkNode::RemoveEntity(cx_data->Id());
		delete cx_data;
	}
}
