#ifndef NETWORKCLIENT_H_INCLUDED
#define NETWORKCLIENT_H_INCLUDED

#include "netport/include/net/network.h"

using namespace network;

namespace Sigma {
	class NetworkClient {
	public:
		NetworkClient() {};
		virtual ~NetworkClient() {};

		bool Connect(const char *ip, unsigned short port);


	private:
		TCPConnection cnx;
	};
}

#endif // NETWORKCLIENT_H_INCLUDED
