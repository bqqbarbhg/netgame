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

	bool is_reliable() const {
		return m_type == RELIABLE || m_type == SEQUENTIAL;
	}

protected:
	Type m_type;
};
class ChannelIn : public Channel
{
public:
	ChannelIn(PacketPool& pool)
		: Channel()
		, fragPool(pool)
	{ }
	explicit ChannelIn(PacketPool& pool, Type t)
		: Channel(t)
		, fragPool(pool)
	{ }

	struct PendingPacket
	{
		PendingPacket(fragment_bitfield_t frags, seq_t seq, Packet&& p);
		PendingPacket(PendingPacket&& p);
		PendingPacket& operator=(PendingPacket p);

		fragment_bitfield_t frag_need;
		seq_t seq;
		Packet packet;
	};

	// Add a packet to the received queue with the sequnece number `seq`
	// Optional fragment data
	// Returns the address of the packet if created (nullptr otherwise)
	PendingPacket* add_packet(seq_t seq, Packet packet, fragment_bitfield_t frags=0);

	void add_fragment(seq_t seq, const Packet& packet, fragment_id_t fragId, fragment_id_t fragCount, unsigned int startIndex, message_size_t msgSize);

	// If there is a packet to receive pop and return it
	// Else return an empty packet
	Packet receive();
private:
	seq_t m_last_read;
	std::deque<PendingPacket> m_received;
	PacketPool& fragPool;
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

	struct OutgoingPacket
	{
		OutgoingPacket(seq_t seq, Packet&& p);
		OutgoingPacket(OutgoingPacket&& p);
		OutgoingPacket& operator=(OutgoingPacket p);

		seq_t seq;
		Packet packet;
	};

	void send(Packet packet);
private:
	friend class Connection;
	std::vector<OutgoingPacket> m_outgoing;
	seq_t m_seq;
};

#endif