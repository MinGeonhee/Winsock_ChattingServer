#pragma once

/* 채팅방 하나 = 서버 전체 나중에 마피아 = 서버 전체 로직에서 관리하는 마피아 방하나로 추가 구현 */
/* ChatRoom Manager 추가하면 좋을듯? */


class ChatRoom
{
private:
	ChatRoom() 
		: m_socketRoomMater{0}, m_umapUserData{}, m_vecChatLog{}, m_iRoomNumber{}
	{}

public:
	virtual ~ChatRoom() {}
private:
	bool Initialize();
public:
	/* 소켓번호가 필요할까? */
	bool Enter(SOCKET pUser);
	bool Leave(SOCKET pUser);

	/* 해당 방에 있는 사람들 모두에게 메세지 뿌리기 */
	void BroadCast_Chat(SOCKET pSender,const Protocal::Packet::Chat& tChat );

public:
	static ChatRoom* Create()
	{
		ChatRoom* pChatRoom = new ChatRoom;
		if (pChatRoom == nullptr) return nullptr;

		if (pChatRoom->Initialize() == false)	return nullptr;

		return pChatRoom;
	}
private:
	size_t			m_iRoomNumber; // Room 번호
	SOCKET			m_socketRoomMater;
	/* 채팅 로그 관리 */
	unordered_map<SOCKET, vector<Protocal::Packet::Chat>> m_umapUserData;//사람별 채팅 내역
	vector<Protocal::Packet::Chat> m_vecChatLog; //시간에 따라서 Sort할 에정
};

