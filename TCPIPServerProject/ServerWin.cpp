

#include <iostream>
#include <stdlib.h>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT        9591

int main(int args, char* argv[])
{
	WSADATA wsaData;
	SOCKET hServerSock, hClientSocket;
	SOCKADDR_IN ServerAddr, ClientAddr;

	int ClientAddrSize;
	char Message[] = "Hello World!!!";
	args = 2;
	if (args != 2)
	{
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NULL)
	{
		std::cout << "Error!!!";
		exit(1);
	}

	hServerSock = socket(PF_INET, SOCK_STREAM, 0);

	if (hServerSock == INVALID_SOCKET)
		exit(1);

	int opt = 1;
	setsockopt(hServerSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

	memset(&ServerAddr, 0, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	ServerAddr.sin_port = htons(SERVER_PORT);

	bind(hServerSock, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));

	if (listen(hServerSock, 5) == SOCKET_ERROR)
		exit(1);

	ClientAddrSize = sizeof(ClientAddr);
	hClientSocket = accept(hServerSock, (SOCKADDR*)&ClientAddr, &ClientAddrSize);

	if (hClientSocket == INVALID_SOCKET)
		exit(1);

	send(hClientSocket, Message, sizeof(Message), 0);

	closesocket(hClientSocket);
	closesocket(hServerSock);
	WSACleanup();

	return 0;
}