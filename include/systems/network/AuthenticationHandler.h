#ifndef AUTHENTICATIONHANDLER_H_INCLUDED
#define AUTHENTICATIONHANDLER_H_INCLUDED

#include <cstdint>
#include <memory>
#include "AtomicMap.hpp"
#include "AtomicQueue.hpp"
#include "ThreadPool.h"
#include "systems/network/Crypto.h"
#include "systems/network/Protocol.h"
#include <utmpx.h>

#define LOGIN_FIELD_SIZE	16
#define SALT_SIZE			8
#define ALEA_SIZE			16
#define NONCE2_SIZE			16
#define	NONCE_SIZE			8
#define VMAC_MSG_SIZE		(ALEA_SIZE + NONCE2_SIZE + NONCE_SIZE)

#define AUTH_NONE			0
#define AUTH_INIT			1
#define AUTH_SEND_SALT		2
#define AUTH_KEY_EXCHANGE	3
#define AUTH_KEY_REPLY		4
#define AUTH_SHARE_KEY		5
#define AUTHENTICATED		5

// The server stores salt and SHA-256(salt,SHA-256(login|":"|password)), salt being chosen randomly

namespace Sigma {
	struct GetSaltTaskRequest;
	struct KeyExchangePacket;
	class FrameObject;
	class NetworkSystem;

	namespace cryptography {
		class VMAC_StreamHasher;
	}

	struct SendSaltPacket {
		byte salt[SALT_SIZE];								// the stored number used as salt for password
		std::shared_ptr<FrameObject> GetKeyExchangePacket();
	};

	struct KeyExchangePacket;

	struct KeyReplyPacket {
		friend KeyExchangePacket;
		bool VerifyVMAC() const;
		bool VerifyVMAC(cryptography::VMAC_StreamHasher* hasher) const;
		void Compute(const byte* m);
	private:
		byte challenge[NONCE2_SIZE];
		byte vmac[VMAC_SIZE];
	};

	struct KeyExchangePacket {
		byte salt[SALT_SIZE];
		byte alea[ALEA_SIZE];								// Random number used to derive the shared secret
		byte nonce2[NONCE2_SIZE];							// A random number used as nonce for the VMAC hasher to build
		byte nonce[NONCE_SIZE];								// a random number used as nonce for the VMAC of this packet
		byte vmac[VMAC_SIZE];								// VMAC of the message using nonce and shared secret

		std::unique_ptr<cryptography::VMAC_StreamHasher> VMAC_getHasher(std::unique_ptr<std::vector<byte>>&& key) const;
		std::unique_ptr<std::vector<byte>> VMAC_BuildHasher(const byte* key) const;
		std::unique_ptr<std::vector<byte>> VMAC_BuildHasher() const;
		bool VerifyVMAC(const byte* key) const;
	};

	class Authentication {
		friend std::shared_ptr<FrameObject> SendSaltPacket::GetKeyExchangePacket();
		friend std::unique_ptr<std::vector<byte>> KeyExchangePacket::VMAC_BuildHasher() const;
	public:
		Authentication() {};

		template<bool isClient>
		void Initialize(NetworkSystem* netsystem) const {
			Authentication::SetSystem<isClient>(netsystem);
			auto f = chain_t({
				std::bind(&Sigma::Authentication::RetrieveSalt, this),
				std::bind(&Sigma::Authentication::SendSalt, this)
			});
			auth_init_handler = std::make_shared<chain_t>(f);
		}

		static void SetPassword(const std::string& password) { Authentication::password = password; };

		template<bool isClient = false>
		static NetworkSystem* const GetNetworkSystem() { return netsystem; };

		static void SetHasher(std::unique_ptr<cryptography::VMAC_StreamHasher>&& hasher);

		static void SetAuthState(uint32_t state);
		static void CheckKeyExchange(const std::list<std::shared_ptr<FrameObject>>& req_list);
		static void CreateSecureKey(const std::list<std::shared_ptr<FrameObject>>& req_list);
		static std::shared_ptr<chain_t> GetAuthInitHandler() { return auth_init_handler; };

		int SendSalt() const;
	private:

		int RetrieveSalt() const;
		static const std::string& Password() { return password; };

		static unsigned char* GetSecretKey() { return secret_key.data(); };

		template<bool isClient>
		static void SetSystem(NetworkSystem* system) { netsystem = system; };

		static NetworkSystem* netsystem;
		static NetworkSystem* netsystem_client;
		static std::shared_ptr<chain_t> auth_init_handler;
		const AtomicQueue<std::shared_ptr<FrameObject>> salt_retrieved;
		static std::string password;
		static std::vector<byte> secret_key;
	};

	struct AuthInitPacket {
		msg_hdr header;
		char login[LOGIN_FIELD_SIZE];
	};

	struct GetSaltTaskRequest {
		GetSaltTaskRequest(int fd, char login[LOGIN_FIELD_SIZE]) : fd(fd), login(login) {};
		SendSaltPacket packet;
		char* login;
		int fd;
	};

	namespace network_packet_handler {
		template<>
		void INetworkPacketHandler::Process<NET_MSG,AUTH_INIT, false>();

		template<>
		void INetworkPacketHandler::Process<NET_MSG,AUTH_SEND_SALT, true>();

		template<>
		void INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_EXCHANGE, false>();

		template<>
		void INetworkPacketHandler::Process<NET_MSG,AUTH_KEY_REPLY, true>();
	}
}

#endif // AUTHENTICATIONHANDLER_H_INCLUDED
