#ifndef __CLUALOADER_H__
#define __CLUALOADER_H__
#include <memory>
#include <windows.h>

class CLuaLoader final
{
public:
	static std::shared_ptr<CLuaLoader> Instance();

private:
	CLuaLoader();
	~CLuaLoader();

	CLuaLoader(const CLuaLoader&) = delete;
	CLuaLoader& operator=(const CLuaLoader&) = delete;

private:
	bool InitEnvironment();
	void InitFunctionPtr();

private:
	HMODULE m_handle{ nullptr };
};
#endif
