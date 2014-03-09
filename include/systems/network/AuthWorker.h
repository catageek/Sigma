#ifndef AUTHWORKER_H_INCLUDED
#define AUTHWORKER_H_INCLUDED

#include "systems/network/AuthenticationPacket.h"
#include "systems/network/NetworkSystem.h"
#include "netport/include/net/network.h"

using namespace network;

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

		void Work() {
			auto req_list = NetworkSystem::GetAuthRequestQueue()->Poll();
			for (auto req : *req_list) {
				AuthenticationPacket packet;
				auto len = TCPConnection(req, NETA_IPv4, SCS_CONNECTED).Recv(packet.buffer, 32);
				LOG_DEBUG << "received " << len << " bytes";
				if (! Authenticate(&packet)) {
					// close coonection
					LOG_DEBUG << "Authentication failed";
				}
				LOG_DEBUG << "Authentication OK";
			}
		}
	};
}

#endif // AUTHWORKER_H_INCLUDED
