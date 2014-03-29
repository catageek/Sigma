#include <cstring>
#include <thread>
#include "systems/network/NetworkClient.h"
#include "systems/network/NetworkSystem.h"
#include "systems/network/AuthenticationHandler.h"
#include "systems/network/Protocol.h"

namespace Sigma {
	void NetworkClient::Start() {
		NetworkSystem::GetAuthenticationComponent().GetCryptoEngine()->InitializeDH();
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

		// TODO
		char login[] = "my_login";
		auto packet = FrameObject();
		std::strncpy(packet.Content<AuthInitPacket, char>(), login, LOGIN_FIELD_SIZE - 1);
		SendMessage(NET_MSG, AUTH_INIT, packet);
		auth_state = AUTH_INIT;

		std::thread wait_msg(&NetworkClient::Authenticate, this);
		wait_msg.join();

		return (auth_state == AUTH_SHARE_KEY);
	}

	inline void NetworkClient::SendMessage(unsigned char major, unsigned char minor, FrameObject& packet) {
		packet.SendMessage(cnx.Handle(), major, minor);
	}

	std::unique_ptr<FrameObject> NetworkClient::RecvMessage() {
		auto frame = std::unique_ptr<FrameObject>(new FrameObject());
		// Get the length
		auto len = cnx.Recv(reinterpret_cast<char*>(&frame->FullFrame()->length), sizeof(uint32_t));
		if (len <= 0) {
			return std::unique_ptr<FrameObject>();
		}
		auto length = frame->FullFrame()->length;
		if (len < sizeof(uint32_t) || length < sizeof(msg_hdr)) {
			LOG_ERROR << "Connection error : received " << len << " bytes as length instead of " << sizeof(uint32_t);
			return std::unique_ptr<FrameObject>();
		}
		frame->Resize(length - sizeof(msg_hdr));
		auto header = frame->Header();
		len = cnx.Recv(reinterpret_cast<char*>(header), length);
		if (len < length) {
			LOG_ERROR << "Connection error : received " << len << " bytes as header instead of " << length;
			return std::unique_ptr<FrameObject>();
		}
		return std::move(frame);
	}

	void NetworkClient::Authenticate() {
		while(auth_state != AUTH_SHARE_KEY) {
			LOG_DEBUG << "Waiting message...";
			auto m = std::move(RecvMessage());
			if(m) {
				LOG_DEBUG << "Received message of " << m->PacketSize() << " bytes";
				auto header = m->Header();
				auto major = header->type_major;
				switch(major) {
					case NET_MSG:
					{
						auto minor = header->type_minor;
						switch(minor) {
							case AUTH_SEND_SALT:
							{
								// We must send keys
								auto body = m->Content<SendSaltPacket>();
								LOG_DEBUG << "salt received : " << std::string(reinterpret_cast<const char*>(body->salt), 8);
//								Authentication::SetSalt(std::unique_ptr<std::vector<unsigned char>>(m->body.get()));
								auto frame = body->GetKeyExchangePacket();
								auto packet = frame->Content<KeyExchangePacket>();
								packet->VMAC_BuildHasher();
								SendMessage(NET_MSG, AUTH_KEY_EXCHANGE, *frame);
								LOG_DEBUG << "Sending keys : " << frame->PacketSize() << " bytes";
								auth_state = AUTH_KEY_EXCHANGE;
								break;
							}
							case AUTH_KEY_REPLY:
								// We received reply
								auto body = m->Content<KeyReplyPacket>();
								if (body->VerifyVMAC()) {
									LOG_DEBUG << "Secure key created";
									auth_state = AUTH_SHARE_KEY;
								}
								else {
									LOG_DEBUG << "VMAC failed";
									auth_state = AUTH_NONE;
								}
								break;
						}
					break;
					}
				}
			}
			else {
				LOG_DEBUG << "Closing connection";
				auth_state = AUTH_NONE;
				cnx.Close();
				return;
			}
		}
	}
}
