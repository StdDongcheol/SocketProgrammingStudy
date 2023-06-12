// MultiThread_Server.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define NAME_SIZE 20
#define BUF_SIZE 100
#define SERVER_PORT 30928
#define MAX_THREAD 64

// 쓰레드에 할당될 함수.
unsigned WINAPI HandleClient(void* arg);

typedef struct _WorkerThread
{
    HANDLE hThread;	            // 실제 쓰레드핸들.
    HANDLE hEvent;              // 이벤트 오브젝트 핸들
    DWORD idThread;	            // 쓰레드 id
    SOCKET CurrentSocket;       // 현재 할당되어있는 소켓
    SOCKADDR_IN addressInfo;    // 소켓의 주소정보
    bool IsActive;              // 쓰레드 활성상태 여부 변수
} WorkerThread;

struct _ThreadPool
{
    WorkerThread    workerThreadArr[MAX_THREAD];

    // 풀에 존재하는 쓰레드 갯수.
    DWORD threadSize;

    _ThreadPool() : 
        threadSize(0), 
        workerThreadArr{} {};

    // 쓰레드 사이즈만큼 풀에 쓰레드를 생성.
    void Init(size_t _ThreadCount)
    {
        if (_ThreadCount > MAX_THREAD)
            return;

        for (size_t i = 0; i < _ThreadCount; i++)
        {
            unsigned int idThread;
            HANDLE hThread;

            // manual-reset 모드로 제어하는 event오브젝트 생성.
            workerThreadArr[i].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            // HandleClient함수포인터와 (LPVOID)i를 파라미터로 받는 쓰레드 생성
            hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClient, (LPVOID)i, 0, &idThread);

            // 쓰레드 핸들과 id를 저장.
            workerThreadArr[i].hThread = hThread;
            workerThreadArr[i].idThread = (DWORD)&idThread;

            // 쓰레드 갯수 증가
            threadSize++;
        }
    }

    // 현재 대기중인 쓰레드에 소켓 정보를 할당하는 함수.
    HANDLE& AllocateSocketThread(SOCKET& _clientSock, SOCKADDR_IN& _clientAddress)
    {
        HANDLE handle = NULL;

        for (int i = 0; i < (int)threadSize; ++i)
        {
            // 비활성중인 쓰레드인 경우, i번째 쓰레드에 소켓정보 할당.
            if (!workerThreadArr[i].IsActive)
            {
                workerThreadArr[i].CurrentSocket = _clientSock;     // 소켓 할당
                workerThreadArr[i].addressInfo = _clientAddress;    // 소켓 SOCKADDR_IN 정보 할당
                workerThreadArr[i].IsActive = true;                 // 쓰레드 활성화

                // 할당된 쓰레드의 이벤트오브젝트를 반환.
                return workerThreadArr[i].hEvent;
            }
        }

        // 모두 활성화상태일 경우 NULL 반환
        return handle;
    }
};

_ThreadPool ThreadPool;             // 별도 정의한 쓰레드풀
int ClientCount = 0;                // 현재 접속중인 클라이언트 수
SOCKET clientSockets[MAX_THREAD];   // 현재 접속중인 클라이언트 소켓 배열
HANDLE hMutex;                      // 메인쓰레드 뮤텍스

int main()
{
    SYSTEM_INFO* Info;
    WSADATA wsaData;
    SOCKET hServerSocket, hClientSocket;
    SOCKADDR_IN serverAddress, clientAddress;
    int ClientAddressSize = 0;

    // SystemInfo를 받아 시스템에서 할당가능한 쓰레드 갯수만큼 풀을 초기화.
    Info = (SYSTEM_INFO*)malloc(sizeof(SYSTEM_INFO));
    GetSystemInfo(Info);
    ThreadPool.Init(Info->dwNumberOfProcessors);
    
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 메인 쓰레드 뮤텍스 생성
    hMutex = CreateMutex(NULL, FALSE, NULL);
    hServerSocket = socket(PF_INET, SOCK_STREAM, NULL); // 서버 소켓 생성

    // 서버 소켓주소 초기화후 정보 할당
    memset(&serverAddress, 0, sizeof(SOCKADDR_IN));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(SERVER_PORT);

    bind(hServerSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));

    listen(hServerSocket, 5);
    
    while (true)
    {
        ClientAddressSize = sizeof(clientAddress);
        hClientSocket = accept(hServerSocket, (sockaddr*)&clientAddress, &ClientAddressSize);
        std::cout << "New client entered. (IP : " << inet_ntoa(clientAddress.sin_addr) << ")\n";

        // 메인 쓰레드 뮤텍스 획득
        WaitForSingleObject(hMutex, INFINITE);

        // 해당 쓰레드에 소켓 정보할당 및 이벤트오브젝트를 반환.
        HANDLE hEvent = ThreadPool.AllocateSocketThread(hClientSocket, clientAddress);
        
        // NULL시, 모든 쓰레드가 할당되어있음.
        // 해당 소켓을 종료시키고 뮤텍스 해제.
        if (hEvent == NULL)
        {
            closesocket(hClientSocket);
            ReleaseMutex(hMutex);
            continue;
        }

        clientSockets[ClientCount] = hClientSocket;     // 클라이언트 소켓 배열에 소켓 등록

        // SetEvent로 해당 이벤트오브젝트를 signaled 상태로 전환, 쓰레드 활성화.
        SetEvent(hEvent);

        ++ClientCount;
        std::cout << "[ServerInfo]  Current Users : " << ClientCount << '\n';

        // 메인 쓰레드 뮤텍스 해제
        ReleaseMutex(hMutex);
    }

    closesocket(hServerSocket);
    WSACleanup();
    free(Info);
    return 0;
}

// 쓰레드에 할당될 함수.
// 파라미터에는 쓰레드풀에서의 각 쓰레드 인덱스를 전달받음.
unsigned WINAPI HandleClient(void* arg)
{
    while (true)
    {
        // 풀에서 쓰레드를 생성한 직후, non-signaled로 전환.
        WaitForSingleObject(ThreadPool.workerThreadArr[(int)arg].hEvent, INFINITE);
        // SetEvent로부터 이벤트가 signaled 상태로 전환되면 동작.

        SOCKET hClientSocket = ThreadPool.workerThreadArr[(int)arg].CurrentSocket;
        int strLength = 0;
        bool FirstMsgCheck = true;
        char Name[NAME_SIZE] = {};
        char msg[BUF_SIZE] = {};

        while ((strLength = recv(hClientSocket, msg, sizeof(msg), 0)) != -1)
        {
            // 메인 뮤텍스 획득, 뮤텍스 반환지점까지 임계영역 설정.
            WaitForSingleObject(hMutex, INFINITE);

            // 첫 접속시, 입장메시지를 각 클라이언트에게 send
            if (FirstMsgCheck && msg)
            {
                std::string str = msg;
                strcpy_s(msg, str.substr(0, strLength).c_str());
                strcpy_s(Name, msg);
                strcat_s(msg, " is Entered !");

                strLength = strlen(msg);
                FirstMsgCheck = false;
            }

            // 자기 자신 클라이언트를 제외하고
            // 접속 중인 각 클라이언트 소켓에게 length만큼 msg를 전송.
            for (int i = 0; i < ClientCount; ++i)
            {
                if (hClientSocket == clientSockets[i])
                    continue;

                send(clientSockets[i], msg, strLength, 0);
            }

            // 뮤텍스 반환하여 non-signaled로 전환
            ReleaseMutex(hMutex);
        }

        // length값이 -1의 경우, 루프종료 및 해당 클라이언트 종료 시작
        // 메인 뮤텍스 획득
        WaitForSingleObject(hMutex, INFINITE);

        std::string str = Name;
        str += " was disconnected.... \n";

        // 각 클라이언트에게 메시지 전송. ex)[Name] was disconnected....
        strcpy_s(msg, str.c_str());
        strLength = strlen(msg);
        for (int i = 0; i < ClientCount; ++i)
        {
            if (hClientSocket == clientSockets[i])
                continue;

            send(clientSockets[i], msg, strLength, 0);
        }

        // 서버에는 접속 종료한 클라이언트 주소정보 표기
        std::cout << inet_ntoa(ThreadPool.workerThreadArr[(int)arg].addressInfo.sin_addr) << " was disconnected.\n";

        // 연결이 끊긴 소켓 제거
        for (int i = 0; i < ClientCount; ++i)
        {
            if (clientSockets[i] == hClientSocket)
            {
                // 소켓을 하나씩 밀어서 클라이언트 소켓 배열에서 제거.
                while (i < ClientCount)
                {
                    clientSockets[i] = clientSockets[i + 1];
                    ++i;
                }
                break;
            }
        }

        // 클라이언트 수 감소
        --ClientCount;

        // 쓰레드내 소켓정보 초기화 및 쓰레드 비활성으로 설정
        ThreadPool.workerThreadArr[(int)arg].IsActive = false;
        ThreadPool.workerThreadArr[(int)arg].CurrentSocket = NULL;
        memset(&ThreadPool.workerThreadArr[(int)arg].addressInfo, 0, sizeof(SOCKADDR_IN));
        
        // resetEvent로 쓰레드 내 이벤트 오브젝트를 signaled 상태로 전환.
        ResetEvent(ThreadPool.workerThreadArr[(int)arg].hEvent);

        // 메인 뮤텍스를 반환하여 non-signaled로 전환.
        ReleaseMutex(hMutex);

        // 소켓 연결종료
        closesocket(hClientSocket);
    }

    return 0;
}
