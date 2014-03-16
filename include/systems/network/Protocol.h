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
		uint32_t length;
		uint32_t flags;
		unsigned char type_major;
		unsigned char type_minor;
		char padding[2];
	};

	struct Message {
		Message(std::shared_ptr<msg_hdr>& header, std::shared_ptr<std::vector<char>>& body) : header(header), body(body) {};
		Message(std::shared_ptr<msg_hdr>&& header, std::shared_ptr<std::vector<char>>&& body) : header(std::move(header)), body(std::move(body)) {};
		std::shared_ptr<msg_hdr> header;
		std::shared_ptr<std::vector<char>> body;
	};

	struct Frame {
		Frame(int f) : fd(f), length(0) {};

		int fd;
		std::shared_ptr<std::vector<char>> data;
		uint32_t length;
	};
}

#endif // PROTOCOL_H_INCLUDED
