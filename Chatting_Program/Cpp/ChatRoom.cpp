#include "pch.h"
#include "ChatRoom.h"

bool ChatRoom::Initialize()
{

	return true;
}

bool ChatRoom::Enter(SOCKET pUser)
{
	if (pUser == INVALID_SOCKET) return false;
	
	/* 이미 현재 방에 접속해있는 사람인지 */
	if (m_umapUserData.find(pUser) != m_umapUserData.end())
		return false;

	/* 방에 첫 진입시 방장 부여 */
	if (m_socketRoomMater == 0 && m_umapUserData.size() == 0)
		m_socketRoomMater = pUser;

	/* 용량이 많아지면 Static아니면 힙 영역에 올리는게 좋을듯 추후에 생각 */
	m_umapUserData.emplace(pUser,vector<Protocal::Packet::Chat>());

	/* 해당 User에게 방에 입장했다는 알림 */
	Protocal::Packet::Chat tChat{};
	sprintf_s(tChat.szChat, "채팅 방에 입장 하였습니다");
	tChat.iChatSize = static_cast<uint16_t>(strlen(tChat.szChat));

	/* 서버 이름과 이름 */
	strcpy_s(tChat.szName,g_szServerName);
	tChat.iNameSize = static_cast<uint16_t>(strlen(g_szServerName));

	tChat.iTime[static_cast<int>(Protocal::TIME::HOUR)] = 1;
	tChat.iTime[static_cast<int>(Protocal::TIME::MINUTE)] = 1;
	tChat.iTime[static_cast<int>(Protocal::TIME::SECOND)] = 1;

	/* 보내기 */
	if (send(pUser, reinterpret_cast<char*>(&tChat), sizeof(tChat), 0) == SOCKET_ERROR)
		return false;
	///////////////
	return true;
}

bool ChatRoom::Leave(SOCKET pUser)
{
	if (pUser == SOCKET_ERROR) return false;

	/* User와 통신을 끊어 버린다 */
	m_umapUserData.erase(pUser);

	return true;
}

void ChatRoom::BroadCast_Chat(SOCKET pSender,const Protocal::Packet::Chat& tChat)
{
	Protocal::Packet::Chat copyChat = tChat;

	/* 채팅내역 업데이트 */
	m_umapUserData[pSender].push_back(copyChat);
	m_vecChatLog.push_back(copyChat);

	/* 해당 ChatLog에서 추가된 부분만 Send해주면될려나 */
	for (const auto& Pair : m_umapUserData)
	{
		const SOCKET& socket = Pair.first; //SOKET 보낸 사람도 받기
		
		if (send(socket, reinterpret_cast<char*>(&copyChat), sizeof(copyChat), 0) == SOCKET_ERROR)
		{
			cout << "전송 오류" << endl;
			break;
		}
	}

	return;
}
