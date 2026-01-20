#pragma once

#include <iostream>

#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BROWN "\033[38;6;94m"
#define ORANGE "\033[38;5;214m"
#define DEFAULT "\033[39m"
#define SGREEN "\033[92m"
#define SRED "\033[91m"


struct ID {
	int IDs[100];
	int lastID = 0;

	inline static ID* instance = nullptr;

	ID();
	static int GetID();
};



namespace utils {
	int Random(int min, int max) = delete;

	void PrintError(std::string msg);
	void PrintError(std::string msg, int data);
	void PrintMSG(std::string msg);
	void PrintData(std::string data);
	void PrintData(int data);
	void PrintData(float data);
	void PrintData(std::string msg, std::string data);
	void PrintData(std::string msg, int data);
	void PrintData(std::string msg, float data);

	void InitLib();

	int GetId();

	std::string GetAddressStr(sockaddr_in& addr);

	bool StrContain(std::string toAnalyze, std::string word);

	template <typename T>
	inline void Deserialize(std::string data, std::string varName, T* var) {
		varName += ":";
		int pos = data.find(varName);
		if (pos == data.npos) {
			PrintError("No variable " + varName + " found in " + data);
			return;
		}
		pos += varName.size();
		char c = data.at(pos);
		std::string tempStr;
		while (c != ' ' && c != '\n') {
			tempStr.push_back(c);
			if (pos == data.size() - 1)
				break;
			pos++;
			c = data.at(pos);
		}

		if (std::is_same<T, float>::value)
			*var = std::stof(tempStr);
		else if (std::is_same<T, int>::value)
			*var = std::stoi(tempStr);
		else if (std::is_same<T, std::string>::value)
			((std::string*)var)->assign(tempStr);
	}
}

