#pragma once


/* 멀티 스레드 환경에서 안전하게 쓰기 위한 Queue */

class ServerEventQueue
{
public:
	ServerEventQueue() : m_Queue{}, m_mutex{}
	{}
	~ServerEventQueue() {}
public:
	void				Push_Event(const Server::ServerEvent& tEvent);
	bool				Pop_Event(Server::ServerEvent& outEvent);
	bool				Is_Empty() { return m_Queue.empty(); }
private:
	mutex						m_mutex;
	queue<Server::ServerEvent>	m_Queue;
};

