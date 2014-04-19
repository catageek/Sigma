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
#define PUBLIC_KEY_SIZE     32
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
		void Compute(const byte* m, const cryptography::VMAC_StreamHasher* const hasher);
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

		std::unique_ptr<std::vector<byte>> VMAC_BuildHasher(const byte* key) const;
		std::unique_ptr<std::vector<byte>> VMAC_BuildHasher() const;
		bool VerifyVMAC(const byte* key) const;
	};

	class Authentication {
		friend std::shared_ptr<FrameObject> SendSaltPacket::GetKeyExchangePacket();
		friend std::unique_ptr<std::vector<byte>> KeyExchangePacket::VMAC_BuildHasher() const;

	public:
		Authentication() {};

		template<TagType T>
		void Initialize(NetworkSystem* netsystem) const {
			SetSystem<T>(netsystem);
			auto f = chain_t({
				std::bind(&Sigma::Authentication::RetrieveSalt, this),
				std::bind(&Sigma::Authentication::SendSalt, this)
			});
			auth_init_handler = std::make_shared<chain_t>(f);
		}

		static void SetPassword(const std::string& password) { Authentication::password = password; };

		template<TagType T>
		static NetworkSystem* const GetNetworkSystem() { return netsystem; };

		template<TagType T>
		static void SetHasher(std::unique_ptr<std::function<void(unsigned char*,const unsigned char*,size_t)>>&& hasher) {
		    GetNetworkSystem<T>()->SetHasher(std::move(hasher));
	    };

		template<TagType T>
		static void SetVerifier(std::unique_ptr<std::function<bool(const unsigned char*,const unsigned char*,size_t)>>&& verifier) {
		    GetNetworkSystem<T>()->SetVerifier(std::move(verifier));
	    };

		template<TagType T>
		static void SetAuthState(uint32_t state) { GetNetworkSystem<T>()->SetAuthState(state); };

		static void CheckKeyExchange(const std::list<std::shared_ptr<FrameObject>>& req_list);
		static void CreateSecureKey(const std::list<std::shared_ptr<FrameObject>>& req_list);
		static std::shared_ptr<chain_t> GetAuthInitHandler() { return auth_init_handler; };

		int SendSalt() const;
	private:

		int RetrieveSalt() const;
		static const std::string& Password() { return password; };

		static unsigned char* GetSecretKey() { return secret_key.data(); };

		template<TagType T>
		static void SetSystem(NetworkSystem* system);

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
		template<>
		void INetworkPacketHandler<SERVER>::Process<NET_MSG,AUTH_INIT>();

		template<>
		template<>
		void INetworkPacketHandler<CLIENT>::Process<NET_MSG,AUTH_SEND_SALT>();

		template<>
		template<>
		void INetworkPacketHandler<SERVER>::Process<NET_MSG,AUTH_KEY_EXCHANGE>();

		template<>
		template<>
		void INetworkPacketHandler<CLIENT>::Process<NET_MSG,AUTH_KEY_REPLY>();
	}
}

#endif // AUTHENTICATIONHANDLER_H_INCLUDED
