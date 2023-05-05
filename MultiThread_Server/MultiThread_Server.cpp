// MultiThread_Server.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <string>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 100
#define MAX_CLNT 256

#define SERVER_PORT 30928

int ClientCount = 0;
SOCKET clientSockets[MAX_CLNT];
HANDLE hMutex;

void SendMessageClient(char* msg, int length)
{
    WaitForSingleObject(hMutex, INFINITE);

    for (int i = 0; i < ClientCount; ++i)
        send(clientSockets[i], msg, length, 0);

    ReleaseMutex(hMutex);
}

unsigned WINAPI HandleClient(void* arg)
{
    SOCKET hClientSocket = (SOCKET)arg;
    int strLength = 0;
    char msg[BUF_SIZE];
    
    while ((strLength = recv(hClientSocket, msg, sizeof(msg), 0)) != 0)
    {
        SendMessageClient(msg, strLength);
    }

    WaitForSingleObject(hMutex, INFINITE);

    // 연결이 끊겨있는 클라이언트 제거
    for (int i = 0; i < ClientCount; ++i)
    {
        if (clientSockets[i] == hClientSocket)
        {
            // 이미 끊겨있는 대상 소켓에 하나씩 소켓 밀기 
            while (i < ClientCount - 1)
            {
                ++i;
                clientSockets[i] = clientSockets[i + 1];
            }
            break;
        }
    }

    --ClientCount;
    ReleaseMutex(hMutex);
    closesocket(hClientSocket);
    return 0;
}

int main()
{
    WSADATA wsaData;
    SOCKET hServerSocket, hClientSocket;
    SOCKADDR_IN serverAddress, clientAddress;
    HANDLE hThread;

    int ClientAddressSize = 0;

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    hMutex = CreateMutex(NULL, FALSE, NULL);
    hServerSocket = socket(PF_INET, SOCK_STREAM, NULL);

    memset(&serverAddress, 0, sizeof(SOCKADDR_IN));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(SERVER_PORT);

    bind(hServerSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));

    listen(hServerSocket, 5);

    std::cout << "Current server IP : " << inet_ntoa(serverAddress.sin_addr) << "\n";
    
    while (true)
    {
        ClientAddressSize = sizeof(clientAddress);
        hClientSocket = accept(hServerSocket, (sockaddr*)&clientAddress, &ClientAddressSize);

        WaitForSingleObject(hMutex, INFINITE);

        clientSockets[ClientCount] = hClientSocket;     // CLIENTARR에 CLIENT 등록
        ++ClientCount;


        ReleaseMutex(hMutex);

        hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClient, (void*)hClientSocket, 0, NULL);

        std::cout << "Client Entered! (IP : " << inet_ntoa(clientAddress.sin_addr) << ")\n";
    }

    closesocket(hServerSocket);
    WSACleanup();

    return 0;
}
