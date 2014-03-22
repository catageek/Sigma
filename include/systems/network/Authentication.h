#ifndef AUTHENTICATION_H_INCLUDED
#define AUTHENTICATION_H_INCLUDED

#include <cstdint>
#include <memory>
#include "systems/network/AtomicMap.hpp"
#include "systems/network/NetworkSystem.h"
#include "systems/network/Crypto.h"

#define LOGIN_FIELD_SIZE	16
#define SALT_SIZE			8

#define AUTH_INIT		1
#define SEND_SALT		2
#define DH_EXCHANGE		3
#define DH_SHARE_KEY	4
#define AUTHENTICATED	5
#define USER_UNKNOWN	16

// The server stores salt and SHA-256(salt,SHA-256(login|":"|password)), salt being chosen randomly

namespace Sigma {
	struct GetSaltTaskRequest;
	struct DHKeyExchangePacket;

	class Authentication {
	public:
		static Crypto* GetCryptoEngine() { return &crypto; };
		static bool ComputeSharedSecretKey(const DHKeyExchangePacket& packet);
		static void SetSalt(std::unique_ptr<std::vector<byte>>&& salt) { crypto.SetSalt(std::move(salt)); };
		static std::shared_ptr<DHKeyExchangePacket> GetDHKeysPacket();
	private:
		static Crypto crypto;
	};

	struct DHKeyExchangePacket {
		byte dhkey[32];										// DH public key
		byte alea[16];										// A random number used to derive the key
		byte nonce[8];										// a random number used as nonce
		byte vmac[8];										// VMAC of the message

		bool ComputeSharedKey() {
			if(! VerifyVMAC()) {
				return false;
			}
			// TODO
			return true;
		}
	private:
		bool VerifyVMAC();
	};

	struct SendSaltPacket {
		byte salt[SALT_SIZE];								// the stored number used as salt for password
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
