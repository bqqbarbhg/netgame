#include "connection.h"
#include "packet.h"

#include <netlib/serialization.h>

Connection::Connection(const Address& addr, magic_t magic)
	: m_magic(magic)
	, m_address(addr)
	, m_sequence(0)
	, m_out_ack(0)
	, m_out_ack_bits(0)
	, m_packet_pool(MAX_PACKET_SIZE)
#ifdef _DEBUG
	, m_DEBUG_packet_loss(0.0f)
#endif
{
	// Create control channels
	m_channels_in[0] = ChannelIn(Channel::SEQUENTIAL);
	m_channels_out[0] = ChannelOut(Channel::SEQUENTIAL);
}

// = default
Connection::Connection(Connection&& c)
	: m_magic(c.m_magic)
	, m_address(std::move(c.m_address))
	, m_channels_in(std::move(c.m_channels_in))
	, m_channels_out(std::move(c.m_channels_out))
	, m_sequence(c.m_sequence)
	, m_out_ack(c.m_out_ack)
	, m_out_ack_bits(c.m_out_ack_bits)
	, m_sent(std::move(m_sent))
	, m_packet_pool(std::move(m_packet_pool))
{
}

Connection& Connection::operator=(Connection c)
{
	std::swap(m_magic, c.m_magic);
	std::swap(m_address, c.m_address);
	m_channels_in.swap(c.m_channels_in);
	m_channels_out.swap(c.m_channels_out);
	std::swap(m_sequence, c.m_sequence);
	std::swap(m_out_ack, c.m_out_ack);
	std::swap(m_out_ack_bits, c.m_out_ack_bits);
	m_sent.swap(c.m_sent);
	std::swap(m_packet_pool, c.m_packet_pool);

	return *this;
}

bool Connection::process_packet(const Packet& packet)
{
	// Ignore the packet if simulating packet loss
#ifdef _DEBUG
	if ((float)rand() / RAND_MAX < m_DEBUG_packet_loss)
		return false;
#endif

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

		auto it = m_sent.find(ack);
		if (it != m_sent.end())
			m_sent.erase(it);

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
	NetWriter writer(m_packet_pool.nextData(), m_packet_pool.nextSize());

	m_sequence++;

	// Write the header
	writer.write(m_magic);
	writer.write(m_sequence);
	writer.write(m_out_ack);
	writer.write(m_out_ack_bits);

	for (auto it = m_sent.begin(); it != m_sent.end();)
	{
		if (m_sequence - it->first > 30)
			it = m_sent.erase(it);
		else
			++it;
	}

	Packet packet = m_packet_pool.allocate(writer.write_amount());

	// Don't send the packet if simulating packet loss
#ifdef _DEBUG
	if ((float)rand() / RAND_MAX >= m_DEBUG_packet_loss)
#endif
	socket.send_to(m_address, packet.data(), packet.size());

	m_sent[m_sequence] = SentPacket(packet, m_sequence);
}

#include <cstdio>
void Connection::DEBUG_print_status()
{
	printf("%4d %4d %8d %8d ", m_packet_pool.numPages(), m_sent.size(), m_sequence, m_out_ack);
	for (int i = 0; i < ACKS_PER_BITFIELD; i++) {
		if (m_out_ack_bits & (1 << i))
			putchar('#');
		else
			putchar(' ');
	}
	printf("\n");
}