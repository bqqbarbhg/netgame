#include "channel.h"
#include <algorithm>
#include <utility>
#include "util.h"

ChannelIn::PendingPacket::PendingPacket(fragment_bitfield_t frags, seq_t seq, Packet&& p)
	: frag_need(frags)
	, seq(seq)
	, packet(std::move(p))
{ }

ChannelIn::PendingPacket::PendingPacket(PendingPacket&& p)
	: frag_need(p.frag_need)
	, seq(p.seq)
	, packet(std::move(p.packet))
{
}

ChannelIn::PendingPacket& ChannelIn::PendingPacket::operator=(PendingPacket p)
{
	frag_need = p.frag_need;
	seq = p.seq;
	std::swap(packet, p.packet);
	return *this;
}

ChannelIn::PendingPacket* ChannelIn::add_packet(seq_t seq, Packet packet, fragment_bitfield_t frags)
{
	auto pending = PendingPacket(frags, seq, std::move(packet));

	if (m_type == RAW) {
		// Remove too old packets (if raw)
		if (m_received.size() > 32) {
			m_last_read = m_received.front().seq;
			m_received.pop_front();
		}
	}
	// Don't add duplicate packets
	if (pending.seq <= m_last_read)
		return nullptr;

	if (m_received.empty() || pending.seq > m_received.back().seq) {
		// Always push if empty or newer than the last
		m_received.push_back(std::move(pending));
		return &m_received.back();
	} else {
		// Else insert to the correct position in the queue (if new)
		auto pos = std::lower_bound(m_received.begin(), m_received.end(), pending,
			[](const decltype(pending)& a, const decltype(pending)& b) {
				return a.seq < b.seq;
			});
		NETGAME_ASSERT(pos != m_received.end());
		if (pos->seq != pending.seq)
			m_received.insert(pos, std::move(pending));
		else
			return nullptr;
		return &*pos;
	}
}

void ChannelIn::add_fragment(seq_t seq, const Packet& packet, fragment_id_t fragId, fragment_id_t fragCount, unsigned int startIndex, message_size_t msgSize)
{
	// Find the pending packet that misses the fragment
	auto pos = std::find_if(m_received.begin(), m_received.end(),
		[=](const PendingPacket& p){ return p.seq == seq; });

	PendingPacket *pending = nullptr;
	if (pos != m_received.end()) {
		// If the packet is pending copy to it
		pending = &*pos;
	} else {
		// Else try to create a new packet
		pending = add_packet(seq, fragPool.allocate(msgSize), (~(fragment_bitfield_t)0 >> (FRAGMENTS_PER_BITFIELD - fragCount)) & ~(1 << fragId));
	}
	
	// Extend a packet if found and is not consumed
	if (pending != nullptr && !pending->packet.empty()) {
		fragment_bitfield_t bit = 1 << fragId;
		// If the fragment is still missing
		if (pending->frag_need & bit) {
			// Clear the filled bit
			pending->frag_need &= ~bit;
			// Copy the fragment data to the buffer
			memcpy(pending->packet.data() + startIndex, packet.data(), packet.size());
		}
	}
}

Packet ChannelIn::get_next()
{
	if (m_received.empty())
		return Packet();
	Packet ret;
	switch (m_type) {
	case UNKNOWN:
		break;
	case RAW:
		// Receive all complete packets
		for (auto& pkt : m_received) {
			if (!pkt.frag_need && !pkt.packet.empty()) {
				ret = pkt.packet;
				pkt.packet = Packet();
				break;
			}
		}
		break;
	case NEWEST:
		{
		// Receive the newest packet
		typedef decltype(m_received) ct;
		ct::reverse_iterator it;
		for (it = m_received.rbegin(); it != m_received.rend(); ++it) {
			if (!it->frag_need && !it->packet.empty() && it->seq > m_last_read) {
				m_last_read = it->seq;
				ret = it->packet;
				break;
			}
		}
		// Remove older packets
		m_received.erase(m_received.begin(), it.base());
		} break;
	case RELIABLE:
		// Find the first non-empty packet
		for (auto& pkt : m_received) {
			if (!pkt.frag_need && !pkt.packet.empty()) {
				ret = pkt.packet;
				pkt.packet = Packet();
				break;
			}
		}
		// Remove the read packets from the front
		while (m_received.front().packet.empty() && m_received.front().seq == m_last_read + 1) {
			m_received.pop_front();
			m_last_read++;
		}
		break;
	case SEQUENTIAL:
		// Receive only if complete and it's the next packet in the queue
		if (!m_received.front().frag_need && m_received.front().seq == m_last_read + 1) {
			ret = m_received.front().packet;
			m_received.pop_front();
			m_last_read++;
		}	
		break;
	}
	return ret;
}