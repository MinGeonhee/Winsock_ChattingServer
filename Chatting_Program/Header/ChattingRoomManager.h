#pragma once


/* 채팅 방을 관리할 매니저 */

class ChatRoom;

class ChattingRoomManager
{
private:
	ChattingRoomManager();
	ChattingRoomManager(const ChattingRoomManager& Copy) = delete;
	const ChattingRoomManager& operator=(const ChattingRoomManager& Copy) = delete;
	~ChattingRoomManager(){
		/* 채팅룸 내역 정리 */
		Clear();
	}
private:
	bool Clear();
public:
	bool Enter_ChatRoom(uint64_t iRoomIndedxNum,SOCKET pUser);
	bool Leave_ChatRoom(uint64_t iRoomIndedxNum,SOCKET pUser);
	void BroadCast_Chat(uint64_t iChatRoomNum, SOCKET pSender, const Protocal::Packet::Chat& tChat);
public:
	static ChattingRoomManager* Get_Instance()
	{
		if (m_pthis == nullptr)
		{
			m_pthis = new ChattingRoomManager;

			/* 초기화 해주기 */
			if (!m_pthis->Initialize())
			{
				cout << " 채팅 룸 초기화 실패 " << endl;
				return nullptr;
			}
		}

		cout << " 채팅 룸 초기화 완료 " << endl;
		return m_pthis;
	}
	void	Destroy()
	{
		if (m_pthis != nullptr)
			delete m_pthis;

		m_pthis = nullptr;
	}
public:
	/* 서버에 채팅 보내기 */
	uint64_t	Create_Room(SOCKET pUser);
public:
	bool Initialize()
	{
		bool is = Clear();

		return is;
	}
private:
	static ChattingRoomManager* m_pthis;
private:
	/* 채팅 내역 */
	uint64_t							 m_iChatRoomCounter;
	unordered_map<uint64_t, ChatRoom*> m_umapChatRooms{};
};

