#ifndef __CNETPARSER_H__
#define __CNETPARSER_H__
#include <vector>

struct bufferevent;
namespace net
{
	class CNetParser
	{
	public:
		static int BufferEventReader(struct bufferevent* pEvent, std::vector<char>& buffer);
	};
}
#endif
