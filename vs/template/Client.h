#pragma once
class Client
{
	sockaddr_in m_servAddr;
	SOCKET m_sck;
	char* m_ip;
	int m_port;

	ClientApp* m_pApp;

	std::string inputs = "";

	void ResetInputsBuffer();

public:
	Client(const char* ip, int port = PORT_DEFAULT);

	void Send(const char* msg);
	bool Receive(char* recvBuffer);

	void UpdateClient();
	void UpdateInputs();
	void UpdateClientGame();
};

