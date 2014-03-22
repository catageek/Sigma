#include <cstring>
#include <thread>
#include "systems/network/NetworkClient.h"
#include "systems/network/Authentication.h"
#include "systems/network/Protocol.h"

namespace Sigma {
	void NetworkClient::Start() {
		Authentication::GetCryptoEngine()->InitializeDH();
	}

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
		SendMessage(NET_MSG, AUTH_INIT, reinterpret_cast<char*>(&packet), sizeof(AuthInitPacket));
		return true;
	}

	inline void NetworkClient::SendMessage(unsigned char major, unsigned char minor, char* body, uint32_t len) {
		NetworkSystem::SendMessage(cnx.Handle(), major, minor, body, len);
	}

	std::unique_ptr<MessageObject> NetworkClient::RecvMessage() {
		uint32_t length;
		// Get the length
		auto len = cnx.Recv(reinterpret_cast<char*>(&length), sizeof(uint32_t));
		if (len <= 0) {
			return std::unique_ptr<MessageObject>();
		}
		if (len < sizeof(uint32_t) || length < sizeof(msg_hdr)) {
			LOG_ERROR << "Connection error : received " << len << " bytes as length instead of " << sizeof(uint32_t);
			return std::unique_ptr<MessageObject>();
		}
		length -= sizeof(msg_hdr);
		auto header = std::make_shared<msg_hdr>();
		len = cnx.Recv(reinterpret_cast<char*>(header.get()), sizeof(msg_hdr));
		if (len < sizeof(msg_hdr)) {
			LOG_ERROR << "Connection error : received " << len << " bytes as header instead of " << sizeof(msg_hdr);
			return std::unique_ptr<MessageObject>();
		}
		auto body = std::make_shared<std::vector<unsigned char>>(length);
		len = cnx.Recv(reinterpret_cast<char*>(body->data()), length);
		if (len < length) {
			LOG_ERROR << "Connection error : received " << len << " bytes as body instead of " << length;
			return std::unique_ptr<MessageObject>();
		}
		return std::unique_ptr<MessageObject>(new MessageObject(std::move(header), std::move(body)));
	}

	void NetworkClient::WaitMessage() {
		while(1) {
			LOG_DEBUG << "Waiting message...";
			auto m = std::move(RecvMessage());
			if(m) {
				LOG_DEBUG << "Received message of " << m->body->size() << " bytes";
				auto header = m->header;
				auto major = header->type_major;
				switch(major) {
					case NET_MSG:
					{
						auto minor = header->type_minor;
						switch(minor) {
							case SEND_SALT:
							{
								// We must send DH keys
								auto body = reinterpret_cast<SendSaltPacket*>(m->body->data());
								LOG_DEBUG << "salt received : " << std::string(reinterpret_cast<const char*>(body->salt), 8);
								Authentication::SetSalt(std::unique_ptr<std::vector<unsigned char>>(m->body.get()));
								auto dhkeys = Authentication::GetDHKeysPacket();
								SendMessage(NET_MSG, DH_EXCHANGE, reinterpret_cast<char*>(dhkeys.get()), sizeof(DHKeyExchangePacket));
								LOG_DEBUG << "Sending DH keys : " << sizeof(DHKeyExchangePacket) << " bytes";
								break;
							}
							case DH_EXCHANGE:
								// We received DH Keys
								auto body = reinterpret_cast<DHKeyExchangePacket*>(m->body.get());
								if (Authentication::ComputeSharedSecretKey(*body))
									LOG_DEBUG << "Secure key created";
								break;
						}
					break;
					}
				}
			}
			else {
				LOG_DEBUG << "Closing connection";
				cnx.Close();
				return;
			}
		}
	}
}
