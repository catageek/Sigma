#ifndef AUTHENTICATIONHANDLER_H_INCLUDED
#define AUTHENTICATIONHANDLER_H_INCLUDED

#include <cstdint>
#include <memory>
#include "AtomicMap.hpp"
#include "ThreadPool.h"
#include "systems/network/Crypto.h"
#include "systems/network/Protocol.h"

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

	namespace cryptography {
		class VMAC_StreamHasher;
	}

	class Authentication {
	public:
		Authentication()  {};

		void Initialize() {
			auto f = chain_t({
				std::bind(&Sigma::Authentication::RetrieveSalt, this),
				std::bind(&Sigma::Authentication::SendSalt, this)
			});
			auth_init_handler = std::make_shared<chain_t>(f);
		}

		std::shared_ptr<chain_t> GetAuthInitHandler() { return auth_init_handler; };
		Crypto* GetCryptoEngine() { return &crypto; };
		void SetSalt(std::unique_ptr<std::vector<byte>>&& salt) { crypto.SetSalt(std::move(salt)); };
		AtomicMap<int,char>* GetAuthStateMap() { return &auth_state; };
	private:

		int RetrieveSalt();

		int SendSalt();

		Crypto crypto;
		AtomicMap<int,char> auth_state;						// state of the connections
		std::shared_ptr<chain_t> auth_init_handler;
		AtomicQueue<std::shared_ptr<FrameObject>> salt_retrieved;
	};

	struct KeyExchangePacket;

	struct KeyReplyPacket {
		friend KeyExchangePacket;
		bool VerifyVMAC();
		bool VerifyVMAC(cryptography::VMAC_StreamHasher* hasher);
	private:
		void Compute(const byte* m);
		byte challenge[NONCE2_SIZE];
		byte vmac[VMAC_SIZE];
	};

	struct KeyExchangePacket {
		byte alea[ALEA_SIZE];								// Random number used to derive the shared secret
		byte nonce2[NONCE2_SIZE];							// A random number used as nonce for the VMAC hasher to build
		byte nonce[NONCE_SIZE];								// a random number used as nonce for the VMAC of this packet
		byte vmac[VMAC_SIZE];								// VMAC of the message using nonce and shared secret

		std::shared_ptr<FrameObject> ComputeSessionKey(int fd);
		std::unique_ptr<cryptography::VMAC_StreamHasher> VMAC_getHasher(std::unique_ptr<std::vector<byte>>&& key) const;
		std::unique_ptr<std::vector<byte>> VMAC_BuildHasher() const;
		bool VerifyVMAC();
	};

	struct SendSaltPacket {
		byte salt[SALT_SIZE];								// the stored number used as salt for password
		std::shared_ptr<FrameObject> GetKeyExchangePacket();
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
}

#endif // AUTHENTICATIONHANDLER_H_INCLUDED
