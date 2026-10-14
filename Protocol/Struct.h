#pragma once
#include <cstdint>
#include "Enum.h"

namespace Protocal 
{

	namespace Value
	{
		constexpr uint16_t Server_Port = 16623;               // 서버가 bind한 포트와 같아야 함
		constexpr uint16_t MaxChatSize = 256;
		constexpr uint16_t MaxNameSize = 32;
	}

	namespace Packet
	{
#pragma pack(push, 1)
		struct Chat
		{
			uint16_t	iTime[static_cast<int>(Protocal::TIME::END)];	// 시간
			uint16_t	iChatSize{ 0 };									// Chat Size
			uint16_t	iNameSize{ 0 };									// 이름 사이즈
			char		szName[Value::MaxNameSize]{};					// 보낸 사람 이름
			char		szChat[Value::MaxChatSize]{};					// 보낸 내용
		};
#pragma pack(pop)
	}
}