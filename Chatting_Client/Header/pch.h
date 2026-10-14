#pragma once

// STD
// C++ STD
#include <iostream>
#include <cmath>


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

#include <thread>
#include <mutex>


#include <Enum.h>
#include <Struct.h> 
#include "../../Protocol/Enum.h"
#include "../../Protocol/Struct.h"

using namespace Client;
using namespace std;



inline int CountChars(const string& str)
{
    // 변환에 필요한 와이드 문자 개수를 구함 (CP_ACP = 한국어 윈도우 기본 코드페이지, 보통 CP949)
    return MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
}