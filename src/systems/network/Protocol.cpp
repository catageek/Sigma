#include "systems/network/Protocol.h"
#include "Sigma.h"
#include "netport/include/net/network.h"
#include "composites/NetworkNode.h"

using namespace network;

namespace Sigma {
	void FrameObject::SendMessage(int fd, unsigned char major, unsigned char minor) {
		if(fd < 0) {
			return;
		}
		auto header = Header();
		header->type_major = major;
		header->type_minor = minor;
		FullFrame()->length = PacketSize() - sizeof(Frame_hdr);
		if (TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(FullFrame()), PacketSize()) <= 0) {
			LOG_ERROR << "could not send data" ;
		};
	}

	void FrameObject::SendMessage(id_t id, unsigned char major, unsigned char minor) {
		SendMessage(NetworkNode::GetFileDescriptor(id), major, minor);
	}
}
