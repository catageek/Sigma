#include "systems/network/TestPacketHandler.h"

#include "systems/network/NetworkPacketHandler.hpp"
#include "systems/network/Protocol.h"
#include "composites/NetworkNode.h"

namespace Sigma {
	namespace network_packet_handler {
		template<>
		void INetworkPacketHandler::Process<TEST,TEST>() {
			auto req_list = GetQueue<TEST,TEST>()->Poll();
			if (!req_list) {
				return;
			}
			for(auto& req : *req_list) {
				LOG_DEBUG << "Received authenticated test message with content: " << std::string(req->Content<TestPacket>()->message, 8) << " from id #" << req->GetId();
			}
		}
	}

	namespace reflection {
		template <> inline const char* GetNetworkHandler<TEST,TEST>(void) { return "TestPacketHandler"; }
	}

}
