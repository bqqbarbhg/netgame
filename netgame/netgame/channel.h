#ifndef _NETGAME_CHANNEL_H
#define _NETGAME_CHANNEL_H

#include <map>

#include "protocol.h"

class ReceivedPacket
{
};

class Channel
{
public:
	enum Type
	{
		// Not usable
		UNKNOWN = 0,
		// Send and receive as-is
		RAW = 1,
		// Receive only if the packet is newer than the one received before
		NEWEST = 2,
		// Make sure the client receives every packet sent (not in order)
		RELIABLE = 3,
		// Make sure the client receives every packet in the order they are sent
		SEQUENTIAL = 4,
	};

	Channel()
		: type(UNKNOWN)
	{ }
	explicit Channel(Type t)
		: type(t)
	{ }

	Type type;
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

	seq_t last_received;
	std::map<seq_t, ReceivedPacket> received;
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