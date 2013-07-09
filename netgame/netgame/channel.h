#ifndef _NETGAME_CHANNEL_H
#define _NETGAME_CHANNEL_H

#include <deque>
#include <utility>

#include "protocol.h"
#include "packet.h"

class Channel
{
public:
	enum Type
	{
		// Not usable
		UNKNOWN = 0,
		// Send and receive as-is
		RAW = 1,
		// Receive only the newest packet
		NEWEST = 2,
		// Make sure the client receives every packet sent (not in order)
		RELIABLE = 3,
		// Make sure the client receives every packet in the order they are sent
		SEQUENTIAL = 4,
	};

	Channel()
		: m_type(UNKNOWN)
	{ }
	explicit Channel(Type t)
		: m_type(t)
	{ }

protected:
	Type m_type;
};
class ChannelIn : public Channel
{
public:
	ChannelIn()
		: Channel()
	{ }
	explicit ChannelIn(Type t)
		: Channel(t)
	{ }

	// Add a packet to the received queue with the sequnece number `seq`
	void add_to_queue(seq_t seq, Packet packet);

	// If there is a packet to receive pop and return it
	// Else return an empty packet
	Packet get_next();
private:
	seq_t m_last_read;
	std::deque<std::pair<seq_t, Packet>> m_received;
};
class ChannelOut : public Channel
{
public:
	ChannelOut()
		: Channel()
	{ }
	explicit ChannelOut(Type t)
		: Channel(t)
	{ }

	seq_t sequence;
};

#endif