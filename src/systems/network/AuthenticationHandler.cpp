#include "systems/network/AuthenticationHandler.h"
#include "systems/network/NetworkSystem.h"
#include <cstring>
#include "composites/VMAC_Checker.h"
#include "composites/NetworkNode.h"
#include "crypto++/vmac.h"

namespace Sigma {

	int Authentication::RetrieveSalt() {
		auto req_list = network_packet_handler::INetworkPacketHandler::GetQueue<NET_MSG,AUTH_INIT>()->Poll();
		if (!req_list) {
			return STOP;
		}
		for(auto& req : *req_list) {
			auto reply = std::make_shared<FrameObject>(req->fd);
			reply->Resize(sizeof(SendSaltPacket));
			// TODO : salt is hardcoded !!!
			auto login = req->Content<AuthInitPacket>()->login;
			// GetSaltFromsomewhere(reply->Body(), packet->login);
			std::memcpy(reply->Content<SendSaltPacket,char>(), std::string("abcdefgh").data(), 8);
			LOG_DEBUG << "Queing salt to send";
			salt_retrieved.Push(std::move(reply));
		}
		return CONTINUE;
	}


	int Authentication::SendSalt() {
		auto req_list = salt_retrieved.Poll();
		if (!req_list) {
			return STOP;
		}
		for(auto& req : *req_list) {
			auto reply = req->FullFrame();
			if (reply) {
				// send salt
				req->SendMessageNoVMAC(req->fd, 1, 2);
				Authentication::GetAuthStateMap()->At(req->fd) = AUTH_KEY_EXCHANGE;
			}
			else {
				// close connection
				// TODO: send error message
				//TCPConnection(req->fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(challenge.get()), sizeof(*challenge));
				LOG_DEBUG << "User unknown";
				NetworkSystem::CloseConnection(req->fd);
			}
		}
		return CONTINUE;
	}


	std::shared_ptr<FrameObject> SendSaltPacket::GetKeyExchangePacket() {
		auto frame = std::make_shared<FrameObject>();
		auto packet = frame->Content<KeyExchangePacket>();
		// TODO : hard coded key
		byte key[] = "very_secret_key";
		auto crypto = NetworkSystem::GetAuthenticationComponent().GetCryptoEngine();
		crypto->GetRandom64(packet->nonce);
		crypto->GetRandom128(packet->nonce2);
		crypto->GetRandom128(packet->alea);
		crypto->VMAC64(packet->vmac, frame->Content<KeyExchangePacket,const byte>(), VMAC_MSG_SIZE, key, packet->nonce);
		return frame;
	}

	bool KeyExchangePacket::VerifyVMAC() {
		// TODO : hard coded key
		byte key[] = "very_secret_key";
		return NetworkSystem::GetAuthenticationComponent().GetCryptoEngine()->VMAC_Verify(vmac, reinterpret_cast<const byte*>(this), VMAC_MSG_SIZE, key, nonce);
	}

	std::shared_ptr<FrameObject> KeyExchangePacket::ComputeSessionKey(int fd) {
		if(! VerifyVMAC()) {
			return std::shared_ptr<FrameObject>();
		}
		auto checker_key = VMAC_BuildHasher();
		if(! checker_key) {
			return std::shared_ptr<FrameObject>();
		}
		// TODO: Entity #id is hardcoded
		auto hasher = VMAC_Checker::AddEntity(1, std::move(checker_key), nonce2, 8);
		NetworkSystem::Poller()->Create(fd, reinterpret_cast<void*>(hasher));

		auto ret = std::make_shared<FrameObject>();
		ret->Content<KeyReplyPacket>()->Compute(nonce2);
		return ret;
	}


	std::unique_ptr<std::vector<byte>> KeyExchangePacket::VMAC_BuildHasher() const {
		// TODO : hard coded key of the player
		byte key[] = "very_secret_key";
		// The variable that will hold the hasher key for this session
		std::unique_ptr<std::vector<byte>> checker_key(new std::vector<byte>(16));
		// We derive the player key using the alea given by the player
		NetworkSystem::GetAuthenticationComponent().GetCryptoEngine()->VMAC128(checker_key->data(), alea, ALEA_SIZE, key, nonce2);
		return std::move(checker_key);
/*		// We store the key in the component
		LOG_DEBUG << "Inserting checker";
		if (fd <0) {
			// client hasher is stored under id #1
			VMAC_Checker::AddEntity(1, std::move(checker_key), nonce2, 8);
		}
		else {
			// server
			// TODO : entity ID is hardcoded
			auto hasher = VMAC_Checker::AddEntity(1, std::move(checker_key), nonce2, 8);
			NetworkSystem::Poller()->Create(fd, reinterpret_cast<void*>(hasher));
		}
		return true;
*/	}

	std::unique_ptr<cryptography::VMAC_StreamHasher> KeyExchangePacket::VMAC_getHasher(std::unique_ptr<std::vector<byte>>&& key) const {
		return std::unique_ptr<cryptography::VMAC_StreamHasher>(new cryptography::VMAC_StreamHasher(std::move(key), nonce2, 8));
	}

	void KeyReplyPacket::Compute(const byte* m) {
		std::memcpy(challenge, m, NONCE2_SIZE);
		// TODO : entity ID is hardcoded
		VMAC_Checker::Digest(1, vmac, challenge, NONCE2_SIZE);
	}

	bool KeyReplyPacket::VerifyVMAC() {
		// TODO : entity ID is hardcoded
		return VMAC_Checker::Verify(1, vmac, challenge, NONCE2_SIZE);
	}

	bool KeyReplyPacket::VerifyVMAC(cryptography::VMAC_StreamHasher* hasher) {
		return hasher->Verify(vmac, challenge, NONCE2_SIZE);
	}

	namespace network_packet_handler {
		template<>
		void INetworkPacketHandler::Process<NET_MSG,AUTH_INIT>() {
			NetworkSystem::GetThreadPool()->Execute(std::make_shared<TaskReq<chain_t>>(NetworkSystem::GetAuthenticationComponent().GetAuthInitHandler()));
		}

		template<>
		void INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_EXCHANGE>() {
			auto req_list = GetQueue<NET_MSG,AUTH_KEY_EXCHANGE>()->Poll();
			if (!req_list) {
				return;
			}
			for(auto& req : *req_list) {
				if(! NetworkSystem::GetAuthenticationComponent().GetAuthStateMap()->Count(req->fd) || NetworkSystem::GetAuthenticationComponent().GetAuthStateMap()->At(req->fd) != AUTH_KEY_EXCHANGE) {
					// This is not the packet expected
					LOG_DEBUG << "Unexpected packet received with length = " << req->FullFrame()->length;
					NetworkSystem::CloseConnection(req->fd);
					continue;
				}
				auto reply_packet = req->Content<KeyExchangePacket>()->ComputeSessionKey(req->fd);
				if(reply_packet) {
					LOG_DEBUG << "VMAC check passed";
					reply_packet->SendMessageNoVMAC(req->fd, NET_MSG, AUTH_KEY_REPLY);
					NetworkSystem::GetAuthenticationComponent().GetAuthStateMap()->At(req->fd) = AUTH_SHARE_KEY;
					// TODO: Hardcoded entity
					NetworkNode::AddEntity(1, network::TCPConnection(req->fd, network::NETA_IPv4, network::SCS_CONNECTED));
					continue;
				}
				else {
					LOG_DEBUG << "VMAC check failed";
				}
			}
		}
	}

	namespace reflection {
		template <> inline const char* GetNetworkHandler<NET_MSG,AUTH_INIT>(void) { return "AuthenticationHandler"; }
		template <> inline const char* GetNetworkHandler<NET_MSG,AUTH_KEY_EXCHANGE>(void) { return "AuthenticationHandler"; }
	}
}
