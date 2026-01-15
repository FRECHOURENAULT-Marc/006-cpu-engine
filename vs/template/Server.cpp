#include "pch.h"
#include "Server.h"

#include "utils.h"
#include "Entity.h"
#include "App.h"

////////////////////////////////
// CLIENT DATA
////////////////////////////////

ClientData::ClientData()
{
	// data = new std::string();
}

////////////////////////////////
// SERVER
////////////////////////////////

Server::Server(int port)
{
	m_pApp = (ServerApp*) &App::GetInstance();
	m_sck = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_sck == INVALID_SOCKET)
	{
		utils::PrintError("Init socket error", WSAGetLastError());
		return;
	}
	m_port = port;
	//Addr
	m_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_addr.sin_port = htons(m_port);
	m_addr.sin_family = AF_INET;

	//Bind
	int bindRes = bind(m_sck, (sockaddr*)&m_addr, sizeof(m_addr));
	if (bindRes != 0)
	{
		utils::PrintError("Bind error", WSAGetLastError());
		throw;
	}
	utils::PrintMSG("Server started on port " + std::to_string(m_port));
}

auto& Server::GetClients()
{
	return m_clients;
}

ClientData* Server::GetClient(sockaddr_in& clientAddr)
{
	std::string clientAddrStr = utils::GetAddressStr(clientAddr);

	for (ClientData* actualClient : m_clients) {
		sockaddr_in actualAddr = *actualClient->addr;
		std::string actualAddrStr = utils::GetAddressStr(actualAddr);
		if (actualAddrStr != clientAddrStr)
			continue;

		return actualClient;
	}

	return nullptr;
}

bool Server::ClientExist(sockaddr_in& clientAddr)
{
	std::string clientAddrStr = utils::GetAddressStr(clientAddr);

	for (ClientData* actualClient : m_clients) {
		sockaddr_in actualAddr = *actualClient->addr;
		std::string actualAddrStr = utils::GetAddressStr(actualAddr);
		if (actualAddrStr != clientAddrStr)
			continue;

		return true;
	}

	return false;
}

bool Server::SendTo(const char* msg, sockaddr* addrTo)
{
	int clientAddrLen = sizeof(*addrTo);
	int sendRes = sendto(m_sck, msg, BUF_LEN_DEFAULT, 0, addrTo, clientAddrLen);
	if (sendRes > 0)
		return true;
	utils::PrintError("Error when send data", sendRes);
	return false;
}
bool Server::ReceiveFrom(char* recvBuffer, sockaddr* addrFrom)
{
	int clientAddrLen = sizeof(*addrFrom);
	int recvRes = recvfrom(m_sck, recvBuffer, BUF_LEN_DEFAULT, 0, addrFrom, &clientAddrLen);
	if (recvRes <= 0)
	{
		utils::PrintError("Error with receive", recvRes);
		return false;
	}
	return true;
}

void Server::InterpretData(char* buffer, sockaddr_in& addr)
{
	//t:cre name:... 
	//t:upd key:zddq

	bool isCreation = utils::StrContain(buffer, BUF_TYPE_CREATION);
	bool isUpdate = utils::StrContain(buffer, BUF_TYPE_UPDATE);

	if (isCreation) {

		//reconnexion
		if (ClientExist(addr))
			return;

		//Add new entity
		ClientData* c = new ClientData;
		c->addr = new sockaddr_in(addr);
		c->entity = m_pApp->CreateEntity();
		m_clients.push_back(c);
		return;
	}
	if (isUpdate) {

		//no creation before
		if (ClientExist(addr) == false)
			return;

		ClientData* c = GetClient(addr);

		std::string keys = "";
		utils::Deserialize<std::string>(buffer, BUF_DATA_KEY, &keys);

		for(int i = 0; i < keys.size(); i++) {
			
			char key = keys[i];

			if(key == 'z')
				c->entity->Move(0, 0, 1);
			if (key == 's')
				c->entity->Move(0, 0, -1);
			if (key == 'd')
				c->entity->Move(1, 0, 0);
			if(key == 'q')
				c->entity->Move(-1, 0, 0);
		}

		return;
	}

	utils::PrintError("Data cannot be Interpreted");
}

DWORD WINAPI dataSendLoop(_In_ LPVOID lpParameter)
{
	Server* serv = (Server*)lpParameter;

	//TO DO
	while (true) {
		auto& clients = serv->GetClients();
		Sleep(500);

		for (ClientData* cData : clients) {
			cData->addr;
		}
	}

	return 0;
}
DWORD WINAPI dataReceiveLoop(_In_ LPVOID lpParameter)
{
	Server* serv = (Server*)lpParameter;

	while (true) {

		char recvBuffer[BUF_LEN_DEFAULT] = "";
		sockaddr_in clientAddr;
		if (serv->ReceiveFrom(recvBuffer, (sockaddr*)&clientAddr) == false)
			continue;

		auto& clients = serv->GetClients();

		utils::PrintMSG(
			"Message receive (from client " 
			+ utils::GetAddressStr(clientAddr) + ", ID " 
			+ std::to_string(clients.size() - 1) + ")");
		utils::PrintData(recvBuffer);

		serv->InterpretData(recvBuffer, clientAddr);
	}

	return 0;
}

void Server::Listen()
{
	HANDLE dataSendThread = CreateThread(NULL, 0, dataSendLoop, this, 0, 0);

	//Receive from client
	HANDLE dataReceiveThread = CreateThread(NULL, 0, dataReceiveLoop, this, 0, 0);

	//wait dataReceiveThread
	WaitForSingleObject(dataReceiveThread, INFINITE);

	return;
}
