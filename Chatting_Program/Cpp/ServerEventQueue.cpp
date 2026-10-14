#include "pch.h"
#include "ServerEventQueue.h"


void ServerEventQueue::Push_Event(const Server::ServerEvent& tEvent)
{
	m_mutex.lock();
	m_Queue.push(tEvent);
	m_mutex.unlock();
}

bool ServerEventQueue::Pop_Event(Server::ServerEvent& outEvent)
{
	m_mutex.lock();

	if (m_Queue.empty())
	{
		m_mutex.unlock();
		return false;
	}

	outEvent = std::move(m_Queue.front());
	m_Queue.pop();
	m_mutex.unlock();

	return true;
}
