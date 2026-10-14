#pragma once

// C++ STD
#include <iostream>
#include <cmath>

#include <thread>
#include <mutex>
#include <atomic>

// STL
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <string>
#include <queue>

//Server
#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>

#include <limits.h>

//서버 프로그램에서 사용
#include "Enum.h"
#include "Struct.h" 

//공용 프로토콜
#include "../../Protocol/Enum.h"
#include "../../Protocol/Struct.h"

/* 서버에서 메세지 보낼 때 사용할 이름 */
constexpr char		g_szServerName[Protocal::Value::MaxNameSize]{'S','e','r','v','e','r'};
constexpr uint16_t	g_iServerNameSize = 6;
constexpr uint64_t	g_iMaxChatRoomCount = ULLONG_MAX;


using namespace std;

