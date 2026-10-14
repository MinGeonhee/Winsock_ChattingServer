#include "pch.h"             // pch를 쓰면 반드시 맨 위 (이보다 위의 코드는 무시됨)
#include "ChattingLogic.h"

#include <future>
#include <chrono>

using namespace std;

constexpr const char* SERVER_IP = "127.0.0.1";          // 같은 PC 테스트: 루프백 주소 (IP 입력 시 Enter 기본값)
constexpr uint16_t    SERVER_PORT = 1234;               // 서버가 bind한 포트와 같아야 함
constexpr int         MAX_NAME_LEN = 10;                // 닉네임 글자 수 제한

string InputNickname()
{
    while (true)
    {
        cout << " 채팅방에서 사용할 닉네임을 정하세요 : ";
        string name;
        getline(cin, name);

        // CP949 문자열을 와이드 문자로 바꿨을 때의 칸 수 = 글자 수
        int count = MultiByteToWideChar(CP_ACP, 0, name.c_str(), (int)name.size(), nullptr, 0);

        if (count == 0)
            cout << " 경고 : 닉네임을 입력해 주세요" << endl << endl;
        else if (count > MAX_NAME_LEN)
            cout << " 경고 : 닉네임은 [ " << MAX_NAME_LEN << "글자 ] 를 넘을 수 없습니다" << endl << endl;
        else if (name.find(' ') != string::npos)
            cout << " 경고 : 닉네임에 띄어쓰기를 넣을 수 없습니다" << endl << endl;
        else
            return name;
    }
}

/* 접속할 서버 IP 입력 (Enter = 127.0.0.1) → 4바이트 주소로 변환해서 outAddr에 저장 */
void InputServerIP(IN_ADDR& outAddr)
{
    while (true)
    {
        cout << " 접속할 서버 IP (Enter = " << SERVER_IP << ") : ";
        string ip;
        getline(cin, ip);
        if (ip.empty())
            ip = SERVER_IP;

        if (inet_pton(AF_INET, ip.c_str(), &outAddr) == 1)   // "127.0.0.1" 문자열 → 4바이트 숫자, 형식이 틀리면 0
            return;

        cout << " 경고 : IP 형식이 올바르지 않습니다 (예: 192.168.0.5)" << endl << endl;
    }
}

/* 접속할 서버 포트 입력 (Enter = 기본 포트) */
uint16_t InputPort(uint16_t defaultPort)
{
    while (true)
    {
        cout << " 접속할 포트 번호 (1024 ~ 65535, Enter = " << defaultPort << ") : ";
        string line;
        getline(cin, line);

        if (line.empty())
            return defaultPort;

        if (line.size() <= 5 && line.find_first_not_of("0123456789") == string::npos)
        {
            int port = stoi(line);
            if (port >= 1024 && port <= 65535)
                return static_cast<uint16_t>(port);
        }
        cout << " 경고 : 1024 ~ 65535 사이 숫자를 입력해 주세요" << endl << endl;
    }
}

int ConnectWithLoading(SOCKET sock, const SOCKADDR_IN& addr)
{
    using namespace std::chrono;
    const auto minDuration = milliseconds(1500);   // 최소 1.5초는 보여주기
    const char spinner[] = { '|', '/', '-', '\\' };

    // 에러 번호는 connect를 실행한 스레드에서 가져와야 정확함
    auto result = async(launch::async, [sock, &addr]() -> int {
        if (connect(sock, (const SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
            return WSAGetLastError();
        return 0;
        });

    const auto start = steady_clock::now();
    int frame = 0;

    while (true)
    {
        bool done = result.wait_for(milliseconds(0)) == future_status::ready;
        bool minTime = steady_clock::now() - start >= minDuration;
        if (done && minTime)
            break;

        int dots = (frame / 3) % 4;
        cout << "\r 채팅 서버 접속 중 " << spinner[frame % 4] << " "
            << string(dots, '.') << string(3 - dots, ' ') << flush;

        ++frame;
        Sleep(100);
    }

    int error = result.get();
    cout << "\r 채팅 서버 접속 " << (error == 0 ? "완료!          " : "실패           ") << endl;
    if (error == 0)
        Sleep(300);   // 완료 문구를 잠깐 보여주기

    return error;
}


int main()
{
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hIn, &mode);
    mode |= ENABLE_EXTENDED_FLAGS;        // 이 플래그가 있어야 빠른 편집 설정이 적용됨
    mode &= ~ENABLE_QUICK_EDIT_MODE;      // 빠른 편집(마우스 선택) 끄기
    SetConsoleMode(hIn, mode);


    // 1. 닉네임
    string strName = InputNickname();

    // 2. Winsock 초기화 (클라이언트도 프로세스마다 따로 해야 함)
    WSADATA wsaData{};
    int startError = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (startError != 0)
    {
        // WSAStartup은 실패하면 에러 번호를 바로 반환 (WSAGetLastError 사용 불가)
        cout << " 초기화 실패 : " << startError << endl;
        return 1;
    }

    // 3. 접속할 서버 주소
    SOCKADDR_IN servAddr{};
    servAddr.sin_family = AF_INET;                       // IPv4
    InputServerIP(servAddr.sin_addr);
    servAddr.sin_port = htons(InputPort(Protocal::Value::Server_Port));   // 네트워크 바이트 순서(빅 엔디안)로

    // 4. TCP 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        cout << " 소켓 생성 실패 : " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // 5. 서버 접속 (OS가 3-way handshake 진행)
    int connectError = ConnectWithLoading(sock, servAddr);
    if (connectError != 0)
    {
        // 10061: 서버가 꺼져 있음 / 10060: 응답 없음(주소 오류, 방화벽)
        cout << " 서버 연결 실패 : " << connectError << endl;
        closesocket(sock);
        WSACleanup();
        system("pause");
        return 1;
    }

    // 6. 채팅 화면
    //    수신 스레드(서버 메시지 → 큐)와 Input → Update → Render 루프를 이 안에서 처리
    //    서버가 끊거나 ESC를 누르면 돌아옴
   
    ChattingLogic cChatLogic(sock, strName);
    cChatLogic.Run();

    closesocket(sock);
    WSACleanup();
    return 0;
}