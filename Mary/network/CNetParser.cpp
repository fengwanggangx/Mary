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

		// 处理数据
		std::cout << "Received: " << n << " bytes" << std::endl;

		// 如果数据是文本，可以这样打印
		std::cout << "Data: " << std::string(buffer.data(), n) << std::endl;

		// 准备响应
		const char* response = "Server has received your message";
		bufferevent_write(pEvent, response, strlen(response));
		return n;
	}
}