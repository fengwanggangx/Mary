#include "CNetParser.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
namespace net
{
	int CNetParser::BufferEventReader(struct bufferevent* pEvent, std::vector<char>& buffer)
	{
		evbuffer* input = bufferevent_get_input(pEvent);
		size_t nLength = evbuffer_get_length(input);
		if (nLength > buffer.capacity())
		{
			buffer.resize(std::ceil(nLength * 1.5)); // 增加50%的余量
		}
		size_t n = evbuffer_remove(input, buffer.data(), nLength);
		if (n <= 0)
		{
			return false;
		}
		// 确保字符串以'\0'结尾（如果需要将数据视为字符串）
		if (n < buffer.size())
		{
			buffer[n] = '\0';
		}
		return n;
	}
}