#ifndef __CDataTable_H__
#define __CDataTable_H__

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using _TyCellData = std::variant<std::int32_t, std::int64_t, double, bool, char, std::string>;
using _TyColumnData = std::variant<std::vector<std::int32_t>, std::vector<std::int64_t>, std::vector<double>, std::vector<bool>, std::vector<char>, std::vector<std::string>>;

struct CColumnInfo
{
	CColumnInfo(unsigned int nID, const std::string& strColName) : m_id(nID), m_strName(strColName), m_tHash(std::hash<std::string>{}(GetColumnDataType()))
	{
	}

	CColumnInfo(unsigned int nID, const std::string& strColName, _TyColumnData&& data) : m_id(nID), m_strName(strColName), m_data(std::move(data)), m_tHash(std::hash<std::string>{}(GetColumnDataType()))
	{
	}

	std::string GetColumnDataType() const
	{
		return std::visit([](const auto& vec)
						  {
			using _Ty = typename std::decay_t<decltype(vec)>::value_type;
			if constexpr (std::is_same_v<_Ty, std::int32_t>)
			{
				return "int32";
			}
			else if constexpr (std::is_same_v<_Ty, std::int64_t>)
			{
				return "int64";
			}
			else if constexpr (std::is_same_v<_Ty, double>)
			{
				return "double";
			}
			else if constexpr (std::is_same_v<_Ty, bool>)
			{
				return "bool";
			}
			else if constexpr (std::is_same_v<_Ty, char>)
			{
				return "char";
			}
			else if constexpr (std::is_same_v<_Ty, std::string>)
			{
				return "string";
			}
			else
			{
				return "unknown";
			} }, m_data);
	}

	std::size_t GetDataCount() const
	{
		return std::visit([](const auto& vec)
						  { return vec.size(); }, m_data);
	}

	template <typename _Ty>
	bool IsType() const
	{
		using _TyVec = std::vector<_Ty>;
		return std::holds_alternative<_TyVec>(m_data);
	}

	unsigned int m_id{0};
	std::string m_strName;
	_TyColumnData m_data;
	std::size_t m_tHash{0};
};

class CDataTable
{
  public:
	CDataTable() = default;
	CDataTable(const CDataTable&) = delete;
	CDataTable& operator=(const CDataTable&) = delete;
	CDataTable(CDataTable&&) = default;
	CDataTable& operator=(CDataTable&&) = default;

	template <typename _Ty>
	bool AddColumn(unsigned int nId, const std::string& strColName, std::vector<_Ty>&& data)
	{
		if ((0 < m_name_idx.count(strColName)) || (0 < m_id_idx.count(nId)))
		{
			return false;
		}

		if (!CheckColDataSize(data.size()))
		{
			return false;
		}

		m_columns.emplace_back(nId, strColName, _TyColumnData(std::move(data)));
		m_name_idx[strColName] = m_columns.size() - 1;
		m_id_idx[nId] = m_columns.size() - 1;
		return true;
	}

	bool CheckColDataSize(std::size_t sz)
	{
		if (!m_isRowCountInitialized)
		{
			m_nRowCount = sz;
			m_isRowCountInitialized = true;
			for (CColumnInfo& column : m_columns)
			{
				std::visit([sz](auto& values)
						   { values.resize(sz); }, column.m_data);
			}
		}
		else
		{
			if (m_nRowCount != sz)
			{
				return false;
			}
		}
		return true;
	}

	bool AddColumn(unsigned int nId, const std::string& strColName)
	{
		if ((0 < m_name_idx.count(strColName)) || (0 < m_id_idx.count(nId)))
		{
			return false;
		}

		m_columns.emplace_back(nId, strColName);
		if (m_isRowCountInitialized)
		{
			std::get<std::vector<std::int32_t>>(m_columns.back().m_data).resize(m_nRowCount);
		}
		m_name_idx[strColName] = m_columns.size() - 1;
		m_id_idx[nId] = m_columns.size() - 1;
		return true;
	}

	template <typename _Ty>
	bool SetIdxColumnData(std::size_t nIdx, std::vector<_Ty>&& data)
	{
		if (m_columns.size() <= nIdx)
		{
			return false;
		}

		if (!CheckColDataSize(data.size()))
		{
			return false;
		}

		m_columns[nIdx].m_data = std::move(data);
		m_columns[nIdx].m_tHash = std::hash<std::string>{}(m_columns[nIdx].GetColumnDataType());
		return true;
	}

	template <typename _Ty>
	bool SetColumnData(std::size_t nId, std::vector<_Ty>&& data)
	{
		auto it = m_id_idx.find(nId);
		if (m_id_idx.end() == it)
		{
			return false;
		}
		return SetIdxColumnData(it->second, std::move(data));
	}

	template <typename _Ty>
	bool SetColumnData(const std::string& strColName, std::vector<_Ty>&& data)
	{
		auto it = m_name_idx.find(strColName);
		if (m_name_idx.end() == it)
		{
			return false;
		}
		return SetIdxColumnData(it->second, std::move(data));
	}

	std::size_t GetColumnCount() const
	{
		return m_columns.size();
	}

	std::size_t GetRowCount() const
	{
		return m_nRowCount;
	}

	void ReserveRows(std::size_t rowCount)
	{
		for (CColumnInfo& column : m_columns)
		{
			std::visit([rowCount](auto& values)
					   { values.reserve(rowCount); }, column.m_data);
		}
	}

	bool IsHasColumn(const std::string& strName) const
	{
		return 0 < m_name_idx.count(strName);
	}

	bool IsHasColumn(unsigned int nId) const
	{
		return 0 < m_id_idx.count(nId);
	}

	std::vector<std::string> GetColumnNames() const
	{
		std::vector<std::string> names;
		names.reserve(m_columns.size());
		for (const auto& col : m_columns)
		{
			names.emplace_back(col.m_strName);
		}
		return names;
	}

	std::vector<unsigned int> GetColumnIds() const
	{
		std::vector<unsigned int> ids;
		ids.reserve(m_columns.size());
		for (const auto& col : m_columns)
		{
			ids.emplace_back(col.m_id);
		}
		return ids;
	}

	template <typename _Ty>
	std::vector<_Ty>& GetColumnData(const std::string& strName)
	{
		auto it = m_name_idx.find(strName);
		if (m_name_idx.end() == it)
		{
			throw std::out_of_range("CDataTable column does not exist: " + strName);
		}

		CColumnInfo& column = m_columns[it->second];
		if (!column.IsType<_Ty>())
		{
			throw std::bad_variant_access();
		}

		return std::get<std::vector<_Ty>>(column.m_data);
	}

	template <typename _Ty>
	const std::vector<_Ty>& GetColumnData(const std::string& strName) const
	{
		auto it = m_name_idx.find(strName);
		if (m_name_idx.end() == it)
		{
			throw std::out_of_range("CDataTable column does not exist: " + strName);
		}

		const CColumnInfo& column = m_columns[it->second];
		if (!column.IsType<_Ty>())
		{
			throw std::bad_variant_access();
		}

		return std::get<std::vector<_Ty>>(column.m_data);
	}

	std::vector<_TyCellData> GetRowData(std::size_t nIdx) const
	{
		if (nIdx >= m_nRowCount)
		{
			return {};
		}

		std::vector<_TyCellData> row;
		row.reserve(m_columns.size());
		for (const auto& col : m_columns)
		{
			std::visit([&](const auto& vec)
					   { row.emplace_back(vec[nIdx]); }, col.m_data);
		}
		return row;
	}

	bool AddRow(const std::vector<_TyCellData>& row)
	{
		std::size_t nColCount = m_columns.size();
		if ((0 == nColCount) || (row.size() != nColCount))
		{
			return false;
		}

		for (std::size_t i = 0; i < nColCount; ++i)
		{
			bool isTypeMatched = std::visit([&](const auto& value)
											{
				using _Ty = std::decay_t<decltype(value)>;
				return m_columns[i].IsType<_Ty>(); }, row[i]);
			if (!isTypeMatched)
			{
				return false;
			}
		}

		std::size_t appendedCount = 0;
		try
		{
			for (; appendedCount < nColCount; ++appendedCount)
			{
				std::visit([&](const auto& value)
						   {
					using _Ty = std::decay_t<decltype(value)>;
					std::get<std::vector<_Ty>>(m_columns[appendedCount].m_data).emplace_back(value); }, row[appendedCount]);
			}
		}
		catch (...)
		{
			for (std::size_t i = 0; i < appendedCount; ++i)
			{
				std::visit([](auto& values)
						   { values.pop_back(); }, m_columns[i].m_data);
			}
			throw;
		}

		++m_nRowCount;
		m_isRowCountInitialized = true;
		return true;
	}

	template <typename _Ty>
	void SortByColumn(const std::string& strColName, bool bAsc)
	{
		std::vector<_Ty>& columnData = GetColumnData<_Ty>(strColName);
		std::vector<std::size_t> indices(m_nRowCount);

		for (std::size_t i = 0; i < m_nRowCount; ++i)
		{
			indices[i] = i;
		}

		if (bAsc)
		{
			std::sort(
				indices.begin(),
				indices.end(),
				[&](std::size_t a, std::size_t b)
				{
					return columnData[a] < columnData[b];
				});
		}
		else
		{
			std::sort(
				indices.begin(),
				indices.end(),
				[&](std::size_t a, std::size_t b)
				{
					return columnData[a] > columnData[b];
				});
		}

		ReorderRows(indices);
	}

	void Clear()
	{
		m_columns.clear();
		m_name_idx.clear();
		m_id_idx.clear();
		m_nRowCount = 0;
		m_isRowCountInitialized = false;
	}

  private:
	void ReorderRows(const std::vector<std::size_t>& order)
	{
		if (order.size() != m_nRowCount)
		{
			return;
		}

		for (CColumnInfo& column : m_columns)
		{
			std::visit([&](auto& values)
					   {
				using _Ty = typename std::decay_t<decltype(values)>::value_type;
				std::vector<_Ty> newData;
				newData.reserve(m_nRowCount);

				for (std::size_t i = 0; i < m_nRowCount; ++i)
				{
					if constexpr (std::is_same_v<_Ty, std::string>)
					{
						newData.emplace_back(std::move(values[order[i]]));
					}
					else
					{
						newData.emplace_back(values[order[i]]);
					}
				}

				values = std::move(newData); }, column.m_data);
		}
	}

  private:
	std::vector<CColumnInfo> m_columns;
	std::unordered_map<std::string, std::size_t> m_name_idx;
	std::unordered_map<unsigned int, std::size_t> m_id_idx;
	std::size_t m_nRowCount = 0;
	bool m_isRowCountInitialized{false};
};
#endif
