// MultiThread_Client.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <string>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 100
#define NAME_SIZE 20
#define PORTS 30928

char msg[BUF_SIZE];
char Nickname[NAME_SIZE] = "[UNKNOWN]";

unsigned WINAPI SendMsgFunc(void* arg)
{
    SOCKET hSock = (SOCKET)arg;
    char NameMsg[NAME_SIZE + BUF_SIZE];
    bool FirstCheck = true;

    while (true) 
    {
        std::cin.getline(msg, BUF_SIZE);

        if (msg[0] == -1)
        {
            closesocket(hSock);
            exit(0);
        }
        sprintf_s(NameMsg, "[%s]: %s", Nickname, msg);

        if (FirstCheck)
        {
            FirstCheck = false;
            sprintf_s(NameMsg, "[%s]", Nickname);
        }
       
        send(hSock, NameMsg, (int)strlen(NameMsg), 0);
    }

    return 0;
}

unsigned WINAPI RecvMsgFunc(void* arg)
{
    SOCKET hSock = (SOCKET)arg;

    char NameMsg[NAME_SIZE + BUF_SIZE];

    int strLength = 0;

    while (true)
    {
        strLength = recv(hSock, NameMsg, NAME_SIZE + BUF_SIZE - 1, 0);

        if (strLength == -1)
            return -1;

        NameMsg[strLength] = 0;

        std::cout << NameMsg << std::endl;
    }

    return 0;
}

int main()
{
    WSADATA wsaData;
    SOCKET hMySocket;
    SOCKADDR_IN serverAddress;
    HANDLE hSendThread, hRecvThread;

    std::string strServerIP;

    std::cout << "input your name :";
    std::cin >> Nickname;
    std::cout << "input connect to serverIP :";
    std::cin >> strServerIP;

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    hMySocket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serverAddress, 0, sizeof(SOCKADDR_IN));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.S_un.S_addr = inet_addr(strServerIP.c_str());
    serverAddress.sin_port = htons(PORTS);

    if (SOCKET_ERROR == connect(hMySocket, (sockaddr*)&serverAddress, sizeof(serverAddress)))
    {
        std::cout << "Error!!!!!!!!";
        exit(-1);
    }
    
    hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendMsgFunc, (void*)hMySocket, 0, NULL);
    hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsgFunc, (void*)hMySocket, 0, NULL);

    WaitForSingleObject(hSendThread, INFINITE);
    WaitForSingleObject(hRecvThread, INFINITE);

    closesocket(hMySocket);
    WSACleanup();
    return 0;
}
