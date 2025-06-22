#ifndef __CDBENGINEx_H__
#define __CDBENGINEx_H__

#include <string>
#include <unordered_map>
#include "./common/ISingleton.h"
#include "CSQLite3.h"
#include "CMySQL.h"
#include "COracle.h"
#include "CODBC.h"

enum class sqlite
{
	system = 0,
	userdata
};

enum class mysql
{
	system = 0,
	userdata
};

enum class oracle
{
	system = 0,
	userdata
};

class CDBEngine final : public ISingleton<CDBEngine>
{
	DECLARE_SINGLE_DFAULT(CDBEngine)
public:
	db::CSQLite3* Connect(sqlite ty);
	db::CMySQL* Connect(mysql ty);
	db::COracle* Connect(oracle ty);

	int Close(sqlite ty);
	int Close(mysql ty);
	int Close(oracle ty);

	int Close(db::database ty);

	void InitConnectParam();

private:
	std::string GetConnectParam(sqlite ty);
	std::string GetConnectParam(mysql ty);
	std::string GetConnectParam(oracle ty);

private:
	std::unordered_map<sqlite, std::string> m_sqlite;
	std::unordered_map<mysql, std::string> m_mysql;
	std::unordered_map<oracle, std::string> m_oracle;
};
#endif
