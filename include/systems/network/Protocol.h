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

	struct MessageObject {
		MessageObject(std::shared_ptr<msg_hdr>& header, std::shared_ptr<std::vector<unsigned char>>& body) : header(header), body(body) {};
		MessageObject(std::shared_ptr<msg_hdr>&& header, std::shared_ptr<std::vector<unsigned char>>&& body) : header(std::move(header)), body(std::move(body)) {};
		std::shared_ptr<msg_hdr> header;
		std::shared_ptr<std::vector<unsigned char>> body;
	};

	struct Frame {
		Frame_hdr fheader;
		msg_hdr mheader;
	};

	class FrameObject {
	public:
		FrameObject(int f) : fd(f), packet_size(0) {};
		FrameObject() : fd(-1), packet_size(0) {};

		template<class T>
		T* Content() {
			if(! data) {
				CreateBodySpace(sizeof(T));
			}
			return reinterpret_cast<T*>(Body());
		}

		void CreateFrameSpace(uint32_t len) {
			data.reset(new std::vector<char>(len));
			packet_size = len;
		};

		void CreateMessageSpace(uint32_t len) {
			data.reset(new std::vector<char>(len + sizeof(Frame_hdr)));
			packet_size = len + sizeof(Frame_hdr);
		};

		void CreateBodySpace(uint32_t len) {
			data.reset(new std::vector<char>(len + sizeof(Frame)));
			packet_size = len + sizeof(Frame);
		};

		uint32_t PacketSize() { return packet_size; };

		char* Body() { return ( data ? data->data() + sizeof(msg_hdr) + sizeof(Frame_hdr) : nullptr); };
		msg_hdr* Header() { return reinterpret_cast<msg_hdr*>((data ? data->data() + sizeof(Frame_hdr) : nullptr)); };
		Frame_hdr* Length() { return reinterpret_cast<Frame_hdr*>(data ? data->data() : nullptr); };

		int fd;
	private:
		std::shared_ptr<std::vector<char>> data;
		uint32_t packet_size;
	};
}

#endif // PROTOCOL_H_INCLUDED
