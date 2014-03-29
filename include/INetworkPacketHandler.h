#ifndef INETWORKPACKETHANDLER_H_INCLUDED
#define INETWORKPACKETHANDLER_H_INCLUDED

#include "systems/network/AtomicQueue.hpp"
#include "Log.h"

namespace Sigma {

	class FrameObject;

	namespace network_packet_handler {

		template<int Major,int Minor>
		class PacketQueue {
		public:
			static AtomicQueue<std::shared_ptr<FrameObject>> input_queue;
		};

		template<int Major,int Minor>
		AtomicQueue<std::shared_ptr<FrameObject>> PacketQueue<Major,Minor>::input_queue;

		class INetworkPacketHandler {
		public:
			template<int Major, int Minor>
			static int Process() {
				LOG_DEBUG << "parent process here...";
			};

			template<int Major, int Minor>
			static AtomicQueue<std::shared_ptr<FrameObject>>* GetQueue() { return &PacketQueue<Major,Minor>::input_queue; };

		};
	} // namespace network_packet_handler
} // namespace Sigma

#endif // INETWORKPACKETHANDLER_H_INCLUDED
