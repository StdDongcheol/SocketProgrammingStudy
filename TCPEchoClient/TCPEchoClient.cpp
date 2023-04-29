
#include <iostream>
#include <stdlib.h>
#include <string>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE        1024
#define SERVER_PORT        9591

int main()
{
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN ServerAddr;
	
	char ServerIP[16];
	std::string strMessage;
	char Message[BUF_SIZE];
	int strLength;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NULL)
	{
		std::cout << "Error!!!";
		exit(1);
	}

	hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
		exit(1);

	std::cout << "input to Connect server IP : ";
	std::cin >> ServerIP;

	if (!ServerIP)
		exit(1);

	memset(&ServerAddr, 0, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.S_un.S_addr = inet_addr(ServerIP); // 127.0.0.1
	ServerAddr.sin_port = htons(SERVER_PORT);	// port 9591

	if (connect(hSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
		exit(1);

	std::cout << "Server Connected !!!!\n";
	std::cout << "================================================\n";

	while (true)
	{
		std::cout << "input your message. (0 will quit.) : ";
		std::cin >> strMessage;

		if (strMessage[0] == '0')
			break;

		send(hSocket, strMessage.c_str(), (int)strMessage.size(), 0);
		strLength = recv(hSocket, Message, BUF_SIZE - 1, 0);
		Message[strLength] = 0;

		std::cout << "Recieved message from Server : " << Message << '\n';
	}

	closesocket(hSocket);
	WSACleanup();

	return 0;
}