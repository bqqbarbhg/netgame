#include <netlib/socket.h>
#include <netlib/address.h>
#include <netlib/net.h>

#include "protocol.h"
#include "packet.h"
#include "connection.h"

#include <memory>
#include <thread>
#include <map>
#include <iostream>

NetServiceHandle handle;

void server(unsigned short port)
{
	Address address = Address::inet_any(port);
	Socket socket(address, SocketType::UDP);
	std::cout << "Opening a socket at " << address << ": " << (socket.is_open() ? "Success" : "Failure") << std::endl;

	auto ret = socket.set_blocking(false);
	if (ret != 0)
		std::cout << "Failed to set the socket non-blocking (" << ret << ")" << std::endl;

	std::map<Address, Connection> connections;

	PacketPool recvPool(MAX_PACKET_SIZE);
	Address recva(sizeof (sockaddr_in));

	while (true)
	{
		Packet recvp;
		while (!(recvp = recvPool.allocate(socket.receive_from(recva, recvPool.nextData(), recvPool.nextSize()))).empty()) {
			auto it = connections.find(recva);
			if (it == connections.end()) {
				// A new connection
				std::cout << "New connection " << recva << std::endl;
				connections[recva] = Connection(recva, 0xDEADBEEF);
				connections[recva].m_DEBUG_packet_loss = 0.5f;
				std::cout << "Creating a connection with the magic number " << 0xDEADBEEF << std::endl;
				char buf[4]; NetWriter writer(buf, 4); writer.write(0xDEADBEEF);
				socket.send_to(recva, writer.data(), writer.write_amount());
			} else {
				it->second.process_packet(recvp);
			}
		}

		for (auto& conn : connections) {
			conn.second.DEBUG_print_status();
			conn.second.send_outgoing(socket);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(33));
	}

}

void client(const char* addr, const char* port)
{
	Address address = *Address::find_by_name(addr, port, SocketType::UDP, AF_INET).begin();
	Address sadd = Address::inet_any();
	Socket socket(sadd, SocketType::UDP);
	std::cout << "Opening a socket at " << sadd << ": " << (socket.is_open() ? "Success" : "Failure") << std::endl;

	auto ret = socket.set_blocking(false);
	if (ret != 0)
		std::cout << "Failed to set the socket non-blocking (" << ret << ")" << std::endl;
	std::cout << "Connecting to " << address << std::endl;
	std::cout << "Sent connection request (" << socket.send_to(address, "ebin", 4) << ")" << std::endl;

	Connection connection;

	PacketPool recvPool(MAX_PACKET_SIZE);
	
	while (true) {
		Packet recvp = recvPool.allocate(socket.receive(recvPool.nextData(), recvPool.nextSize()));
		if (!recvp.empty()) {
			NetReader reader = recvp.read();
			magic_t magic;
			if (!reader.read(magic)) {
				continue;
			}
			std::cout << "Established a connection with the magic number " << magic << std::endl;
			connection = Connection(address, magic);
			break;
		}
	}

	while (true) {
		Packet recvp;
		while (!(recvp = recvPool.allocate(socket.receive(recvPool.nextData(), recvPool.nextSize()))).empty()) {
			connection.process_packet(recvp);
		}
		
		connection.send_outgoing(socket);

		connection.DEBUG_print_status();

		std::this_thread::sleep_for(std::chrono::milliseconds(33));
	}
}

int main(int argc, char **argv)
{
	switch (getchar()) {
	case 'c':
		client("localhost", "1337");
		break;
	case 's':
		server(1337);
		break;
	}
}