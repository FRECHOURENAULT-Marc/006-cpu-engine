#include "pch.h"
#include "utils.h"

namespace utils {
	//int Random(int min, int max)
	//{
	//	return rand() % max + min;
	//}
	void PrintError(std::string msg) {
		OutputDebugStringA("[**** *** ***]\n[**** ERR ***] ");
		OutputDebugStringA(msg.c_str());
		OutputDebugStringA("\n[**** *** ***]\n");
	}
	void PrintError(std::string msg, int data) {
		OutputDebugStringA("[**** *** ***]\n[**** ERR ***] ");
		OutputDebugStringA(msg.c_str());
		OutputDebugStringA(" : ");
		OutputDebugStringA(std::to_string(data).c_str());
		OutputDebugStringA("\n[**** *** ***]\n");
	}
	void PrintMSG(std::string msg) {
		OutputDebugStringA("[#### ### ###]\n[#### MSG ###] ");
		OutputDebugStringA(msg.c_str());
		OutputDebugStringA("\n[#### ### ###]\n");
	}
	void PrintData(std::string data) {
		OutputDebugStringA("[++++ +++ +++]\n[+++ DATA +++] ");
		OutputDebugStringA(data.c_str());
		OutputDebugStringA("\n[++++ +++ +++]\n");
	}
	void PrintData(int data) {
		PrintData(std::to_string(data));
	}
	void PrintData(float data) {
		PrintData(std::to_string(data));
	}
	void PrintData(std::string msg, std::string data) {

		OutputDebugStringA("[#### #++ +++]\n[# MSG DATA +] ");
		OutputDebugStringA(msg.c_str());
		OutputDebugStringA(" : ");
		OutputDebugStringA(data.c_str());
		OutputDebugStringA("\n[#### #++ +++]\n");
	}
	void PrintData(std::string msg, int data) {
		PrintData(msg, std::to_string(data));
	}
	void PrintData(std::string msg, float data) {
		PrintData(msg, std::to_string(data));
	}

	void InitLib()
	{
		WSADATA* data = new WSADATA;
		WSAStartup(MAKEWORD(2, 2), data);

		srand(time(NULL));
	}

	int GetId() {
		return ID::GetID();
	}

	std::string GetAddressStr(sockaddr_in& addr) {
		char buff[INET6_ADDRSTRLEN] = { 0 };
		return inet_ntop(addr.sin_family, (void*)&(addr.sin_addr), buff, INET6_ADDRSTRLEN);
	}

	bool StrContain(std::string toAnalyze, std::string word)
	{
		if (toAnalyze.empty() || word.empty())
			return false;

		int actualIndex = 0;
		int lastIndex = 0;
		for (int i = 0; i < toAnalyze.size(); i++) {

			char c = toAnalyze[i];

			if (actualIndex >= word.size())
				return true;

			if (actualIndex == 0 && word[0] == c) {
				actualIndex++;
				lastIndex = i;
				continue;
			}
			if (word[actualIndex] == c && lastIndex == i - 1) {
				actualIndex++;
				lastIndex = i;
				continue;
			}
			actualIndex = 0;
			lastIndex = 0;
		}

		if (actualIndex >= word.size())
			return true;

		return false;
	}
}

ID::ID()
{
	instance = this;
	for (int i = 0; i < 100; i++) {
		IDs[i] = i;
	}
}

int ID::GetID()
{
	int res = instance->IDs[instance->lastID];
	instance->lastID++;
	return res;
}
