#include "systems/network/TestPacketHandler.h"

#include <cstring>
#include "composites/NetworkNode.h"
#include "systems/network/NetworkPacketHandler.hpp"
#include "systems/network/Protocol.h"

namespace Sigma {
	namespace network_packet_handler {
		template<>
		void INetworkPacketHandler::Process<TEST,TEST, false>() {
			auto req_list = GetQueue<TEST,TEST>()->Poll();
			if (!req_list) {
				return;
			}
			for(auto& req : *req_list) {
				LOG_DEBUG << "Received authenticated test message with content: " << std::string(req->Content<TestPacket>()->message, 8) << " from id #" << req->GetId();
				Sigma::FrameObject packet{};
				std::strcpy(packet.Content<Sigma::TestPacket>()->message, std::string("trillekreply").c_str());
				packet.SendMessage(req->GetId(), TEST, TEST);
			}
		}

		template<>
		void INetworkPacketHandler::Process<TEST,TEST, true>() {
			auto req_list = GetQueue<TEST,TEST>()->Poll();
			if (!req_list) {
				return;
			}
			for(auto& req : *req_list) {
				LOG_DEBUG << "Received authenticated test message with content: " << std::string(req->Content<TestPacket>()->message, 8);
			}
		}
	}

	namespace reflection {
		template <> inline const char* GetNetworkHandler<TEST,TEST>(void) { return "TestPacketHandler"; }
	}

}
