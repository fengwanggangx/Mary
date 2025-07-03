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
		buffer.clear();
		struct evbuffer* input = bufferevent_get_input(pEvent);
		std::size_t nLength = evbuffer_get_length(input);
		if (nLength > buffer.capacity())
		{
			buffer.resize(std::ceil(nLength * 1.5)); // 增加50%的余量
		}
		std::size_t n = evbuffer_remove(input, buffer.data(), nLength);
		return n;
	}
}