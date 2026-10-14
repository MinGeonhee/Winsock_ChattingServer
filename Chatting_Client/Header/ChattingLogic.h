#pragma once
// 콘솔 채팅 화면 (간단 버전)
// - 수신 스레드는 받은 Chat을 큐에 넣기만 하고 화면에는 손대지 않는다
// - 화면은 메인 스레드의 Render 한 곳에서만 그린다
// - 위쪽: 채팅 내역 / 아래 두 줄: 구분선 + 입력 줄

class ChattingLogic
{
public:
    ChattingLogic(SOCKET sock, const std::string& name);
    ~ChattingLogic();

    ChattingLogic(const ChattingLogic&) = delete;
    ChattingLogic& operator=(const ChattingLogic&) = delete;

    // 서버와 연결이 끊기거나 ESC를 누를 때까지 Input → Update → Render 반복
    void Run();

private:
    void RecvThread();
    void Input();
    void Update();
    void Render();

    void SendChat(const std::wstring& text);
    void AddLog(const std::string& line);

    // 화면 출력 도우미
    void ClearScreen();
    void WriteAt(int x, int y, const std::string& text);

    // 문자열 도우미 (CP949 기준: 한글 2바이트 = 화면 2칸)
    static std::string              ToMultiByte(const std::wstring& w);
    static std::string              Pad(const std::string& s, int width);
    static std::vector<std::string> Wrap(const std::string& s, int maxCols);
    static void                     RemoveFirstChar(std::string& s);

private:
    SOCKET              m_sock;
    std::string         m_name;

    std::thread         m_recvWorker;
    std::atomic<bool>   m_running{ true };

    // 수신 스레드 → 메인 스레드
    std::mutex                          m_recvMutex;
    std::queue<Protocal::Packet::Chat>  m_recvQueue;

    // 메인 스레드만 사용
    std::deque<std::string> m_log;          // 화면에 보여줄 채팅 내역
    std::wstring            m_input;        // 입력 중인 글자 (한글도 1칸)
    bool                    m_dirty = true; // 다시 그릴 필요가 있는지

    HANDLE  m_hOut;
    int     m_recvCount = 0;
    int     m_frameCount=0;
    int     m_cols = 0;   // 창 가로 칸 수
    int     m_rows = 0;   // 창 세로 줄 수
    int     m_left = 0;   // 창이 버퍼에서 시작하는 위치
    int     m_top = 0;
};