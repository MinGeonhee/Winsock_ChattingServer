#pragma once


namespace Server
{
	enum class Event_Type
	{
		Connect = 0,
		Receive_Packet = 1,
		DisConnect = 2,
		END,
	};
}