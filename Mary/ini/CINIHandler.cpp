#include "CINIHandler.h"

#include <array>
#include <filesystem>
#include <utility>

namespace ini
{
	namespace
	{
		using ConfigFile = std::pair<Config, std::filesystem::path>;

		const std::array<ConfigFile, 1> configFiles{
			ConfigFile{Config::System, std::filesystem::path("ini") / "system.ini"}};
	} // namespace

	CINIHandler::CINIHandler()
	{
		Load();
	}

	CINIHandler::~CINIHandler() = default;

	std::vector<std::pair<std::string, std::string>> CINIHandler::GetSection(Config config, const std::string &section) const
	{
		auto iter = m_iniFiles.find(config);
		if (m_iniFiles.end() == iter)
		{
			return {};
		}
		return iter->second->GetSection(section);
	}

	bool CINIHandler::UpdateEntry(Config config, const std::string &section, const std::string &key, const std::optional<std::string> &value, const std::string &oldKey)
	{
		auto iter = m_iniFiles.find(config);
		return m_iniFiles.end() != iter && iter->second->UpdateEntry(section, key, value, oldKey);
	}

	bool CINIHandler::Load()
	{
		if (!m_iniFiles.empty())
		{
			return true;
		}

		for (const ConfigFile &configFile : configFiles)
		{
			m_iniFiles.emplace(configFile.first, std::make_unique<CIniFile>(configFile.second.string()));
		}
		return true;
	}
} // namespace ini
