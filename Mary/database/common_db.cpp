#include "common_db.h"

#include <mysql/field_types.h>
#include <vector>

#include "../common/utility.h"

namespace db
{
	CConnectParam::CConnectParam(const std::string& host, unsigned int port, const std::string& account, const std::string& password, const std::string& database, const std::string& charset)
		: m_strHost(host), m_nPort(port), m_strAccount(account), m_strPasswd(password), m_strDataBase(database), m_strCharset(charset)
	{
	}

	CConnectParam::CConnectParam(const std::string& parameter, char delimiter)
	{
		std::vector<std::string> data;
		unsigned int port = 0;
		if ((6 == utility::split(parameter, data, delimiter, true)) && utility::to_number(data[1], port))
		{
			m_strHost = data[0];
			m_nPort = port;
			m_strAccount = data[2];
			m_strPasswd = data[3];
			m_strDataBase = data[4];
			m_strCharset = data[5];
		}
	}

	std::string GetDBName(em_database databaseType)
	{
		switch (databaseType)
		{
		case em_database::mysql:
			return "mysql";
		case em_database::oracle:
			return "oracle";
		case em_database::sqlite:
			return "sqlite";
		default:
			return "";
		}
	}

	em_database GetDBType(const std::string& name)
	{
		if ("mysql" == name)
		{
			return em_database::mysql;
		}
		if ("oracle" == name)
		{
			return em_database::oracle;
		}
		if ("sqlite" == name)
		{
			return em_database::sqlite;
		}
		return em_database::unknown;
	}

	em_data_types GetDataType(em_database databaseType, int nativeType)
	{
		if (em_database::mysql != databaseType)
		{
			return em_data_types::em_string;
		}
		switch (nativeType)
		{
		case MYSQL_TYPE_TINY:
		case MYSQL_TYPE_SHORT:
		case MYSQL_TYPE_INT24:
		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_YEAR:
			return em_data_types::em_int32;
		case MYSQL_TYPE_LONGLONG:
			return em_data_types::em_int64;
		case MYSQL_TYPE_DECIMAL:
		case MYSQL_TYPE_NEWDECIMAL:
		case MYSQL_TYPE_FLOAT:
		case MYSQL_TYPE_DOUBLE:
			return em_data_types::em_double;
		case MYSQL_TYPE_BOOL:
		case MYSQL_TYPE_BIT:
			return em_data_types::em_bool;
		case MYSQL_TYPE_TINY_BLOB:
		case MYSQL_TYPE_MEDIUM_BLOB:
		case MYSQL_TYPE_LONG_BLOB:
		case MYSQL_TYPE_BLOB:
			return em_data_types::binary;
		default:
			return em_data_types::em_string;
		}
	}
} // namespace db
