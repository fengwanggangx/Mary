#include "CAccountRepository.h"

#include "CDBEngine.h"
#include "CSQLite3.h"


std::vector<CAccountInfo> CAccountStorager::LoadAccounts() const
{
	std::vector<CAccountInfo> accounts;
	db::_TyDBPtr db = CDBEngine::InstanceRef().GetDBPtr(db::em_database::sqlite);
	db::CSQLite3::_TySqlRows rows;
	if (0 != db->ExecQueryPrepared("SELECT account, password, create_time, update_time, last_success_time FROM account ORDER BY last_success_time DESC, account ASC", {}, rows))
	{
		return accounts;
	}
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
	std::string strSQL = "INSERT INTO account(account, password, create_time, update_time, last_success_time) VALUES(?, ?, ?, ?, ?) ON CONFLICT(account) DO UPDATE SET password=excluded.password, update_time=excluded.update_time, last_success_time=excluded.last_success_time";
	db::CSQLite3::_TySqlParameters parameters{ strAccount, password, nTimestamp, nTimestamp, nTimestamp };
	return 0 == db->ExecUpdatePrepared(strSQL, parameters);
}

bool CAccountStorager::DeleteAccount(const std::string& strAccount) const
{
	db::_TyDBPtr db = CDBEngine::InstanceRef().GetDBPtr(db::em_database::sqlite);
	return 0 == db->ExecUpdatePrepared("DELETE FROM account WHERE account=?", { strAccount });
}
