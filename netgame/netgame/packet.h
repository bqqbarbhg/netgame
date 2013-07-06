#ifndef _NETGAME_PACKET_H
#define _NETGAME_PACKET_H

#include <utility>
#include <netlib/serialization.h>

class Packet
{
public:
	explicit Packet(unsigned int sz)
		: m_ptr(new char[sz])
		, m_size(sz)
	{
	}
	Packet(const Packet& p)
		: m_ptr(new char[p.m_size])
		, m_size(p.m_size)
	{
		memcpy(m_ptr, p.m_ptr, m_size);
	}
	Packet(Packet&& p)
		: m_ptr(p.m_ptr)
		, m_size(p.m_size)
	{
		p.m_ptr = nullptr;
	}
	Packet& operator=(Packet p)
	{
		std::swap(m_ptr, p.m_ptr);
		m_size = p.m_size;
		return *this;
	}
	~Packet()
	{
		delete[] m_ptr;
	}

	NetReader read() const { return NetReader(m_ptr, m_size); }
	NetWriter write()      { return NetWriter(m_ptr, m_size); }

	char* data() { return m_ptr; }
	const char* data() const { return m_ptr; }

	unsigned int size() const { return m_size; }

private:
	char *m_ptr;
	unsigned int m_size;
};

#endif