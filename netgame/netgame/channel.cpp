#include "channel.h"
#include <algorithm>
#include <utility>
#include "util.h"

void ChannelIn::add_to_queue(seq_t seq, Packet packet)
{
	auto pair = std::make_pair(seq, std::move(packet));

	switch (m_type) {
	case UNKNOWN:
		break;
	case RAW:
		// Always push back to the queue
		m_received.push_back(std::move(pair));
		break;
	case NEWEST:
		// Push only if emtpy, the only element is the newest
		if (m_received.empty()) {
			m_received.push_back(std::move(pair));
		} else if (m_received.front().first < pair.first) {
			m_received.front() = std::move(pair);
		}
	case RELIABLE:
	case SEQUENTIAL:
		// Don't add duplicate packets
		if (pair.first <= m_last_read)
			return;
		
		if (m_received.empty()) {
			// Always push if empty (and newer than last read)
			m_received.push_back(std::move(pair));
		} else if (pair.first > m_received.back().first) {
			// If this is the newest packet push back (usual case)
			m_received.push_back(std::move(pair));
		} else {
			// Else insert to the correct position in the queue (if new)
			auto pos = std::lower_bound(m_received.begin(), m_received.end(), pair,
				[](const decltype(pair)& a, const decltype(pair)& b) {
					return a.first < b.first;
				});
			// TODO: Check lower_bound semantics
			NETGAME_ASSERT(pos != m_received.end());
			if (pos->first != pair.first)
				m_received.insert(pos, std::move(pair));
		}
		break;
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
	case NEWEST:
		// Receive always
		ret = m_received.front().second;
		m_received.pop_front();
		break;
	case RELIABLE:
		// Find the first non-empty packet
		for (auto& pair : m_received) {
			if (!pair.second.empty()) {
				ret = pair.second;
				pair.second = Packet();
				break;
			}
		}
		// Remove the read packets from the front
		while (m_received.front().second.empty() && m_received.front().first == m_last_read + 1) {
			m_received.pop_front();
			m_last_read++;
		}
		break;
	case SEQUENTIAL:
		// Receive only if it's the next packet in the queue
		if (m_received.front().first == m_last_read + 1) {
			ret = m_received.front().second;
			m_received.pop_front();
			m_last_read++;
		}	
		break;
	}
	return ret;
}