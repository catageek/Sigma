#ifndef AUTHENTICATION_H_INCLUDED
#define AUTHENTICATION_H_INCLUDED

#include <cstdint>
#include <memory>
#include "systems/network/AtomicMap.hpp"
#include "systems/network/NetworkSystem.h"
#include "systems/network/Crypto.h"

#define LOGIN_FIELD_SIZE	16
#define SALT_SIZE			8
#define ALEA_SIZE			16
#define NONCE2_SIZE			16
#define	NONCE_SIZE			8
#define VMAC_SIZE			8
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

	class Authentication {
	public:
		static Crypto* GetCryptoEngine() { return &crypto; };
		static void SetSalt(std::unique_ptr<std::vector<byte>>&& salt) { crypto.SetSalt(std::move(salt)); };
	private:
		static Crypto crypto;
	};

	struct KeyExchangePacket;

	struct KeyReplyPacket {
		friend KeyExchangePacket;
		bool VerifyVMAC();
	private:
		bool Compute(const byte* m);
		byte challenge[NONCE2_SIZE];
		byte vmac[VMAC_SIZE];
	};

	struct KeyExchangePacket {
		byte alea[ALEA_SIZE];								// Random number used to derive the shared secret
		byte nonce2[NONCE2_SIZE];							// A random number used as nonce for the VMAC hasher to build
		byte nonce[NONCE_SIZE];								// a random number used as nonce for the VMAC of this packet
		byte vmac[VMAC_SIZE];								// VMAC of the message using nonce and shared secret

		std::shared_ptr<FrameObject> ComputeSessionKey() {
			if(! VerifyVMAC()) {
				return std::shared_ptr<FrameObject>();
			}
			if(! VMAC_BuildHasher()) {
				return std::shared_ptr<FrameObject>();
			}
			auto ret = std::make_shared<FrameObject>();
			if(! ret->Content<KeyReplyPacket>()->Compute(nonce2)) {
				return std::shared_ptr<FrameObject>();
			}
			return ret;
		}

		bool VMAC_BuildHasher();
		bool VerifyVMAC();
	};

	struct SendSaltPacket {
		byte salt[SALT_SIZE];								// the stored number used as salt for password
		std::shared_ptr<FrameObject> GetKeyExchangePacket();
	};

	struct AuthInitPacket {
		msg_hdr header;
		char login[LOGIN_FIELD_SIZE];

	private:
		byte* GetSalt() const {
			// TODO
			return nullptr;
		}

		static AtomicMap<int, std::shared_ptr<GetSaltTaskRequest>> salt_req;

	};

	struct GetSaltTaskRequest {
		GetSaltTaskRequest(int fd, char login[LOGIN_FIELD_SIZE]) : fd(fd), login(login) {};
		SendSaltPacket packet;
		char* login;
		int fd;
	};
}

#endif // AUTHENTICATION_H_INCLUDED
