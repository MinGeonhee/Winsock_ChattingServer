#pragma once
#include "Enum.h"
#include "../../Protocol/Struct.h"

namespace Server
{
	/* Server Event 최상위 구조체 */
	struct ServerEvent
	{
		Server::Event_Type	eEventType{Server::Event_Type::END};
		SOCKET				socketSender{};		
		SOCKET				socketReceiver{};


		/* Connected 일때 */
		SOCKADDR_IN			tIPv4{};

		/* Receive_Packet 일때만 */
		Protocal::Packet::Chat tChat{};

	};
}