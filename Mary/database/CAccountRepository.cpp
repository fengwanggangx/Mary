#include "CAccountRepository.h"

#include "CDBEngine.h"
#include "CSQLite3.h"

namespace
{
	db::CSQLite3* GetDataBase(db::_TyDBPtr& database)
	{
		database = CDBEngine::InstanceRef().GetDBPtr(db::em_database::sqlite);
		return nullptr == database ? nullptr : dynamic_cast<db::CSQLite3*>(database.get());
	}
} // namespace

std::vector<CStoredAccount> CAccountRepository::LoadAccounts() const
{
	std::vector<CStoredAccount> accounts;
	db::_TyDBPtr database;
	db::CSQLite3* pDatabase = GetDataBase(database);
	if (nullptr == pDatabase)
	{
		return accounts;
	}

	db::CSQLite3::_TySqlRows rows;
	if (0 != pDatabase->ExecQueryPrepared("SELECT account, password, create_time, update_time, last_success_time FROM account ORDER BY last_success_time DESC, account ASC", {}, rows))
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
		CStoredAccount& account = accounts.emplace_back();
		account.m_strAccount = std::get<std::string>(row[0]);
		account.m_password = std::get<std::vector<std::uint8_t>>(row[1]);
		account.m_nCreateTime = std::get<std::int64_t>(row[2]);
		account.m_nUpdateTime = std::get<std::int64_t>(row[3]);
		account.m_nLastSuccessTime = std::get<std::int64_t>(row[4]);
	}
	return accounts;
}

bool CAccountRepository::SaveAccount(const std::string& strAccount, const std::vector<std::uint8_t>& password, std::int64_t nTimestamp) const
{
	if (strAccount.empty() || password.empty())
	{
		return false;
	}
	db::_TyDBPtr database;
	db::CSQLite3* pDatabase = GetDataBase(database);
	if (nullptr == pDatabase)
	{
		return false;
	}
	std::string strSQL = "INSERT INTO account(account, password, create_time, update_time, last_success_time) VALUES(?, ?, ?, ?, ?) ON CONFLICT(account) DO UPDATE SET password=excluded.password, update_time=excluded.update_time, last_success_time=excluded.last_success_time";
	db::CSQLite3::_TySqlParameters parameters{ strAccount, password, nTimestamp, nTimestamp, nTimestamp };
	return 0 == pDatabase->ExecUpdatePrepared(strSQL, parameters);
}

bool CAccountRepository::DeleteAccount(const std::string& strAccount) const
{
	db::_TyDBPtr database;
	db::CSQLite3* pDatabase = GetDataBase(database);
	if ((nullptr == pDatabase) || strAccount.empty())
	{
		return false;
	}
	return 0 == pDatabase->ExecUpdatePrepared("DELETE FROM account WHERE account=?", { strAccount });
}
