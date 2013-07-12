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

	ChannelIn* get_channel_in_by_id(channel_id_t id) const;
	ChannelOut* get_channel_out_by_id(channel_id_t id) const;

#ifdef _DEBUG
	void DEBUG_print_status();
	float m_DEBUG_packet_loss;
#endif

private:
	Connection(const Connection&);

	void add_packet_header(NetWriter& w);
	unsigned int add_packet(NetWriter& w, channel_id_t ch, const Packet& p, unsigned int start, unsigned int parts);
	void send_packet(NetWriter& w);

	magic_t m_magic;

	Address m_address;

	std::map<channel_id_t, std::unique_ptr<ChannelIn>> m_channels_in;
	std::map<channel_id_t, std::unique_ptr<ChannelOut>> m_channels_out;

	seq_t m_sequence;

	seq_t m_out_ack;
	ack_bitfield_t m_out_ack_bits;

	std::map<seq_t, SentPacket> m_sent;
	
	PacketPool m_packet_pool;
};

#endif