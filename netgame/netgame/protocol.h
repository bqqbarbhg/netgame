#ifndef _NETGAME_PROTOCOL_H
#define _NETGAME_PROTOCOL_H

#include <climits>
#include <cstdint>

// Type of the magic number in the packet header
typedef uint32_t magic_t;

// Sequence number type
typedef uint32_t seq_t;

// Bitfield type to use to acknowledge past packets (one ack per bit)
typedef uint32_t ack_bitfield_t;

// Type that can hold all channel numbers
typedef uint16_t channel_id_t;

enum {
	// Number of bits in a `ack_bitfield_t`
	ACKS_PER_BITFIELD = sizeof(ack_bitfield_t) * CHAR_BIT,

	// Maxium size of a packet to send
	MAX_PACKET_SIZE = 64,
};

#endif