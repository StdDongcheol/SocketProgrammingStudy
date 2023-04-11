

#include <iostream>
#include <stdlib.h>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT        9591

int main(int args, char* argv[])
{
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN ServerAddr;


	char Message[30];
	int strLength;

	if (args != 3)
	{
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NULL)
	{
		std::cout << "Error!!!";
		exit(1);
	}

	hSocket = socket(PF_INET, SOCK_STREAM, 0);

	if (hSocket == INVALID_SOCKET)
		exit(1);

	memset(&ServerAddr, 0, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.S_un.S_addr = inet_addr(argv[1]); // 127.0.0.1
	ServerAddr.sin_port = htons(SERVER_PORT);	// port 9591
	
	if (connect(hSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
		exit(1);

	strLength = recv(hSocket, Message, sizeof(Message) - 1, 0);

	if (strLength == -1)
		exit(1);

	std::cout << "Servers Message : " << Message << "\n";

	closesocket(hSocket);
	WSACleanup();

	return 0;
}