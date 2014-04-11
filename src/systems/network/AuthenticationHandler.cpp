#include "systems/network/AuthenticationHandler.h"
#include "systems/network/NetworkSystem.h"
#include <cstring>
#include "composites/VMAC_Checker.h"
#include "composites/NetworkNode.h"
#include "crypto++/vmac.h"

namespace Sigma {
	NetworkSystem* Authentication::netsystem = nullptr;
	NetworkSystem* Authentication::netsystem_client = nullptr;
	std::shared_ptr<chain_t> Authentication::auth_init_handler;
	std::string Authentication::password;
	std::vector<byte> Authentication::secret_key(16);


	template<>
	NetworkSystem* const Authentication::GetNetworkSystem<true>() { return netsystem_client; };

	template<>
	void Authentication::SetSystem<true>(NetworkSystem* system) { netsystem_client = system; };

	void Authentication::SetHasher(std::unique_ptr<cryptography::VMAC_StreamHasher>&& hasher) {
		GetNetworkSystem()->SetHasher(std::move(hasher));
	}

	void Authentication::SetAuthState(uint32_t state) { GetNetworkSystem()->SetAuthState(state); }


	int Authentication::RetrieveSalt() const {
		auto req_list = network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_INIT>()->Poll();
		if (!req_list) {
			return STOP;
		}
		for(auto& req : *req_list) {
			auto reply = std::make_shared<FrameObject>(req->fd);
			reply->Resize<false>(sizeof(SendSaltPacket));
			// TODO : salt is hardcoded !!!
			auto login = req->Content<AuthInitPacket>()->login;
			// GetSaltFromsomewhere(reply->Body(), packet->login);
			std::memcpy(reply->Content<SendSaltPacket,char>(), std::string("abcdefgh").data(), 8);
			LOG_DEBUG << "(" << sched_getcpu() << ") Queing salt to send";
			salt_retrieved.Push(std::move(reply));
		}
		return CONTINUE;
	}


	int Authentication::SendSalt() const {
		auto req_list = salt_retrieved.Poll();
		if (!req_list) {
			return STOP;
		}
		for(auto& req : *req_list) {
			auto reply = req->FullFrame();
			if (reply) {
				NetworkSystem::GetAuthStateMap()->Insert(req->fd, AUTH_KEY_EXCHANGE);
				// send salt
				req->SendMessageNoVMAC(req->fd, NET_MSG, AUTH_SEND_SALT);
			}
			else {
				// close connection
				// TODO: send error message
				//TCPConnection(req->fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(challenge.get()), sizeof(*challenge));
				LOG_DEBUG << "User unknown";
				netsystem->CloseConnection(req->GetId(), req->fd);
			}
		}
		return CONTINUE;
	}

	void Authentication::CheckKeyExchange(const std::list<std::shared_ptr<FrameObject>>& req_list) {
		// server side
		for(auto& req : req_list) {
			if(! NetworkSystem::GetAuthStateMap()->Compare(req->fd, AUTH_KEY_EXCHANGE)) {
				// This is not the packet expected
				LOG_DEBUG << "(" << sched_getcpu() << ") Unexpected packet received with length = " << req->FullFrame()->length;
				Authentication::GetNetworkSystem()->CloseConnection(req->GetId(), req->fd);
				continue;
			}
			// Derive password and salt to get key
			// TODO : key should be stored and not be computed at runtime. The password is not on the server !
			// TODO : remove salt from KeyExchangePacket
			std::vector<byte> vkey(16);
			auto key = vkey.data();
			std::string password("secret password");
			auto recv_packet = req->Content<KeyExchangePacket>();
			Crypto::PBKDF(key, reinterpret_cast<const byte*>(password.data()), password.size(), recv_packet->salt, SALT_SIZE);
			if(! recv_packet->VerifyVMAC(key)) {
				return;
			}
			auto checker_key = recv_packet->VMAC_BuildHasher(key);
			if(! checker_key) {
				return;
			}

			auto hasher = VMAC_Checker::AddEntity(1, std::move(checker_key), recv_packet->nonce2, 8);
			auto reply_packet = std::make_shared<FrameObject>();
			reply_packet->Content<KeyReplyPacket>()->Compute(recv_packet->nonce2);

			if(reply_packet) {
				LOG_DEBUG << "(" << sched_getcpu() << ") Authentication OK";
				NetworkSystem::GetAuthStateMap()->Insert(req->fd, AUTH_SHARE_KEY);
				// TODO: Hardcoded entity
				NetworkNode::AddEntity(1, network::TCPConnection(req->fd, network::NETA_IPv4, network::SCS_CONNECTED));
				Authentication::GetNetworkSystem()->Poller()->Create(req->fd, reinterpret_cast<void*>(hasher));
				reply_packet->SendMessageNoVMAC(req->fd, NET_MSG, AUTH_KEY_REPLY);
				continue;
			}
			else {
				LOG_DEBUG << "(" << sched_getcpu() << ") Authentication failed";
			}
		}
	}

	void Authentication::CreateSecureKey(const std::list<std::shared_ptr<FrameObject>>& req_list) {
		// client side
		for(auto& req : req_list) {
			// We received reply
			auto body = req->Content<KeyReplyPacket>();
			if (body->VerifyVMAC(Authentication::GetNetworkSystem()->Hasher())) {
				Authentication::GetNetworkSystem()->SetAuthState(AUTH_SHARE_KEY);
				NetworkSystem::GetAuthStateMap<true>()->Insert(req->fd, AUTH_SHARE_KEY);
				LOG_DEBUG << "(" << sched_getcpu() << ") Authentication OK";
				Authentication::GetNetworkSystem()->is_connected.notify_all();
			}
			else {
				LOG_DEBUG << "(" << sched_getcpu() << ") Authentication failed";
				Authentication::GetNetworkSystem()->SetAuthState(AUTH_NONE);
				Authentication::GetNetworkSystem()->CloseClientConnection();
				Authentication::GetNetworkSystem()->is_connected.notify_all();
			}
		}
	}

	std::shared_ptr<FrameObject> SendSaltPacket::GetKeyExchangePacket() {
		// Client received salt
		auto frame = std::make_shared<FrameObject>();
		auto packet = frame->Content<KeyExchangePacket>();
		// Derive password and salt to get key
		Crypto::PBKDF(Authentication::GetSecretKey(), reinterpret_cast<const byte*>(Authentication::Password().data()), Authentication::Password().size(), salt, SALT_SIZE);
		std::memcpy(packet->salt, salt, SALT_SIZE);
		Crypto::GetRandom64(packet->nonce);
		Crypto::GetRandom128(packet->nonce2);
		Crypto::GetRandom128(packet->alea);
		Crypto::VMAC64(packet->vmac, frame->Content<KeyExchangePacket,const byte>(), VMAC_MSG_SIZE, Authentication::GetSecretKey(), packet->nonce);
		return frame;
	}

	bool KeyExchangePacket::VerifyVMAC(const byte* key) const {
		// server side
		return Crypto::VMAC_Verify(vmac, reinterpret_cast<const byte*>(this), VMAC_MSG_SIZE, key, nonce);
	}

	std::unique_ptr<std::vector<byte>> KeyExchangePacket::VMAC_BuildHasher(const byte* key) const {
		// server side
		// The variable that will hold the hasher key for this session
		std::unique_ptr<std::vector<byte>> checker_key(new std::vector<byte>(16));
		// We derive the player key using the alea given by the player
		Crypto::VMAC128(checker_key->data(), alea, ALEA_SIZE, key, nonce2);
		return std::move(checker_key);
	}

	std::unique_ptr<std::vector<byte>> KeyExchangePacket::VMAC_BuildHasher() const {
		// client side
		// The variable that will hold the hasher key for this session
		std::unique_ptr<std::vector<byte>> checker_key(new std::vector<byte>(16));
		// We derive the player key using the alea given by the player
		Crypto::VMAC128(checker_key->data(), alea, ALEA_SIZE, Authentication::GetSecretKey(), nonce2);
		return std::move(checker_key);
	}

	std::unique_ptr<cryptography::VMAC_StreamHasher> KeyExchangePacket::VMAC_getHasher(std::unique_ptr<std::vector<byte>>&& key) const {
		return std::unique_ptr<cryptography::VMAC_StreamHasher>(new cryptography::VMAC_StreamHasher(std::move(key), nonce2, 8));
	}

	void KeyReplyPacket::Compute(const byte* m) {
		std::memcpy(challenge, m, NONCE2_SIZE);
		// TODO : entity ID is hardcoded
		VMAC_Checker::Digest(1, vmac, challenge, NONCE2_SIZE);
	}

	bool KeyReplyPacket::VerifyVMAC() const {
		// TODO : entity ID is hardcoded
		return VMAC_Checker::Verify(1, vmac, challenge, NONCE2_SIZE);
	}

	bool KeyReplyPacket::VerifyVMAC(cryptography::VMAC_StreamHasher* hasher) const {
		return hasher->Verify(vmac, challenge, NONCE2_SIZE);
	}

	namespace network_packet_handler {
		template<>
		void INetworkPacketHandler::Process<NET_MSG,AUTH_INIT, false>() {
			ThreadPool::Execute(std::make_shared<TaskReq<chain_t>>(Authentication::GetAuthInitHandler()));
		}

		template<>
		void INetworkPacketHandler::Process<NET_MSG,AUTH_SEND_SALT, true>() {
			auto req_list = GetQueue<NET_MSG,AUTH_SEND_SALT>()->Poll();
			if (!req_list) {
				return;
			}
			for(auto& req : *req_list) {
				// We must send keys
				auto body = req->Content<SendSaltPacket>();
				LOG_DEBUG << "(" << sched_getcpu() << ") salt received : " << std::string(reinterpret_cast<const char*>(body->salt), 8);
	//								Authentication::SetSalt(std::unique_ptr<std::vector<unsigned char>>(m->body.get()));
				auto frame = body->GetKeyExchangePacket();
				auto packet = frame->Content<KeyExchangePacket>();
				// TODO !!!!!
				auto hasher_key = packet->VMAC_BuildHasher();
				Authentication::SetHasher(packet->VMAC_getHasher(std::move(hasher_key)));
				LOG_DEBUG << "(" << sched_getcpu() << ") Sending keys : " << frame->PacketSize() << " bytes";
				Authentication::SetAuthState(AUTH_KEY_EXCHANGE);
				frame->SendMessageNoVMAC(req->fd, NET_MSG, AUTH_KEY_EXCHANGE);
			}
		}

		template<>
		void INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_EXCHANGE, false>() {
			auto req_list = GetQueue<NET_MSG,AUTH_KEY_EXCHANGE>()->Poll();
			if (!req_list) {
				return;
			}
			Authentication::CheckKeyExchange(*req_list);
		}

		template<>
		void INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_REPLY, true>() {
			auto req_list = GetQueue<NET_MSG,AUTH_KEY_REPLY>()->Poll();
			if (!req_list) {
				return;
			}
			Authentication::CreateSecureKey(*req_list);
		}
	}

	namespace reflection {
		template <> inline const char* GetNetworkHandler<NET_MSG,AUTH_INIT>(void) { return "AuthenticationHandler"; }
		template <> inline const char* GetNetworkHandler<NET_MSG,AUTH_KEY_EXCHANGE>(void) { return "AuthenticationHandler"; }
	}
}
