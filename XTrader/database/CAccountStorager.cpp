#include "CAccountStorager.h"

#include "CDBEngine.h"
#include "IDataBase.h"

namespace
{
	std::string QuoteText(const std::string& strValue)
	{
		std::string strResult("'");
		strResult.reserve(strValue.size() + 2);
		for (char ch : strValue)
		{
			strResult.push_back(ch);
			if ('\'' == ch)
			{
				strResult.push_back(ch);
			}
		}
		strResult.push_back('\'');
		return strResult;
	}

	std::string BlobLiteral(const std::vector<std::uint8_t>& value)
	{
		static constexpr char HEX[] = "0123456789ABCDEF";
		std::string strResult("X'");
		strResult.reserve(value.size() * 2 + 3);
		for (std::uint8_t byte : value)
		{
			strResult.push_back(HEX[byte >> 4]);
			strResult.push_back(HEX[byte & 0x0F]);
		}
		strResult.push_back('\'');
		return strResult;
	}
} // namespace


std::vector<CAccountInfo> CAccountStorager::LoadAccounts() const
{
	std::string strSql = "SELECT account, password, create_time, update_time, last_success_time FROM account ORDER BY last_success_time DESC, account ASC";

	db::_TyDBPtr db = CDBEngine::InstanceRef().GetDBPtr(db::em_database::sqlite);
	const db::CQueryTable& table = db->ExecQuery(strSql);
	if (table.IsEmpty())
	{
		return {};
	}

	const db::CQueryTable::_TyRows& rows = table.m_rows;
	std::vector<CAccountInfo> accounts;
	accounts.reserve(rows.size());
	for (const auto& row : rows)
	{
		if ((5 != row.size()) || (nullptr == std::get_if<std::string>(&row[0])) || (nullptr == std::get_if<std::vector<std::uint8_t>>(&row[1])) || (nullptr == std::get_if<std::int64_t>(&row[2])) || (nullptr == std::get_if<std::int64_t>(&row[3])) || (nullptr == std::get_if<std::int64_t>(&row[4])))
		{
			continue;
		}
		CAccountInfo& account = accounts.emplace_back();
		account.m_strAccount = std::get<std::string>(row[0]);
		account.m_password = std::get<std::vector<std::uint8_t>>(row[1]);
		account.m_nCreateTime = std::get<std::int64_t>(row[2]);
		account.m_nUpdateTime = std::get<std::int64_t>(row[3]);
		account.m_nLastSuccessTime = std::get<std::int64_t>(row[4]);
	}
	return accounts;
}

bool CAccountStorager::SaveAccount(const std::string& strAccount, const std::vector<std::uint8_t>& password, std::int64_t nTimestamp) const
{
	if (strAccount.empty() || password.empty())
	{
		return false;
	}
	db::_TyDBPtr db = CDBEngine::InstanceRef().GetDBPtr(db::em_database::sqlite);
	std::string strTimestamp = std::to_string(nTimestamp);
	std::string strSQL = "INSERT INTO account(account,password,create_time,update_time,last_success_time) VALUES(" + QuoteText(strAccount) + "," + BlobLiteral(password) + "," + strTimestamp + "," + strTimestamp + "," + strTimestamp + ") ON CONFLICT(account) DO UPDATE SET password=excluded.password,update_time=excluded.update_time,last_success_time=excluded.last_success_time";
	return 0 == db->ExecUpdate(strSQL);
}

bool CAccountStorager::DeleteAccount(const std::string& strAccount) const
{
	db::_TyDBPtr db = CDBEngine::InstanceRef().GetDBPtr(db::em_database::sqlite);
	return 0 == db->ExecUpdate("DELETE FROM account WHERE account=" + QuoteText(strAccount));
}
