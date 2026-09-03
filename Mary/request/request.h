#ifndef MARY_REQUEST_REQUEST_H
#define MARY_REQUEST_REQUEST_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace request
{
	class RequestData;
}

class CRequest final
{
public:
	enum class Type
	{
		UNKNOWN = 0,
		QUERY_AUTH = 1,
		QUERY_USERINFO = 2,
		UPDATE_AUTH = 3,
		UPDAT_PRODUCT = 4,
		HQMARKET = 5
	};

	CRequest();
	~CRequest();
	CRequest(const CRequest& arg);
	CRequest& operator=(const CRequest& arg);
	CRequest(CRequest&&) noexcept;
	CRequest& operator=(CRequest&&) noexcept;

	std::uint64_t GetId() const;
	void SetId(std::uint64_t id);
	Type GetType() const;
	void SetType(Type type);
	std::string GetCmd() const;
	void SetCmd(const std::string& strCmd);
	std::string GetExtraData(const std::string& strKey) const;
	std::unordered_map<std::string, std::string> GetExtraData() const;
	void SetExtraData(const std::string& strKey, const std::string& strValue);
	std::string GetReturnData(const std::string& strKey) const;
	std::unordered_map<std::string, std::string> GetReturnData() const;
	void SetReturnData(const std::string& strKey, const std::string& strValue);

	void SetConnectionId(std::int64_t id);
	std::int64_t GetConnectionId() const;
	void SetFd(std::int64_t id);
	std::int64_t GetFd() const;
	bool Serialize(std::string* pOutput) const;
	bool Deserialize(const std::string& strData);

private:
	std::unique_ptr<request::RequestData> m_data;
	std::int64_t m_connectionId{ -1 };
};

#endif
