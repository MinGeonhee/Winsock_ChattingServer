// Chatting_Program.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "ChattingRoomManager.h"
#include "ServerEventQueue.h"

const char* welcome = "안녕하세요. 채팅 서버에 오신 걸 환영합니다.\n";

unordered_map<SOCKET, thread> g_umapReceiveWorker{};
ServerEventQueue g_ServerEvents;

bool			 g_IsRunning = true;

/* 이 PC의 IPv4 주소 출력 (클라이언트가 접속할 주소 안내용, WSAStartup 이후 호출) */
void Print_LocalIP()
{
	char szHost[256]{};
	addrinfo tHint{};
	tHint.ai_family = AF_INET;
	tHint.ai_socktype = SOCK_STREAM;
	addrinfo* pResult = nullptr;

	cout << " 이 PC의 IP 주소 (클라이언트에서 이 주소로 접속)" << endl;
	if (gethostname(szHost, sizeof(szHost)) == 0 && getaddrinfo(szHost, nullptr, &tHint, &pResult) == 0)
	{
		for (addrinfo* p = pResult; p != nullptr; p = p->ai_next)
		{
			char ip[INET_ADDRSTRLEN]{};
			inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(p->ai_addr)->sin_addr, ip, sizeof(ip));
			cout << "   - " << ip << endl;
		}
		freeaddrinfo(pResult);
	}
	cout << "   - 127.0.0.1 (같은 PC에서 테스트)" << endl << endl;
}

/* 서버를 열 포트 번호 입력 (Enter = 기본 포트) */
uint16_t Input_Port(uint16_t iDefaultPort)
{
	while (true)
	{
		cout << " 사용할 포트 번호 (1024 ~ 65535, Enter = " << iDefaultPort << ") : ";
		string line;
		getline(cin, line);

		if (line.empty())
			return iDefaultPort;

		if (line.size() <= 5 && line.find_first_not_of("0123456789") == string::npos)
		{
			int iPort = stoi(line);
			if (iPort >= 1024 && iPort <= 65535)
				return static_cast<uint16_t>(iPort);
		}
		cout << " 경고 : 1024 ~ 65535 사이 숫자를 입력해 주세요" << endl << endl;
	}
}
/* Acccpt 함수 Thraed 돌리기 */
void Accept_Sever(SOCKET pSocket)
{
	cout << " 자동 Accept 실행 중.. " << endl;

	while (g_IsRunning)
	{
		SOCKADDR_IN clntAddr{}; // 접속 데이터를 받을 구조체 빈깡통
		int			clntAddrSize = sizeof(clntAddr);

		SOCKET clntSock = accept(pSocket, (SOCKADDR*)(&clntAddr), &clntAddrSize);
		if (clntSock == INVALID_SOCKET) break;

		Server::ServerEvent tEvent{};
		tEvent.eEventType = Server::Event_Type::Connect;
		tEvent.tIPv4 = std::move(clntAddr);
		tEvent.socketSender = clntSock;



		g_ServerEvents.Push_Event(tEvent);
	}

	return;
}

/* 패킷 받는 이벤트 함수 스레드 */
void Receive_Packet(SOCKET pSenderSocket)
{
	if (pSenderSocket == INVALID_SOCKET) return;

	while (g_IsRunning)
	{
		Protocal::Packet::Chat tChat{};
		/* 추후에는 Packet이 꼭 Message라는 보장이 없음 */
		int ren = recv(pSenderSocket, reinterpret_cast<char*>(&tChat), sizeof(tChat), MSG_WAITALL);
		Server::ServerEvent tEvent{};
		tEvent.socketSender = pSenderSocket;
		tEvent.tChat = std::move(tChat);

		if (ren != sizeof(tChat))
		{
			tEvent.eEventType = Server::Event_Type::DisConnect;
			g_ServerEvents.Push_Event(tEvent);
			return;
		}
		else
		{
			tEvent.eEventType = Server::Event_Type::Receive_Packet;
			g_ServerEvents.Push_Event(tEvent);
		}

	}

	return;
}

/* Main이 서버관리 위주 */
int main()
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(hIn, &mode);
	mode |= ENABLE_EXTENDED_FLAGS;        // 이 플래그가 있어야 빠른 편집 설정이 적용됨
	mode &= ~ENABLE_QUICK_EDIT_MODE;      // 빠른 편집(마우스 선택) 끄기
	SetConsoleMode(hIn, mode);

	//	1. Winsock 초기화

	//소켓 라이브러리를 먼저 불러오는 과정 리눅스에는 없는 과정
	//MAKEWORD(2,2) => Winsock 2.2 버전을 사용
	//WSAStartup => 윈도우에서 소켓 기능이 들어 있는 Winsock 라이브러리(ws2_32.dll)를 이 프로그램에서 사용하도록 준비시키고
	//사용할 버전을 맞추는 초기화 과정입니다.
	WSADATA wsaData{};
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << " Win sock 초기화 실패 : " << WSAGetLastError() << endl;
		return 0;
	}

	cout << "--- Win sock 초기화 ---" << endl;
	cout << "State   : " << wsaData.szSystemStatus << endl;
	cout << "Version : " << (int)LOBYTE(wsaData.wVersion) << " . " << (int)HIBYTE(wsaData.wVersion) << " / "
		<< (int)LOBYTE(wsaData.wHighVersion) << " . " << (int)HIBYTE(wsaData.wHighVersion) << endl;
	cout << "----------------------" << endl;
	//////////////////////////////////////////////

	//	2. 소켓 생성하기 (기본 Main 서버는 TCP 소켓)
	//  socket( _In_ int af, _In_ int type, _In_ int protocol);
	//  af : Address Family(주소 체계)의 줄임말 어떤 종류의 주소로 통신할 건지
	//	type => 어떤 타입인지 TCP면 STREAM 방식 UDP 면 SOCK_DGRAM 
	//	프로토콜 방식 => UDP , TCP 중 선택
	SOCKET listenSocket_TCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket_TCP == INVALID_SOCKET)
	{
		cout << " socket 생성 실패 :  " << WSAGetLastError() << endl;
		return 0;
	}
	cout << " TCP Listen Socket 생성 완료 " << endl;
	
	//	3. 생성된 소켓을 이용하여 서버 열기
	Print_LocalIP();
	const uint16_t iServerPort = Input_Port(Protocal::Value::Server_Port);

	SOCKADDR_IN servAddr{};
	servAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	servAddr.sin_family = AF_INET; // Socket을 생성할 때 만들었던 Address Family(주소 체계)와 동일한 값이 들어가야함
	servAddr.sin_port = htons(iServerPort);

	//IP => 어느 컴퓨터 인지 정보
	//포트 번호 => 그 컴퓨터 안의 어느 프로그램인지 정보

	// 생성한 소켓을 우리가정한 1234 번 포트에 묶는 Bind작업
	if (bind(listenSocket_TCP, reinterpret_cast<sockaddr*>(&servAddr), sizeof(servAddr)) == SOCKET_ERROR)
	{
		printf("bind 실패: %d\n", WSAGetLastError());
		return 0;
	}
	cout << " Bind 성공 " << endl;

	//	listen => Bind가 완료가 된 뒤 이제 접속을 허락한다고 운영채제에게 알리는 역할
	//	SOMAXCONN == 운영체제가 정한 최대 대기열 크기를 사용하겠다는 선언
	if (listen(listenSocket_TCP, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("bind 실패: %d\n", WSAGetLastError());
		return 0;
	}

	//	클라이언트 접속 요청 받기
	vector<SOCKET> clntsockets;

	uint64_t iChatRoomNum{ 0 };
	ChattingRoomManager* pChattingManager = ChattingRoomManager::Get_Instance();
	if (pChattingManager == nullptr) return 0;

	//int select(
	//	int nfds,                        // 윈도우에서는 무시됨 (그냥 0)
	//	fd_set * readfds,                // 읽을 게 생겼는지 볼 소켓 목록
	//	fd_set * writefds,               // 보낼 수 있는지 볼 소켓 목록
	//	fd_set * exceptfds,              // 예외가 생겼는지 볼 소켓 목록
	//	const timeval * timeout          // 얼마나 기다릴지
	//);

	/* 한 스레드가 무한으로 Accept 호출을 받아준다 */
	thread			AcceptWorker(Accept_Sever, listenSocket_TCP);

	while (g_IsRunning)
	{
		/* Queue를 계속 처리 */
		while (g_IsRunning)
		{
			Server::ServerEvent tEvent{};
			if (!g_ServerEvents.Pop_Event(tEvent)) break;
			
			if (tEvent.eEventType == Server::Event_Type::Connect) // 서버 연결 요청이라면
			{
				char ip[INET_ADDRSTRLEN];
				clntsockets.push_back(tEvent.socketSender);
#pragma region inet_ntop
				// 컴퓨터가 쓰는 IP 주소를 사라밍 읽는 문자열로 바꿔주는 함수입니다
				// Accept 과정에서 클라이언트의 주소가 컴퓨터 언어로 바뀌는데 이를 다시 번역해주는과정
				// 그래서 IP 주소를 따로 받아온다 sin_addr에 컴퓨터 언어로 IP주소가들어가 있음
				// port 번호도 ntohs 마찬가지로 리틀엔디언으로 변환하여 출력해야한다 
#pragma endregion
				inet_ntop(AF_INET, &tEvent.tIPv4.sin_addr, ip, sizeof(ip));
				cout << "[Server] Client 접속 IP : " << ip << "  Port : " << ntohs(tEvent.tIPv4.sin_port) << endl;

				if (iChatRoomNum == 0)
					iChatRoomNum = pChattingManager->Create_Room(tEvent.socketSender);
				else
					if (pChattingManager->Enter_ChatRoom(iChatRoomNum, tEvent.socketSender) == false)
						break;

				/* Receive 스레드 호출 */
				/* 나중에 Dis Connected 되었을 경우 해당 스레드를 찾아서 삭제해줄 에정 */
				g_umapReceiveWorker[tEvent.socketSender] = thread(Receive_Packet, tEvent.socketSender);
			}
			else if (tEvent.eEventType == Server::Event_Type::Receive_Packet)
			{
				/* 메세지가 도착했다면? */
				/* 해당 메세지를 모든 Client에게 보낸다 */
				pChattingManager->BroadCast_Chat(iChatRoomNum,tEvent.socketSender,tEvent.tChat);
			}
			else if (tEvent.eEventType == Server::Event_Type::DisConnect)
			{
				pChattingManager->Leave_ChatRoom(iChatRoomNum,tEvent.socketSender);
				
				/* 추후 고칠 예정 */
				SOCKADDR_IN addr{};
				int len = sizeof(addr);
				char ip[INET_ADDRSTRLEN] = "unknown";
				int port = 0;

				if (getpeername(tEvent.socketSender, (SOCKADDR*)&addr, &len) == 0)
				{
					inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));   // tEvent.tIPv4 → addr
					port = ntohs(addr.sin_port);                          // 포트도 addr에서
				}



				/* 스레드 삭제 */
				if (g_umapReceiveWorker[tEvent.socketSender].joinable())
					g_umapReceiveWorker[tEvent.socketSender].join();
				g_umapReceiveWorker.erase(tEvent.socketSender);

				/* 서버에 저장된 해당 클라이언트 소켓 삭제 */
				for (auto iter = clntsockets.begin(); iter != clntsockets.end(); ++iter)
				{
					if ((*iter) == tEvent.socketSender)
					{
						clntsockets.erase(iter);
						break;
					}
				}

				cout << "[Server] Client IP : " << ip << "  Port : " << port << "   접속 해제" << endl;
			}
		}
	}

	AcceptWorker.join();

	/* 프로그램이 끝나면 소켓 삭제 */
	for (auto& Socket : clntsockets)
		closesocket(Socket);

	//끝난 뒤 깔끔하게 정리
	closesocket(listenSocket_TCP);
	WSACleanup();

	return 0;
}