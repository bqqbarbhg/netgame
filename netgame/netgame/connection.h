#ifndef _NETGAME_CONNECTION_H
#define _NETGAME_CONNECTION_H

#include <netlib/address.h>
#include <netlib/socket.h>

#include "protocol.h"

#include <map>

class SentPacket
{
};

class Packet;
class Connection
{
public:
	Connection() { }
	Connection(const Address& addr, magic_t magic);

	bool process_packet(const Packet& packet);

	void send_outgoing(Socket& socket);

	void DEBUG_print_status();

private:

	seq_t m_sequence;

	seq_t m_out_ack;
	ack_bitfield_t m_out_ack_bits;

	std::map<seq_t, SentPacket> sent;
	magic_t m_magic;

	Address m_address;
};

#endif