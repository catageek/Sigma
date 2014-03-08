#ifndef AUTHWORKER_H_INCLUDED
#define AUTHWORKER_H_INCLUDED

#include "systems/network/AuthenticationPacket.h"

namespace Sigma {
	class AuthWorker {
	public:
		AuthWorker() {};
		virtual ~AuthWorker() {};

		bool Authenticate(const AuthenticationPacket* packet) {
			// TODO: find login and retrieve password
			std::string expected = "good_password";
			// TODO: compare submitted pasword to the one we have
			return expected == packet->data.password;
		}
	};
}

#endif // AUTHWORKER_H_INCLUDED
