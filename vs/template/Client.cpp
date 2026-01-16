#include "pch.h"
#include "Client.h"

#include "utils.h"
#include "Entity.h"

void Client::ResetInputsBuffer()
{
	inputs.clear();
	inputs = "t:";
	inputs.append(BUF_TYPE_UPDATE);
	inputs.append(" ");
	inputs.append(BUF_DATA_KEY);
	inputs.append(":");
}

Client::Client(const char* ip, int port)
{
	utils::PrintMSG("CLIENT SIDE");

	m_sck = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	m_ip = (char*)ip;
	m_port = port;

	m_servAddr = { 0 };
	inet_pton(AF_INET, ip, &m_servAddr.sin_addr.s_addr);
	m_servAddr.sin_family = AF_INET;
	m_servAddr.sin_port = htons(port);
}

void Client::Send(const char* msg) {

	int sendRes = sendto(m_sck, msg, BUF_LEN_DEFAULT, 0, (sockaddr*)&m_servAddr, sizeof(m_servAddr));
	if (sendRes <= 0) {
		utils::PrintError("Error when sending message to server");
		return;
	}
	utils::PrintData("Message send", msg);
}

bool Client::Receive(char* recvBuffer) {

	int servAddrLen = sizeof(m_servAddr);
	int recvRes = recvfrom(m_sck, recvBuffer, BUF_LEN_DEFAULT, 0, (sockaddr*)&m_servAddr, &servAddrLen);
	if (recvRes <= 0)
		return false;
	utils::PrintData("Message received", recvBuffer);
	return true;
}

DWORD WINAPI ReceiveClientLoop(_In_ LPVOID lpParameter)
{
	Client* c = (Client*)lpParameter;
	c->UpdateClientGame();
	return 0;
}
DWORD WINAPI SendClientLoop(_In_ LPVOID lpParameter)
{
	Client* c = (Client*)lpParameter;
	c->UpdateInputs();
	return 0;
}

void Client::UpdateClient()
{
	HANDLE receiveThread = CreateThread(NULL, 0, ReceiveClientLoop, this, 0, 0);

	WaitForSingleObject(receiveThread, 1000);

	HANDLE sendThread = CreateThread(NULL, 0, SendClientLoop, this, 0, 0);

	WaitForSingleObject(sendThread, INFINITE);
}

void Client::UpdateInputs()
{
	//Send inputs
	ResetInputsBuffer();
	while (true) {

		if (cpuInput.IsKey('Z')) {
			inputs.append("z");
		}
		if (cpuInput.IsKey('S')) {
			inputs.append("s");
		}
		if (cpuInput.IsKey('D')) {
			inputs.append("d");
		}
		if (cpuInput.IsKey('Q')) {
			inputs.append("q");
		}
		if (inputs.size() > 11) {
			Send(inputs.c_str());
			ResetInputsBuffer();
		}
	}
}

void Client::UpdateClientGame()
{
	while (true) {
		char buffer[BUF_LEN_DEFAULT] = "";
		Receive(buffer);

		utils::PrintData(buffer);

		int eNb = 0;
		utils::Deserialize<int>(buffer, "eNb", &eNb);

		auto& entities = App::GetEntities();
		if (entities.size() < eNb) {
			int diff = eNb - entities.size();
			for (int i = 0; i < diff; i++) {
				entities.push_back(new Entity);
			}
		}

		for (int i = 0; i < eNb; i++) {

			std::string varNameX = "e" + std::to_string(i) + "X";
			std::string varNameY = "e" + std::to_string(i) + "Y";
			std::string varNameZ = "e" + std::to_string(i) + "Z";
			float x, y, z;
			utils::Deserialize<float>(buffer, varNameX, &x);
			utils::Deserialize<float>(buffer, varNameY, &y);
			utils::Deserialize<float>(buffer, varNameZ, &z);
			XMFLOAT3 pos = { x, y, z };

			utils::PrintData(pos.z);

			Entity* e = entities[i];
			e->SetPos(pos);
		}
	}
}
