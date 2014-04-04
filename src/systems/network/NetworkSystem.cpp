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

	bool NetworkSystem::Start(const char *ip, unsigned short port) {
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
		SendUnauthenticatedMessage(NET_MSG, AUTH_INIT, packet);
		SetAuthState(AUTH_INIT);
		std::unique_lock<std::mutex> locker(connecting);
		is_connected.wait_for(locker, std::chrono::seconds(5), [&]() { return (AuthState() == AUTH_SHARE_KEY || AuthState() == AUTH_NONE); });
		if (AuthState() == AUTH_SHARE_KEY) {
			return true;
		}
		CloseClientConnection();
		return false;
	}

	void NetworkSystem::SendMessage(unsigned char major, unsigned char minor, FrameObject& packet) {
		packet.SendMessage(cnx.Handle(), major, minor, Hasher());
	}

	inline void NetworkSystem::SendUnauthenticatedMessage(unsigned char major, unsigned char minor, FrameObject& packet) {
		packet.SendMessageNoVMAC(cnx.Handle(), major, minor);
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


	int NetworkSystem::ReassembleFrame(AtomicQueue<std::shared_ptr<Frame_req>>* input, AtomicQueue<std::shared_ptr<FrameObject>>* output) {
		std::shared_ptr<Frame_req> req;
		if(! input->Pop(req)) {
			return STOP;
		}
		auto target_size = req->length_requested;
		auto frame = req->reassembled_frame;
		char* buffer = reinterpret_cast<char*>(frame->FullFrame());

		auto current_size = req->length_got;

		auto len = TCPConnection(frame->fd, NETA_IPv4, SCS_CONNECTED).Recv(reinterpret_cast<char*>(buffer) + current_size, target_size - current_size);
		current_size += len;
		req->length_got = current_size;

		if(current_size == sizeof(Frame_hdr) && target_size == sizeof(Frame_hdr)) {
			// We now have the length
			auto length = frame->FullFrame()->length;
			target_size = frame->FullFrame()->length + sizeof(Frame_hdr);
			req->length_requested = target_size;
			frame->Resize<false>(length - sizeof(msg_hdr));
			buffer = reinterpret_cast<char*>(frame->FullFrame());
			auto len = TCPConnection(frame->fd, NETA_IPv4, SCS_CONNECTED).Recv(reinterpret_cast<char*>(buffer) + current_size, target_size - current_size);
			current_size += len;
			req->length_got = current_size;
		}

		if (current_size < target_size) {
			// we put again the request in the queue
			LOG_DEBUG << "got only " << current_size << " bytes, waiting " << target_size << " bytes";
			input->Push(req);
			return REPEAT;
		}
		else {
			LOG_DEBUG << "Packet of " << current_size << " put in dispatch queue.";
			output->Push(frame);
		}
		return CONTINUE;
	}
}
