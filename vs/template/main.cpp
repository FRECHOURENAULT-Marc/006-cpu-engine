#include "pch.h"

#include "Server.h"
#include "Client.h"
#include "utils.h"
#include <conio.h>

//////////////////
/// SERVER
//////////////////

DWORD WINAPI ServerAppLoop(_In_ LPVOID lpParameter)
{
	cpu::Run<cpu_engine, ServerApp>(1024, 576);
	return 0;
}
DWORD WINAPI ServerLoop(_In_ LPVOID lpParameter)
{
	Server serv;
	serv.Listen();
	return 0;
}

//////////////////
/// CLIENT
//////////////////

DWORD WINAPI ClientAppLoop(_In_ LPVOID lpParameter)
{
	cpu::Run<cpu_engine, ClientApp>(1024, 576);
	return 0;
}

DWORD WINAPI ClientLoop(_In_ LPVOID lpParameter)
{
	Client c("10.10.137.46");
	c.Send("t:cre name:... ");
	c.UpdateClient();
	return 0;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int cmdShow)
{
	utils::InitLib();

	AllocConsole();
	char choice = _getch();
	if (choice == 's') {
		HANDLE gameThread = CreateThread(NULL, 0, ServerAppLoop, nullptr, 0, 0);

		WaitForSingleObject(gameThread, 1000);

		HANDLE serverThread = CreateThread(NULL, 0, ServerLoop, nullptr, 0, 0);

		WaitForSingleObject(gameThread, INFINITE);
	}
	if (choice == 'c') {
		HANDLE gameThread = CreateThread(NULL, 0, ClientAppLoop, nullptr, 0, 0);

		WaitForSingleObject(gameThread, 1000);

		HANDLE clientThread = CreateThread(NULL, 0, ClientLoop, nullptr, 0, 0);

		WaitForSingleObject(gameThread, INFINITE);
	}

	return 0;
}
