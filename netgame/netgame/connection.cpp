#include "connection.h"
#include "packet.h"

#include <netlib/serialization.h>

Connection::Connection(const Address& addr, magic_t magic)
	: m_magic(magic)
	, m_address(addr)
	, m_sequence(0)
	, m_out_ack(0)
	, m_out_ack_bits(0)
{
}

bool Connection::process_packet(const Packet& packet)
{
	NetReader reader = packet.read();
	magic_t magic;

	// Do not process packets that don't start with the magic number
	if (!reader.read(magic))
		return false;
	if (magic != m_magic)
		return false;

	seq_t remote_seq;
	seq_t ack;
	ack_bitfield_t ack_bitfield;

	if (!reader.read(remote_seq)
	 || !reader.read(ack)
	 || !reader.read(ack_bitfield))
		return false;

	// Remove acknowledged packets from the re-send list
	for (int i = 0; i < ACKS_PER_BITFIELD + 1; i++) {
		if ((ack_bitfield & 1) == 0 && i != 0)
			continue;

		auto it = sent.find(ack);
		if (it != sent.end())
			sent.erase(it);

		ack--;
		if (i != 0)
			ack_bitfield >>= 1;
	}

	// Update the outgoing ack
	if (remote_seq > m_out_ack) {
		m_out_ack_bits <<= remote_seq - m_out_ack;
		m_out_ack_bits |= 1 << (remote_seq - m_out_ack - 1);
		m_out_ack = remote_seq;
	} else {
		m_out_ack_bits |= 1 << (m_out_ack - remote_seq - 1);
	}

	return true;
}

void Connection::send_outgoing(Socket& socket)
{
	Packet packet(MAX_PACKET_SIZE);
	NetWriter writer = packet.write();

	m_sequence++;

	// Write the header
	writer.write(m_magic);
	writer.write(m_sequence);
	writer.write(m_out_ack);
	writer.write(m_out_ack_bits);

	socket.send_to(m_address, writer.data(), writer.write_amount());

	sent[m_sequence] = SentPacket();
}

#include <cstdio>
void Connection::DEBUG_print_status()
{
	printf("%4d %8d %8d ", sent.size(), m_sequence, m_out_ack);
	for (int i = 0; i < ACKS_PER_BITFIELD; i++) {
		if (m_out_ack_bits & (1 << i))
			putchar('#');
		else
			putchar(' ');
	}
	printf("\n");
}