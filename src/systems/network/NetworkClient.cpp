#include <cstring>
#include <thread>
#include "systems/network/NetworkClient.h"
#include "systems/network/Authentication.h"
#include "systems/network/Protocol.h"

namespace Sigma {
	bool NetworkClient::Connect(const char *ip, unsigned short port) {
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
		std::thread wait_msg(&NetworkClient::WaitMessage, this);
		wait_msg.detach();

		// TODO
		char login[] = "my_login";
		AuthInitPacket packet;
		std::strncpy(packet.login, login, LOGIN_FIELD_SIZE - 1);
		SendMessage(NET_MSG, AUTH_REQUEST, packet.login, 16);
		return true;
	}

	void NetworkClient::SendMessage(unsigned char major, unsigned char minor, char* body, size_t len) {
		msg_hdr header;
		header.type_major = major;
		header.type_minor = minor;
		header.length = len;
		cnx.Send(reinterpret_cast<char*>(&header), sizeof(msg_hdr));
		cnx.Send(body, len);
	}

	std::unique_ptr<Message> NetworkClient::RecvMessage() {
		auto header = std::make_shared<msg_hdr>();
		// Get the header
		auto len = cnx.Recv(reinterpret_cast<char*>(header.get()), sizeof(msg_hdr));
		if (len < sizeof(msg_hdr)) {
			LOG_ERROR << "Connection error : received " << len << " bytes as header instead of " << sizeof(msg_hdr);
			return std::unique_ptr<Message>();
		}
		auto body_size = header->length;
		auto body = std::make_shared<std::vector<char>>(body_size);
		len = cnx.Recv(body->data(), body_size);
		if (len < body_size) {
			LOG_ERROR << "Connection error : received " << len << " bytes as header instead of " << body_size;
			return std::unique_ptr<Message>();
		}
		return std::unique_ptr<Message>(new Message(std::move(header), std::move(body)));
	}

	void NetworkClient::WaitMessage() {
		while(1) {
			LOG_DEBUG << "Waiting message...";
			auto m = std::move(RecvMessage());
			if(m) {
				LOG_DEBUG << "Received message of " << m->body->size() << " bytes";
			}
			else {
				LOG_ERROR << "Received null pointer";
			}
		}
	}
}
