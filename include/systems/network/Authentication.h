#ifndef AUTHENTICATION_H_INCLUDED
#define AUTHENTICATION_H_INCLUDED

#include <cstdint>
#include "systems/network/AtomicMap.hpp"

#define AUTH_REQUEST	1
#define USER_UNKNOWN	2

namespace Sigma {
	struct AuthChallReqPacket {
		AuthChallReqPacket(uint64_t nonce, uint16_t salt) : nonce(nonce), salt(salt) {};
		uint64_t nonce;		// a random number
		uint16_t salt;		// a number used as salt for password
		char padding[6];
	};

	struct AuthInitPacket {;
		char login[16];

		std::shared_ptr<AuthChallReqPacket> GetChallenge() {
			// TODO
			return std::make_shared<AuthChallReqPacket>(NetworkSystem::GetCryptoEngine()->GetNonce(), 0);
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
