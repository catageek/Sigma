#ifndef AUTHENTICATIONPACKET_H_INCLUDED
#define AUTHENTICATIONPACKET_H_INCLUDED

#include <cstdint>

#define AUTH_REQUEST			1
#define AUTH_REPLY_OK			2
#define AUTH_REPLY_KO			3

namespace Sigma {
	struct AuthenticationData {
		char login[16];
		char passhash[32];
		uint16_t seed;
	} ;

	struct AuthenticationPacket {;
		union {
			AuthenticationData data;
			char buffer[50];
		};
	};
}

#endif // AUTHENTICATIONPACKET_H_INCLUDED
