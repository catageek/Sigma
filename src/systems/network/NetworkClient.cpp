#include <cstring>
#include "systems/network/NetworkClient.h"
#include "systems/network/Authentication.h"
#include "systems/network/Protocol.h"

namespace Sigma {
	bool NetworkClient::Connect(const char *ip, unsigned short port) {
		cnx = TCPConnection();
		if (! cnx.Init(NETA_IPv4)) {
			LOG_ERROR << "Could not open socket !";
			return false;
		}

		NetworkAddress address;
		address.IP4(ip);
		address.Port(port);
		if (! cnx.Connect(address)) {
			LOG_ERROR << "Could not connect to server !";
			return false;
		}
		char login[] = "my_login";
		AuthInitPacket packet;
		std::strncpy(packet.login, login, LOGIN_FIELD_SIZE - 1);
		msg_hdr_data header;
		header.type_major = NET_MSG;
		header.type_minor = AUTH_REQUEST;
		header.length = 16;
		cnx.Send(reinterpret_cast<char*>(&header), sizeof(msg_hdr_data));
		cnx.Send(packet.login, 16);
		return true;
	}
}
