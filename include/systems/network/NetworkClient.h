#ifndef NETWORKCLIENT_H_INCLUDED
#define NETWORKCLIENT_H_INCLUDED

#include <memory>
#include "netport/include/net/network.h"
#include "composites/VMAC_Checker.h"

using namespace network;

namespace Sigma {
	class FrameObject;

	class NetworkClient {
	public:
		NetworkClient() : auth_state(0) {};
		virtual ~NetworkClient() {};

		void Start();

		bool Connect(const char *ip, unsigned short port);

		void SendMessage(unsigned char major, unsigned char minor, FrameObject& packet);
		void SendUnauthenticatedMessage(unsigned char major, unsigned char minor, FrameObject& packet);

		std::unique_ptr<FrameObject> RecvMessage();

		void Authenticate();

	private:
		uint32_t auth_state;
		TCPConnection cnx;
		std::unique_ptr<cryptography::VMAC_StreamHasher> hasher;
	};
}

#endif // NETWORKCLIENT_H_INCLUDED
