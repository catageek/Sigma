#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#include <cstdint>
#include <memory>
#include <vector>

// We are little endian !!!

// Version MAJOR codes
// unauthenticated messages
#define NET_MSG 		1		// low level messages for authentication and disconnect
#define SERVER_MSG		2		// Server public information
#define PUBPLAYER_MSG	3		// Player public profile
#define	DLBINARY_MSG	4		// for binary download
#define	ASSETS_MSG		5		// for public assets download
#define	RESERVED_6		6
#define	RESERVED_7		7
// authenticated messages
#define	PROFILE_MSG		8		// player private profile
#define WORLD_MSG		9		// world data
#define GAME_MSG		10		// game data
#define SOCIAL_MSG		11		// chat/mail
#define PASSETS_MSG		11		// private assets
#define CPU_MSG			12		// for CPU...

#define IS_RESTRICTED(x)	((x >> 3) != 0)

namespace Sigma {
	struct msg_hdr {
		uint32_t flags;
		unsigned char type_major;
		unsigned char type_minor;
		char padding[2];
	};

	struct Frame_hdr {
		uint32_t length;
	};

	struct Frame {
		Frame_hdr fheader;
		msg_hdr mheader;
	};

	class FrameObject {
	public:
		template<class T=Frame_hdr>
		FrameObject(int fd = -1) : fd(fd), packet_size(sizeof(T)), data(std::vector<char>(sizeof(T))) {};

		void SendMessage(int fd, unsigned char major, unsigned char minor);

		void Resize(size_t new_size) {
			packet_size = new_size + sizeof(Frame);
			data.resize(packet_size);
		}

		template<class T, class U=T>
		U* Content() {
			if(packet_size < sizeof(T) + sizeof(Frame)) {
				Resize(sizeof(T));
			}
			return reinterpret_cast<U*>(Body());
		}

		uint32_t PacketSize() const { return packet_size; };

		msg_hdr* Header() { return reinterpret_cast<msg_hdr*>((data.size() ? data.data() + sizeof(Frame_hdr) : nullptr)); };
		Frame_hdr* FullFrame() { return reinterpret_cast<Frame_hdr*>(data.size() ? data.data() : nullptr); };

		int fd;
	private:
		char* Body() { return ( data.size() ? data.data() + sizeof(msg_hdr) + sizeof(Frame_hdr) : nullptr); };

		std::vector<char> data;
		uint32_t packet_size;
	};
}

#endif // PROTOCOL_H_INCLUDED
