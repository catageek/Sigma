#ifndef NETWORKPACKETHANDLER_HPP_INCLUDED
#define NETWORKPACKETHANDLER_HPP_INCLUDED

#include "AtomicQueue.hpp"
#include "Log.h"
#include "systems/network/Network.h"

namespace Sigma {

	class FrameObject;

	namespace network_packet_handler {

		template<int Major,int Minor, TagType T>
		class PacketQueue {
		public:
			static AtomicQueue<std::shared_ptr<FrameObject>> input_queue;
		};

		template<int Major,int Minor, TagType T>
		AtomicQueue<std::shared_ptr<FrameObject>> PacketQueue<Major,Minor, T>::input_queue;

        template<TagType T>
		class INetworkPacketHandler {
		public:
			template<int Major, int Minor>
			static void Process() {
				LOG_DEBUG << "parent process here...";
			};

			template<int Major, int Minor>
			static const AtomicQueue<std::shared_ptr<FrameObject>>* const GetQueue() { return &PacketQueue<Major,Minor, T>::input_queue; };

		};
	} // namespace network_packet_handler
} // namespace Sigma

#endif // NETWORKPACKETHANDLER_HPP_INCLUDED
