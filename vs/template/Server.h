#pragma once

struct ClientData {
	sockaddr_in* addr;

	Entity* entity = nullptr;

	ClientData();
};

class Server
{
	ServerApp* m_pApp;
	sockaddr_in m_addr;
	SOCKET m_sck;
	int m_port;

	std::vector<ClientData*> m_clients;
public:
	Server(int port = PORT_DEFAULT);

	auto& GetClients();
	ClientData* GetClient(sockaddr_in& clientAddr);
	bool ClientExist(sockaddr_in& clientAddr);

	bool SendTo(const char* sendBuffer, sockaddr* addr);
	bool ReceiveFrom(char* recvBuffer, sockaddr* addrFrom);

	void InterpretData(char* buffer, sockaddr_in& addr);

	void Listen();
};

