#ifndef NETWORKCLIENT_H_INCLUDED
#define NETWORKCLIENT_H_INCLUDED

#include <memory>
#include "netport/include/net/network.h"

using namespace network;

namespace Sigma {
	struct Message;

	class NetworkClient {
	public:
		NetworkClient() {};
		virtual ~NetworkClient() {};

		bool Connect(const char *ip, unsigned short port);

		void SendMessage(unsigned char major, unsigned char minor, char* body, size_t len);

		std::unique_ptr<Message> RecvMessage();

		void WaitMessage();

	private:
		TCPConnection cnx;
	};
}

#endif // NETWORKCLIENT_H_INCLUDED
