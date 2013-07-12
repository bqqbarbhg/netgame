#include "connection.h"
#include "packet.h"

#include <utility>
#include <algorithm>
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
	m_channels_in[0].reset(new ChannelIn(Channel::SEQUENTIAL));
	m_channels_out[0].reset(new ChannelOut(Channel::SEQUENTIAL));
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

struct SendPacket
{
	SendPacket(channel_id_t ch, ChannelOut::OutgoingPacket&& p)
		: chan(ch)
		, seq(p.seq)
		, packet(std::move(p.packet))
	{
	}
	SendPacket(SendPacket&& p)
		: seq(p.seq)
		, packet(std::move(p.packet))
		, chan(p.chan)
	{
	}
	SendPacket& operator=(SendPacket p)
	{
		seq = p.seq;
		std::swap(packet, p.packet);
		chan = p.chan;
		return *this;
	}

	bool operator <(unsigned int u) const {
		return packet.size() < u;
	}

	seq_t seq;
	Packet packet;
	channel_id_t chan;
};

inline bool can_fit_any(unsigned int offset)
{
	// Can fit at least one byte
	return offset + MSG_HEADER_SIZE < MAX_PACKET_SIZE - 1;
}

inline unsigned int fragment_count(unsigned int offset, unsigned int size)
{
	return (size - (MAX_PACKET_SIZE - MSG_HEADER_SIZE - offset)) / MAX_UNFRAGMENTED_MESSAGE_SIZE + 1;
}

void Connection::send_outgoing(Socket& socket)
{
	// Collect all packets to send
	std::vector<SendPacket> send_reliable;
	std::vector<SendPacket> send_unreliable;
	for (auto& chan : m_channels_out)
	{
		auto& src = chan.second->m_outgoing;
		auto& dst = chan.second->is_reliable() ? send_reliable : send_unreliable;
		for (auto& packet : src) {
			dst.push_back(SendPacket(chan.first, std::move(packet)));
		}
		src.clear();
	}

	// Sort by size (descending)
	std::sort(send_reliable.begin(), send_reliable.end(),
		[](const ChannelOut::OutgoingPacket& a,
		   const ChannelOut::OutgoingPacket& b) {
			   return a.packet.size() > b.packet.size();
	});

	NetWriter writer(m_packet_pool.nextData(), MAX_PACKET_SIZE);

	add_packet_header(writer);

	// Packet fill algorithm
	// P := largest packet
	// while (true)
	//      while (P overflows)
	//        send part
	//      append P
	//      while (exists packets that won't overflow)
	//        append largest of them
	//      if (exists packets that would be less fragmented if started here)
	//        P := largest of them
	//      else
	//        send
	//        if (no more packets)
	//          break
	//        else
	//          P := largest packet

	std::vector<SendPacket>::iterator P;

	P = send_reliable.begin();
	while (!send_reliable.empty()) {
		unsigned int fragc = fragment_count(writer.write_amount(), P->packet.size());
		unsigned int start = 0;
		for (unsigned int i = 0; i < fragc - 1; i++) {
			start += add_packet(writer, P->chan, P->packet, start, fragc);
			send_packet(writer);
		}
		add_packet(writer, P->chan, P->packet, start, fragc);
		send_reliable.erase(P);

		auto pos = writer.write_amount();
		while (can_fit_any(pos)) {
			auto it = std::find_if(send_reliable.begin(), send_reliable.end(),
				[=](const SendPacket& p) {
					return fragment_count(pos, p.packet.size()) == 1;
			});
			if (it != send_reliable.end()) {
				add_packet(writer, it->chan, it->packet, 0, 1);
				send_reliable.erase(it);
			} else {
				break;
			}
			pos = writer.write_amount();
		}

		if (can_fit_any(pos)) {
			auto it = std::find_if(send_reliable.begin(), send_reliable.end(),
				[=](const SendPacket& p) {
					return fragment_count(pos, p.packet.size()) < fragment_count(0, p.packet.size());
			});
			if (it != send_reliable.end()) {
				P = it;
				continue;
			}
		}

		send_packet(writer);
		P = send_reliable.begin();
	}

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

void Connection::add_packet_header(NetWriter& w)
{
	w.write(m_magic);
	w.write(m_sequence);
	w.write(m_out_ack);
	w.write(m_out_ack_bits);
}

ChannelIn *Connection::get_channel_in_by_id(channel_id_t id) const
{
	auto it = m_channels_in.find(id);
	if (it == m_channels_in.end())
		return nullptr;
	return it->second.get();
}

ChannelOut *Connection::get_channel_out_by_id(channel_id_t id) const
{
	auto it = m_channels_out.find(id);
	if (it == m_channels_out.end())
		return nullptr;
	return it->second.get();
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