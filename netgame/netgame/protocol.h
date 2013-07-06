#ifndef _NETGAME_PROTOCOL_H
#define _NETGAME_PROTOCOL_H

#include <climits>
#include <cstdint>

typedef uint32_t magic_t;
typedef uint32_t seq_t;
typedef uint32_t ack_bitfield_t;

enum {
	ACKS_PER_BITFIELD = sizeof(ack_bitfield_t) * CHAR_BIT,
};

enum {
	MAX_PACKET_SIZE = 1024 * 1024,
};

#endif