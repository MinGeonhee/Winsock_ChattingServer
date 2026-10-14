#include "pch.h"
#include "ChattingRoomManager.h"
#include "ChatRoom.h"

ChattingRoomManager* ChattingRoomManager::m_pthis = nullptr;

uint64_t ChattingRoomManager::Create_Room(SOCKET pUser)
{
	/* RoomCounter 대입 후 증가 연산 */
	uint64_t iIndex = ++m_iChatRoomCounter;

	ChatRoom* pChatRoom = ChatRoom::Create();
	if (pChatRoom == nullptr)
		return g_iMaxChatRoomCount;

	/* 방 만들고 입장 시키기 */
	pChatRoom->Enter(pUser);

	m_umapChatRooms[iIndex] = pChatRoom;
	
	return iIndex;
}

ChattingRoomManager::ChattingRoomManager()
	: m_iChatRoomCounter{0} , m_umapChatRooms{}
{
}

bool ChattingRoomManager::Clear()
{
	/* 채팅룸 내역 정리 */
	for (auto [iIndex, pChatRoom] : m_umapChatRooms)
	{
		if (pChatRoom == nullptr)
			delete pChatRoom;
		pChatRoom = nullptr;
	}
	m_umapChatRooms.clear();

	return true;
}

bool ChattingRoomManager::Enter_ChatRoom(uint64_t iRoomIndedxNum, SOCKET pUser)
{
	if (m_umapChatRooms.find(iRoomIndedxNum) == m_umapChatRooms.end()) return false;
	
	return m_umapChatRooms[iRoomIndedxNum]->Enter(pUser);
}

bool ChattingRoomManager::Leave_ChatRoom(uint64_t iRoomIndedxNum, SOCKET pUser)
{
	if (m_umapChatRooms.find(iRoomIndedxNum) == m_umapChatRooms.end()) return false;

	return m_umapChatRooms[iRoomIndedxNum]->Leave(pUser);
}


void ChattingRoomManager::BroadCast_Chat(uint64_t iChatRoomNum,SOCKET pSender ,const Protocal::Packet::Chat& tChat)
{
	if (m_umapChatRooms.find(iChatRoomNum) == m_umapChatRooms.end())
		return;

	m_umapChatRooms[iChatRoomNum]->BroadCast_Chat(pSender,tChat);
}
