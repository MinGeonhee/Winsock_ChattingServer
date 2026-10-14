# Winsock Chatting Server

Winsock TCP와 C++로 만든 콘솔 기반 1:N 채팅 서버 / 클라이언트입니다.
마피아 게임 서버([Winsock_MafiaServer](https://github.com/MinGeonhee/Winsock_MafiaServer))를 만들기 전 단계로, TCP 통신과 멀티스레드 구조를 익히기 위해 제작했습니다.

## 구조

- **Accept 스레드**: 새 접속을 받아 `Connect` 이벤트로 큐에 전달
- **수신 스레드** (클라이언트 1명당 1개): 패킷을 받아 `Receive_Packet` / `DisConnect` 이벤트로 전달
- **ServerEventQueue**: mutex로 보호되는 이벤트 큐 (스레드 간 유일한 공유 지점)
- **main 스레드**: 큐에서 이벤트를 꺼내 방 입장, 브로드캐스트, 퇴장 처리

## 실행

1. `Chatting_Program.sln`을 Visual Studio 2022로 열고 x64로 빌드
2. 서버 실행 → 표시되는 IP를 확인하고 포트 입력 (Enter = 기본 포트)
3. 클라이언트 실행 → 닉네임, 서버 IP, 포트 입력
