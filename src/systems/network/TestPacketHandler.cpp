#include "systems/network/TestPacketHandler.h"

#include "composites/NetworkNode.h"
#include "systems/network/NetworkPacketHandler.hpp"
#include "systems/network/Protocol.h"

namespace Sigma {
	namespace network_packet_handler {
		template<>
		template<>
		void INetworkPacketHandler<SERVER>::Process<TEST,TEST>() {
			auto req_list = GetQueue<TEST,TEST>()->Poll();
			if (req_list.empty()) {
				return;
			}
			for(auto& req : req_list) {
				LOG_DEBUG << "Received authenticated test message with content: " << std::string(req->Content<TestPacket>()->message) << " from id #" << req->GetId();
				Sigma::FrameObject packet{};
				packet << std::string(req->Content<TestPacket>()->message);
				packet.SendMessage(req->GetId(), TEST, TEST);
			}
		}

		template<>
		template<>
		void INetworkPacketHandler<CLIENT>::Process<TEST,TEST>() {
			auto req_list = GetQueue<TEST,TEST>()->Poll();
			if (req_list.empty()) {
				return;
			}
			for(auto& req : req_list) {
				LOG_DEBUG << "Received authenticated test message with content: " << std::string(req->Content<TestPacket>()->message);
			}
		}
	}

	namespace reflection {
		template <> inline const char* GetNetworkHandler<TEST,TEST>(void) { return "TestPacketHandler"; }
	}

}
