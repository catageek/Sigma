#ifndef AUTHENTICATION_H_INCLUDED
#define AUTHENTICATION_H_INCLUDED

#include <cstdint>
#include "systems/network/AtomicMap.hpp"
#include "systems/network/NetworkSystem.h"
#include "systems/network/Crypto.h"

#define LOGIN_FIELD_SIZE	16

#define AUTH_REQUEST	1
#define CHALLENGE		2
#define USER_UNKNOWN	16

namespace Sigma {
	struct ChallengePrepareTaskRequest;

	class Authentication {
	public:
		static Crypto* GetCryptoEngine() { return &crypto; };
	private:
		static Crypto crypto;
	};

	struct AuthChallReqPacket {
		AuthChallReqPacket(uint64_t nonce, uint16_t salt) : nonce(nonce), salt(salt) {};
		uint64_t nonce;		// a random number
		uint16_t salt;		// a number used as salt for password
		char padding[6];
	};

	struct AuthInitPacket {;
		char login[LOGIN_FIELD_SIZE];

		std::shared_ptr<AuthChallReqPacket> GetChallenge() {
			// TODO
			return std::make_shared<AuthChallReqPacket>(Authentication::GetCryptoEngine()->GetNonce(), 0);
		}

	private:
		uint16_t GetSalt() const {
			// TODO
			return 0;
		}

		static AtomicMap<int, std::shared_ptr<ChallengePrepareTaskRequest>> challenge_req;

	};

	struct ChallengePrepareTaskRequest {
		ChallengePrepareTaskRequest(int fd, std::shared_ptr<AuthInitPacket>&& packet) : fd(fd), packet(std::move(packet)) {};
		std::shared_ptr<AuthInitPacket> packet;
		int fd;
	};


	struct AuthChallRepPacket {
		char reply[32];		// A 256 bit reply

		bool Authenticate() const {
			// TODO: compare submitted pasword to the one we have
			return true;
		}
	};

	struct AuthResultPacket {
		char code;
	};
}

#endif // AUTHENTICATION_H_INCLUDED
