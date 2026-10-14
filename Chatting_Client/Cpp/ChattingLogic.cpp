#include "pch.h"
#include "ChattingLogic.h"

/* 현재 시간으로부터 */
#include <conio.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace
{
    constexpr int     MAX_INPUT_CHARS = 100;   // 입력 글자 수 제한
    constexpr size_t  MAX_LOG_LINES = 200;   // 화면용 내역 보관 개수

    constexpr wint_t  KEY_ENTER = L'\r';
    constexpr wint_t  KEY_BACK = L'\b';
    constexpr wint_t  KEY_ESC = 27;
}

ChattingLogic::ChattingLogic(SOCKET sock, const std::string& name)
    : m_sock(sock), m_name(name), m_hOut(GetStdHandle(STD_OUTPUT_HANDLE))
{
}

ChattingLogic::~ChattingLogic()
{
    m_running = false;
    if (m_recvWorker.joinable())
    {
        shutdown(m_sock, SD_BOTH);   // recv에서 기다리는 중이면 깨움
        m_recvWorker.join();
    }
}

/* ----------------------------------------------------------------------------
   메인 루프
---------------------------------------------------------------------------- */
void ChattingLogic::Run()
{
    m_recvWorker = std::thread(&ChattingLogic::RecvThread, this);

    while (m_running)
    {
        Input();
        Update();
        Render();

        // 디버그용: 창 제목에 수신 개수와 루프 횟수 표시
        ++m_frameCount;
        if (m_frameCount % 10 == 0)   // 10프레임(약 0.1초)마다 한 번만 갱신
        {
            wchar_t title[64];
            swprintf_s(title, L"recv %d / frame %d", m_recvCount, m_frameCount);
            SetConsoleTitleW(title);
        }

        Sleep(10);
    }

    // 수신 스레드가 recv에서 멈춰 있으면 깨워서 끝나게 한다
    // (소켓을 닫는 건 이 클래스를 만든 쪽에서 closesocket으로)
    shutdown(m_sock, SD_BOTH);
    if (m_recvWorker.joinable())
        m_recvWorker.join();

    if (m_rows > 0)
        WriteAt(0, m_rows - 1, Pad("> 연결이 종료되었습니다. 아무 키나 누르면 종료합니다", m_cols - 1));
    _getwch();
}

/* ----------------------------------------------------------------------------
   수신 스레드: 받아서 큐에 넣기만 한다. 화면 출력 금지
---------------------------------------------------------------------------- */
void ChattingLogic::RecvThread()
{
    while (m_running)
    {
        Protocal::Packet::Chat tChat{};
        int len = recv(m_sock, reinterpret_cast<char*>(&tChat), sizeof(tChat), MSG_WAITALL);

        if (len != sizeof(tChat))   // 0: 서버 종료, -1: 끊김, 그 외: 받다가 끊김
        {
            m_running = false;
            return;
        }

        std::lock_guard<std::mutex> lock(m_recvMutex);
        m_recvQueue.push(tChat);
        ++m_recvCount;
    }
}

/* ----------------------------------------------------------------------------
   Input: 키를 하나씩 받아서 입력 버퍼를 직접 관리 (콘솔 에코 없음)
---------------------------------------------------------------------------- */
void ChattingLogic::Input()
{
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    DWORD count = 0;
    if (!GetNumberOfConsoleInputEvents(hIn, &count) || count == 0)
        return;                                   // 쌓인 입력이 없으면 바로 돌아감 (멈추지 않음)

    INPUT_RECORD records[64];
    DWORD read = 0;
    if (!ReadConsoleInputW(hIn, records, 64, &read))   // 쌓여 있는 것만 읽으므로 기다리지 않음
        return;

    for (DWORD i = 0; i < read; ++i)
    {
        const INPUT_RECORD& rec = records[i];
        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown)
            continue;                             // 키를 누른 이벤트만 처리 (마우스, 포커스 등은 무시)

        const WORD    vk = rec.Event.KeyEvent.wVirtualKeyCode;
        const wchar_t ch = rec.Event.KeyEvent.uChar.UnicodeChar;

        if (vk == VK_ESCAPE)
        {
            m_running = false;
            return;
        }
        if (vk == VK_RETURN)
        {
            if (!m_input.empty())
            {
                SendChat(m_input);
                m_input.clear();
            }
        }
        else if (vk == VK_BACK)
        {
            if (!m_input.empty())
                m_input.pop_back();
        }
        else if (ch >= 32)                        // 글자가 실제로 들어온 경우만 (한글 완성 글자 포함)
        {
            if (static_cast<int>(m_input.size()) >= MAX_INPUT_CHARS)
                continue;

            std::wstring next = m_input + ch;
            if (ToMultiByte(next).size() >= Protocal::Value::MaxChatSize)
                continue;

            m_input = std::move(next);
        }
        else
        {
            continue;                             // 글자가 없는 키(Shift, 방향키 등)는 화면 갱신 불필요
        }

        m_dirty = true;
    }
}

/* ----------------------------------------------------------------------------
   Update: 받은 메시지를 내역에 반영 + 창 크기 변화 확인
---------------------------------------------------------------------------- */
void ChattingLogic::Update()
{
    // 큐를 통째로 가져와서 잠금 시간을 최소화
    std::queue<Protocal::Packet::Chat> local;
    {
        std::lock_guard<std::mutex> lock(m_recvMutex);
        std::swap(local, m_recvQueue);
    }

    while (!local.empty())
    {
        Protocal::Packet::Chat& c = local.front();

        // 네트워크로 받은 문자열은 끝이 막혀 있다고 믿지 않는다
        c.szName[sizeof(c.szName) - 1] = '\0';
        c.szChat[sizeof(c.szChat) - 1] = '\0';

        const int h = c.iTime[static_cast<int>(Protocal::TIME::HOUR)];
        const int m = c.iTime[static_cast<int>(Protocal::TIME::MINUTE)];
        const int s = c.iTime[static_cast<int>(Protocal::TIME::SECOND)];

        std::string line;
        if (h != 0 || m != 0 || s != 0)   // 시간이 채워진 메시지만 시간 표시
        {
            char timeBuf[16];
            sprintf_s(timeBuf, "[%02d:%02d] ", h, m);
            line = timeBuf;
        }
        line += c.szName;
        line += " : ";
        line += c.szChat;

        AddLog(line);
        local.pop();
    }

    // 창 크기나 스크롤 위치가 바뀌었으면 화면을 지우고 전부 다시 그림
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(m_hOut, &info))
    {
        const int cols = info.srWindow.Right - info.srWindow.Left + 1;
        const int rows = info.srWindow.Bottom - info.srWindow.Top + 1;

        if (cols != m_cols || rows != m_rows ||
            info.srWindow.Left != m_left || info.srWindow.Top != m_top)
        {
            m_cols = cols;
            m_rows = rows;
            m_left = info.srWindow.Left;
            m_top = info.srWindow.Top;
            ClearScreen();
            m_dirty = true;
        }
    }
}

/* ----------------------------------------------------------------------------
   Render: 바뀐 게 있을 때만, 정해진 위치에 덮어쓰기 (지우지 않음 → 깜빡임 없음)
---------------------------------------------------------------------------- */
void ChattingLogic::Render()
{
    if (!m_dirty || m_cols < 4 || m_rows < 3)
        return;

    const int width = m_cols - 1;   // 마지막 칸에 쓰면 자동 줄바꿈이 생겨서 한 칸 비움
    const int chatRows = m_rows - 2;   // 아래 두 줄은 구분선과 입력 줄

    // 1. 최신 메시지부터 거꾸로, 화면 폭에 맞게 줄바꿈하면서 채팅 영역만큼 모으기
    std::vector<std::string> lines;
    for (auto it = m_log.rbegin(); it != m_log.rend() && static_cast<int>(lines.size()) < chatRows; ++it)
    {
        std::vector<std::string> wrapped = Wrap(*it, width);
        for (auto w = wrapped.rbegin(); w != wrapped.rend() && static_cast<int>(lines.size()) < chatRows; ++w)
            lines.push_back(*w);
    }
    std::reverse(lines.begin(), lines.end());

    // 2. 채팅 영역: 최신 메시지가 입력 줄 바로 위에 오도록 위쪽을 빈 줄로 채움
    const int emptyRows = chatRows - static_cast<int>(lines.size());
    for (int y = 0; y < chatRows; ++y)
    {
        const std::string& line = (y < emptyRows) ? std::string() : lines[y - emptyRows];
        WriteAt(0, y, Pad(line, width));
    }

    // 3. 구분선
    WriteAt(0, chatRows, std::string(width, '-'));

    // 4. 입력 줄: 길면 앞부분을 잘라서 끝부분만 보여줌
    std::string input = ToMultiByte(m_input);
    const int   avail = width - 2;
    while (static_cast<int>(input.size()) > avail)
        RemoveFirstChar(input);

    WriteAt(0, chatRows + 1, Pad("> " + input, width));

    // 5. 커서를 입력 끝으로
    COORD cursor{ static_cast<SHORT>(m_left + 2 + static_cast<int>(input.size())),
                  static_cast<SHORT>(m_top + chatRows + 1) };
    SetConsoleCursorPosition(m_hOut, cursor);

    m_dirty = false;
}

/* ----------------------------------------------------------------------------
   전송
---------------------------------------------------------------------------- */
void ChattingLogic::SendChat(const std::wstring& text)
{
    Protocal::Packet::Chat tChat{};

    SYSTEMTIME st;
    GetLocalTime(&st);
    tChat.iTime[static_cast<int>(Protocal::TIME::HOUR)] = st.wHour;
    tChat.iTime[static_cast<int>(Protocal::TIME::MINUTE)] = st.wMinute;
    tChat.iTime[static_cast<int>(Protocal::TIME::SECOND)] = st.wSecond;

    strncpy_s(tChat.szName, m_name.c_str(), _TRUNCATE);
    tChat.iNameSize = static_cast<uint16_t>(strlen(tChat.szName));

    const std::string msg = ToMultiByte(text);
    strncpy_s(tChat.szChat, msg.c_str(), _TRUNCATE);
    tChat.iChatSize = static_cast<uint16_t>(strlen(tChat.szChat));

    if (send(m_sock, reinterpret_cast<const char*>(&tChat), sizeof(tChat), 0) == SOCKET_ERROR)
        m_running = false;
}

void ChattingLogic::AddLog(const std::string& line)
{
    m_log.push_back(line);
    if (m_log.size() > MAX_LOG_LINES)
        m_log.pop_front();
    m_dirty = true;
}

/* ----------------------------------------------------------------------------
   화면 출력 도우미
---------------------------------------------------------------------------- */
void ChattingLogic::ClearScreen()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(m_hOut, &info))
        return;

    const DWORD cells = static_cast<DWORD>(info.dwSize.X) * info.dwSize.Y;
    const COORD home{ 0, 0 };
    DWORD written = 0;
    FillConsoleOutputCharacterA(m_hOut, ' ', cells, home, &written);
    FillConsoleOutputAttribute(m_hOut, info.wAttributes, cells, home, &written);
}

void ChattingLogic::WriteAt(int x, int y, const std::string& text)
{
    COORD pos{ static_cast<SHORT>(m_left + x), static_cast<SHORT>(m_top + y) };
    SetConsoleCursorPosition(m_hOut, pos);

    DWORD written = 0;
    WriteConsoleA(m_hOut, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
}

/* ----------------------------------------------------------------------------
   문자열 도우미
   CP949 콘솔에서는 한글 1자 = 2바이트 = 화면 2칸이라 바이트 수를 그대로 칸 수로 쓴다
---------------------------------------------------------------------------- */
std::string ChattingLogic::ToMultiByte(const std::wstring& w)
{
    if (w.empty())
        return std::string();

    const int len = WideCharToMultiByte(CP_ACP, 0, w.c_str(), static_cast<int>(w.size()),
        nullptr, 0, nullptr, nullptr);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), static_cast<int>(w.size()),
        &out[0], len, nullptr, nullptr);
    return out;
}

std::string ChattingLogic::Pad(const std::string& s, int width)
{
    std::string out = s;
    while (static_cast<int>(out.size()) > width)
        out.pop_back();                       // Render에서 Wrap을 거친 문자열만 들어오므로 거의 일어나지 않음
    out.append(static_cast<size_t>(width) - out.size(), ' ');
    return out;
}

std::vector<std::string> ChattingLogic::Wrap(const std::string& s, int maxCols)
{
    std::vector<std::string> out;
    std::string cur;

    for (size_t i = 0; i < s.size();)
    {
        // 한글의 첫 바이트면 두 바이트를 한 덩어리로 (글자 중간에서 자르지 않기 위해)
        const size_t charLen =
            (IsDBCSLeadByte(static_cast<BYTE>(s[i])) && i + 1 < s.size()) ? 2 : 1;

        if (static_cast<int>(cur.size() + charLen) > maxCols)
        {
            out.push_back(cur);
            cur.clear();
        }
        cur.append(s, i, charLen);
        i += charLen;
    }

    if (!cur.empty() || out.empty())
        out.push_back(cur);

    return out;
}

void ChattingLogic::RemoveFirstChar(std::string& s)
{
    if (s.empty())
        return;

    const size_t charLen = (IsDBCSLeadByte(static_cast<BYTE>(s[0])) && s.size() >= 2) ? 2 : 1;
    s.erase(0, charLen);
}