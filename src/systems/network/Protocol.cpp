#include "systems/network/Protocol.h"
#include "Sigma.h"
#include "netport/include/net/network.h"

using namespace network;

namespace Sigma {
	void FrameObject::SendMessage(int fd, unsigned char major, unsigned char minor) const {
		auto header = Header();
		header->type_major = major;
		header->type_minor = minor;
		Length()->length = PacketSize() - sizeof(Frame_hdr);
		if (TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(Length()), PacketSize()) <= 0) {
			LOG_ERROR << "could not send data" ;
		};
	}
}
