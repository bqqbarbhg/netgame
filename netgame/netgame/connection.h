#ifndef _NETGAME_CONNECTION_H
#define _NETGAME_CONNECTION_H

#include <netlib/address.h>
#include <netlib/socket.h>

#include "protocol.h"
#include "channel.h"
#include "packet.h"

#include <map>

class SentPacket
{
public:
	SentPacket()
	{
	}
	SentPacket(Packet p, seq_t s)
		: packet(p)
		, seq(s)
	{
	}

	Packet packet;
	seq_t seq;
};

class Packet;
class Connection
{
public:
	Connection() { }
	Connection(const Address& addr, magic_t magic);
	Connection(Connection&& c);
	Connection& Connection::operator=(Connection c);

	// Call with every packet received with the associated socket
	bool process_packet(const Packet& packet);

	// Send packets (should be called with a fixed rate eg. 33 times a second)
	void send_outgoing(Socket& socket);

	// Send data to channel `chan`
	// Sent when you call `send_outgoing`
	void send(channel_id_t chan, const char* data, unsigned int size);

#ifdef _DEBUG
	void DEBUG_print_status();
	float m_DEBUG_packet_loss;
#endif

private:
	Connection(const Connection&);

	magic_t m_magic;

	Address m_address;

	std::map<channel_id_t, ChannelIn> m_channels_in;
	std::map<channel_id_t, ChannelOut> m_channels_out;

	seq_t m_sequence;

	seq_t m_out_ack;
	ack_bitfield_t m_out_ack_bits;

	std::map<seq_t, SentPacket> m_sent;
	
	PacketPool m_packet_pool;
};

#endif