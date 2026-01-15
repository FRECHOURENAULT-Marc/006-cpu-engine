#include "pch.h"
#include "Client.h"

#include "utils.h"

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

void Client::UpdateClient()
{

}

void Client::UpdateInputs()
{
	//Send inputs
	ResetInputsBuffer();
	while (true) {

		if (cpuInput.IsKeyDown('Z')) {
			inputs.append("z");
		}
		if (cpuInput.IsKeyDown('S')) {
			inputs.append("s");
		}
		if (cpuInput.IsKeyDown('D')) {
			inputs.append("d");
		}
		if (cpuInput.IsKeyDown('Q')) {
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
}
