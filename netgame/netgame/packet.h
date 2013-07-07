#ifndef _NETGAME_PACKET_H
#define _NETGAME_PACKET_H

#include <memory>
#include <vector>

#include <netlib/serialization.h>

class Packet
{
public:
	Packet();
	Packet(const Packet& p);
	Packet(Packet&& p);
	Packet& operator=(Packet p);
	~Packet();

	NetWriter write() { return NetWriter(data(), m_size); }
	NetReader read() const { return NetReader(data(), m_size); }

	Packet subpacket(unsigned int start, unsigned int size) const;

	inline char *data() { return m_buffer + m_start; }
	inline const char *data() const { return m_buffer + m_start; }
	inline unsigned int size() const { return m_size; }

	inline bool empty() const { return m_buffer == nullptr; }
private:
	friend class PacketPool;
	Packet(char *buf, unsigned int start, unsigned int size);

	unsigned int m_start, m_size;
	char* m_buffer;
};

class PacketPool
{
public:
	PacketPool();
	PacketPool(unsigned int maxPacketSize);
	PacketPool(PacketPool&& p);
	PacketPool& operator=(PacketPool p);

	char *nextData() { return &m_pages[m_alloc_page][m_alloc_ptr]; }
	const char *nextData() const { return &m_pages[m_alloc_page][m_alloc_ptr]; }
	unsigned int nextSize() const { return m_packet_size; }

	// Allocate a new packet
	// Contains data written to the buffer pointed by `nextData()`
	// If `size <= 0` returns an empty packet
	Packet allocate(int size);

	unsigned int numPages() const { return m_pages.size(); }

private:
	PacketPool(const PacketPool& p);

	void add_page();

	std::vector<std::unique_ptr<char[]>> m_pages;
	unsigned int m_alloc_page;
	unsigned int m_alloc_ptr;

	unsigned int m_page_size;
	unsigned int m_packet_size;
};

#endif