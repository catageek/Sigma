#ifndef AUTHENTICATIONPACKET_H_INCLUDED
#define AUTHENTICATIONPACKET_H_INCLUDED

namespace Sigma {
			struct AuthenticationData {
				char login[16];
				char password[16];
			} ;


	struct AuthenticationPacket {;
		union {
			AuthenticationData data;
			char buffer[32];
		};
	};
}

#endif // AUTHENTICATIONPACKET_H_INCLUDED
