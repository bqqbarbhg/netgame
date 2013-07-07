#include "packet.h"
#include <utility>

Packet::Packet()
	: m_buffer(nullptr)
	, m_start(0)
	, m_size(0)
{
}
Packet::Packet(char *buf, unsigned int start, unsigned int size)
	: m_buffer(buf)
	, m_start(start)
	, m_size(size)
{
	if (m_buffer != nullptr)
		++*reinterpret_cast<unsigned int*>(m_buffer);
}
Packet::Packet(const Packet& p)
	: m_buffer(p.m_buffer)
	, m_start(p.m_start)
	, m_size(p.m_size)
{
	// Increase refcount
	if (m_buffer != nullptr)
		++*reinterpret_cast<unsigned int*>(m_buffer);
}
Packet::Packet(Packet&& p)
	: m_buffer(p.m_buffer)
	, m_start(p.m_start)
	, m_size(p.m_size)
{
	p.m_buffer = nullptr;
}
Packet& Packet::operator=(Packet p)
{
	std::swap(m_buffer, p.m_buffer),
	m_start = p.m_start;
	m_size = p.m_size;
	return *this;
}
Packet::~Packet()
{
	// Decrease refcount
	if (m_buffer != nullptr)
		--*reinterpret_cast<unsigned int*>(m_buffer);
}
Packet Packet::subpacket(unsigned int start, unsigned int size) const
{
	return Packet(m_buffer, m_start + start, size);
}

// Pages contain a refcount block before the data
#define PAGE_HEADER_SIZE (sizeof(unsigned int))

PacketPool::PacketPool()
{
}
PacketPool::PacketPool(PacketPool&& p)
	: m_pages(std::move(p.m_pages))
	, m_alloc_page(p.m_alloc_page)
	, m_alloc_ptr(p.m_alloc_ptr)
	, m_page_size(p.m_page_size)
	, m_packet_size(p.m_packet_size)
{
}
PacketPool& PacketPool::operator=(PacketPool p)
{
	m_pages.swap(p.m_pages);
	m_alloc_page = p.m_alloc_page;
	m_alloc_ptr = p.m_alloc_ptr;
	m_page_size = p.m_page_size;
	m_packet_size = p.m_packet_size;

	return *this;
}
PacketPool::PacketPool(unsigned int maxPacketSize)
	: m_alloc_page(0)
	, m_alloc_ptr(PAGE_HEADER_SIZE)
	, m_page_size(maxPacketSize * 10 + PAGE_HEADER_SIZE)
	, m_packet_size(maxPacketSize)
{
	add_page();
}

void PacketPool::add_page()
{
	m_pages.emplace_back(new char[m_page_size]);
	*reinterpret_cast<unsigned int*>(m_pages.back().get()) = 0;
}

Packet PacketPool::allocate(int size)
{
	if (size <= 0)
		return Packet();
	Packet ret(m_pages[m_alloc_page].get(), m_alloc_ptr, size);

	// Advance the allocation location
	m_alloc_ptr += size;
	// If there is not enough room in the current page
	if (m_alloc_ptr >= m_page_size - m_packet_size) {
		// Try to find an empty one
		unsigned int i;
		for (i = 0; i < m_pages.size(); i++) {
			if (*reinterpret_cast<unsigned int*>(m_pages[i].get()) == 0) {
				break;
			}
		}
		m_alloc_page = i;
		m_alloc_ptr = PAGE_HEADER_SIZE;
		// If the page doesn't exist create it
		if (i == m_pages.size()) {
			add_page();
		}
	}

	return ret;
}