#include "systems/network/TestPacketHandler.h"

#include "INetworkPacketHandler.h"
#include "systems/network/Protocol.h"
#include "composites/NetworkNode.h"

namespace Sigma {
	namespace network_packet_handler {
		template<>
		int INetworkPacketHandler::Process<TEST,TEST>() {
			auto req_list = GetQueue<TEST,TEST>()->Poll();
			if (!req_list) {
				return STOP;
			}
			for(auto& req : *req_list) {
				LOG_DEBUG << "Received authenticated test message with content: " << std::string(req->Content<TestPacket>()->message, 8) << " from id #" << NetworkNode::getEntityID(req->fd);
			}
			return CONTINUE;
		}
	}

	namespace reflection {
		template <> inline const char* GetNetworkHandler<TEST,TEST>(void) { return "TestPacketHandler"; }
	}

}
