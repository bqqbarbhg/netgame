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

// Type that can hold all fragment indices
typedef uint8_t fragment_id_t;

typedef uint32_t fragment_bitfield_t;

// Can hold every count of messages
typedef uint8_t message_count_t;

// Can hold every size of a message sent over a channel
typedef uint16_t message_size_t;

// Maxium size of a packet to send
const unsigned int MAX_PACKET_SIZE = 512;

// Packet header size
const unsigned int HEADER_SIZE = sizeof(magic_t) + 2 * sizeof(seq_t) + sizeof(ack_bitfield_t) + sizeof(message_count_t);

// Message header size
const unsigned int MSG_HEADER_SIZE = sizeof(channel_id_t) + sizeof(seq_t) + sizeof(message_size_t) + sizeof(fragment_id_t);

// Fragmented message header size
const unsigned int FRAG_MSG_HEADER_SIZE = MSG_HEADER_SIZE + sizeof(fragment_id_t) + sizeof(message_size_t);

// Number of bits in a `ack_bitfield_t`
const unsigned int ACKS_PER_BITFIELD = sizeof(ack_bitfield_t) * CHAR_BIT;

// How many fragments can one bitfield contain
const unsigned int FRAGMENTS_PER_BITFIELD = sizeof(fragment_bitfield_t) * CHAR_BIT;

// Maximum size of an unfragmented message sent through a channel
const unsigned int MAX_UNFRAGMENTED_MESSAGE_SIZE = (MAX_PACKET_SIZE - HEADER_SIZE - FRAG_MSG_HEADER_SIZE);

// Maximum size of a message sent through a channel
const unsigned int MAX_MESSAGE_SIZE = MAX_UNFRAGMENTED_MESSAGE_SIZE * FRAGMENTS_PER_BITFIELD;

#endif