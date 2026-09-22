#ifndef DATABASE_CACCOUNTREPOSITORY_H
#define DATABASE_CACCOUNTREPOSITORY_H

#include <cstdint>
#include <string>
#include <vector>

struct CStoredAccount
{
	std::string m_strAccount;
	std::vector<std::uint8_t> m_password;
	std::int64_t m_nCreateTime{ 0 };
	std::int64_t m_nUpdateTime{ 0 };
	std::int64_t m_nLastSuccessTime{ 0 };
};

class CAccountRepository final
{
  public:
	std::vector<CStoredAccount> LoadAccounts() const;
	bool SaveAccount(const std::string& strAccount, const std::vector<std::uint8_t>& password, std::int64_t nTimestamp) const;
	bool DeleteAccount(const std::string& strAccount) const;
};

#endif
