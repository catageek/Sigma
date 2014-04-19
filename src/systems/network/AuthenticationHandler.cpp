#include "systems/network/AuthenticationHandler.h"
#include "systems/network/NetworkSystem.h"
#include <cstring>
#include "systems/network/ConnectionData.h"
#include "systems/network/ProtocolTemplates.h"
#include "composites/NetworkNode.h"
#include "systems/network/VMAC_StreamHasher.h"
#include "crypto++/vmac.h"

namespace Sigma {
	NetworkSystem* Authentication::netsystem = nullptr;
	NetworkSystem* Authentication::netsystem_client = nullptr;
	std::shared_ptr<chain_t> Authentication::auth_init_handler;
	std::string Authentication::password;
	std::vector<byte> Authentication::secret_key(16);

	extern template void NetworkSystem::CloseConnection<CLIENT>(const FrameObject* frame) const;
	extern template	void NetworkSystem::CloseConnection<SERVER>(const FrameObject* frame) const;

	template<>
	NetworkSystem* const Authentication::GetNetworkSystem<CLIENT>() { return netsystem_client; };

	template<>
	void Authentication::SetSystem<CLIENT>(NetworkSystem* system) { netsystem_client = system; };

	template<>
	void Authentication::SetSystem<SERVER>(NetworkSystem* system) { netsystem = system; };

	int Authentication::RetrieveSalt() const {
		auto req_list = network_packet_handler::INetworkPacketHandler<SERVER>::GetQueue<NET_MSG,AUTH_INIT>()->Poll();
		if (req_list.empty()) {
			return STOP;
		}
		for(auto& req : req_list) {
			auto reply = std::make_shared<FrameObject>(req->FileDescriptor());
			reply->Resize<NONE>(sizeof(SendSaltPacket));
			// TODO : salt is hardcoded !!!
			auto login = req->Content<AuthInitPacket>()->login;
			// GetSaltFromsomewhere(reply->Body(), packet->login);
			std::memcpy(reply->Content<SendSaltPacket,char>(), std::string("abcdefgh").data(), 8);
			req->CxData()->SetAuthState(AUTH_KEY_EXCHANGE);
//			LOG_DEBUG << "(" << sched_getcpu() << ") Queing salt to send";
			salt_retrieved.Push(std::move(reply));
		}
		return CONTINUE;
	}


	int Authentication::SendSalt() const {
		auto req_list = salt_retrieved.Poll();
		if (req_list.empty()) {
			return STOP;
		}
		for(auto& req : req_list) {
			auto reply = req->FullFrame();
			if (reply) {
				// send salt
				req->SendMessageNoVMAC(req->fd, NET_MSG, AUTH_SEND_SALT);
			}
			else {
				// close connection
				// TODO: send error message
				//TCPConnection(req->fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(challenge.get()), sizeof(*challenge));
				LOG_DEBUG << "User unknown";
				netsystem->CloseConnection<SERVER>(req.get());
			}
		}
		return CONTINUE;
	}

	void Authentication::CheckKeyExchange(const std::list<std::shared_ptr<FrameObject>>& req_list) {
		// server side
		for(auto& req : req_list) {
			if(! req->CxData()->CompareAuthState(AUTH_KEY_EXCHANGE)) {
				// This is not the packet expected
				LOG_ERROR << "(" << sched_getcpu() << ") Unexpected packet received with length = " << req->FullFrame()->length << ", closing...";
				Authentication::GetNetworkSystem<SERVER>()->CloseConnection<SERVER>(req.get());
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
				LOG_DEBUG << "Bad password, closing...";
				req->CxData()->SetAuthState(AUTH_NONE);
				Authentication::GetNetworkSystem<SERVER>()->CloseConnection<SERVER>(req.get());
				continue;
			}
			auto checker_key = recv_packet->VMAC_BuildHasher(key);
			// TODO: Hard coded entity #id
			auto authentifier = std::make_shared<cryptography::VMAC_StreamHasher>(std::move(checker_key), recv_packet->nonce2, 8);
			auto cx_data = new ConnectionData(1, authentifier->Hasher(), authentifier->Verifier());
			auto reply_packet = std::make_shared<FrameObject>();
    		*reply_packet << *Authentication::GetNetworkSystem<SERVER>()->ServerPublicKey();
    		std::vector<unsigned char> vmac(8);
	    	(*cx_data->Hasher())(vmac.data(), reply_packet->Content<KeyReplyPacket, unsigned char>(), Authentication::GetNetworkSystem<SERVER>()->ServerPublicKey()->size());
	    	*reply_packet << vmac;
//			reply_packet->Content<KeyReplyPacket>()->Compute(recv_packet->nonce2, cx_data->Hasher());

			LOG_DEBUG << "(" << sched_getcpu() << ") Authentication OK";
			cx_data->SetAuthState(AUTH_SHARE_KEY);
			// TODO: Hardcoded entity
			NetworkNode::AddEntity(1, network::TCPConnection(req->fd, network::NETA_IPv4, network::SCS_CONNECTED));
			Authentication::GetNetworkSystem<SERVER>()->Poller()->Create(req->FileDescriptor(), reinterpret_cast<void*>(cx_data));
			reply_packet->SendMessageNoVMAC(req->fd, NET_MSG, AUTH_KEY_REPLY);
		}
	}

	void Authentication::CreateSecureKey(const std::list<std::shared_ptr<FrameObject>>& req_list) {
		// client side
		for(auto& req : req_list) {
			// We received reply
			auto body = req->Content<KeyReplyPacket,unsigned char>();
			auto v = Authentication::GetNetworkSystem<CLIENT>()->Verifier();
			if ((*v)(body + PUBLIC_KEY_SIZE, body, PUBLIC_KEY_SIZE)) {
                auto key = make_unique<std::vector<unsigned char>>(PUBLIC_KEY_SIZE);
                std::memcpy(key->data(), body, PUBLIC_KEY_SIZE);
                Authentication::GetNetworkSystem<CLIENT>()->SetServerPublicKey(std::move(key));
				req->CxData()->SetAuthState(AUTH_SHARE_KEY);
				SetAuthState<CLIENT>(AUTH_SHARE_KEY);
				LOG_DEBUG << "(" << sched_getcpu() << ") Authentication OK";
				Authentication::GetNetworkSystem<CLIENT>()->is_connected.notify_all();
			}
			else {
				LOG_DEBUG << "(" << sched_getcpu() << ") Authentication failed";
				SetAuthState<CLIENT>(AUTH_NONE);
				Authentication::GetNetworkSystem<CLIENT>()->CloseConnection<CLIENT>(req.get());
				Authentication::GetNetworkSystem<CLIENT>()->is_connected.notify_all();
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

	void KeyReplyPacket::Compute(const byte* m, const cryptography::VMAC_StreamHasher* hasher) {
		std::memcpy(challenge, m, NONCE2_SIZE);
		hasher->CalculateDigest(vmac, challenge, NONCE2_SIZE);
	}

	namespace network_packet_handler {
		template<>
		template<>
		void INetworkPacketHandler<SERVER>::Process<NET_MSG,AUTH_INIT>() {
			ThreadPool::Execute(std::make_shared<TaskReq<chain_t>>(Authentication::GetAuthInitHandler()));
		}

		template<>
		template<>
		void INetworkPacketHandler<CLIENT>::Process<NET_MSG,AUTH_SEND_SALT>() {
			auto req_list = GetQueue<NET_MSG,AUTH_SEND_SALT>()->Poll();
			if (req_list.empty()) {
				return;
			}
			for(auto& req : req_list) {
				// We must send keys
				auto body = req->Content<SendSaltPacket>();
				LOG_DEBUG << "(" << sched_getcpu() << ") salt received : " << std::string(reinterpret_cast<const char*>(body->salt), 8);
	//								Authentication::SetSalt(std::unique_ptr<std::vector<unsigned char>>(m->body.get()));
				auto frame = body->GetKeyExchangePacket();
				auto packet = frame->Content<KeyExchangePacket>();
				// TODO !!!!!
				auto hasher_key = std::move(packet->VMAC_BuildHasher());
//				LOG_DEBUG << "(" << sched_getcpu() << ") Sending keys : " << frame->PacketSize() << " bytes, key is " << std::hex << (uint64_t) hasher_key->data();
				auto authentifier = std::make_shared<cryptography::VMAC_StreamHasher>(std::move(hasher_key), packet->nonce2, 8);
				Authentication::SetHasher<CLIENT>(authentifier->Hasher());
				Authentication::SetVerifier<CLIENT>(authentifier->Verifier());
				Authentication::SetAuthState<CLIENT>(AUTH_KEY_EXCHANGE);
				req->CxData()->SetAuthState(AUTH_KEY_EXCHANGE);
				frame->SendMessageNoVMAC(req->fd, NET_MSG, AUTH_KEY_EXCHANGE);
			}
		}

		template<>
		template<>
		void INetworkPacketHandler<SERVER>::Process<NET_MSG,AUTH_KEY_EXCHANGE>() {
			auto req_list = GetQueue<NET_MSG,AUTH_KEY_EXCHANGE>()->Poll();
			if (req_list.empty()) {
				return;
			}
			Authentication::CheckKeyExchange(req_list);
		}

		template<>
		template<>
		void INetworkPacketHandler<CLIENT>::Process<NET_MSG,AUTH_KEY_REPLY>() {
			auto req_list = GetQueue<NET_MSG,AUTH_KEY_REPLY>()->Poll();
			if (req_list.empty()) {
				return;
			}
			Authentication::CreateSecureKey(req_list);
		}
	}

	namespace reflection {
		template <> inline const char* GetNetworkHandler<NET_MSG,AUTH_INIT>(void) { return "AuthenticationHandler"; }
		template <> inline const char* GetNetworkHandler<NET_MSG,AUTH_KEY_EXCHANGE>(void) { return "AuthenticationHandler"; }
	}
}
