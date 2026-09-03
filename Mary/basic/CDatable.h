#ifndef __CDataTable_H__
#define __CDataTable_H__

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>


using _TyCellData = std::variant<std::int32_t, std::int64_t, double, bool, char, std::string>;
using _TyColumnData = std::variant<std::vector<std::int32_t>, std::vector<std::int64_t>, std::vector<double>, std::vector<bool>, std::vector<char>, std::vector<std::string>>;

struct CColumnInfo
{
	CColumnInfo(unsigned int nID, const std::string& strColName) : m_id(nID), m_strName(strColName), m_tHash(std::hash<std::string>{}(GetColumnDataType()))
	{
	}

	CColumnInfo(unsigned int nID, const std::string& strColName,  _TyColumnData&& data) : m_id(nID), m_strName(strColName), m_data(std::move(data)), m_tHash(std::hash<std::string>{}(GetColumnDataType()))
	{
	}

	std::string GetColumnDataType() const
	{
		return std::visit([](const auto& vec) {
			using _Ty = typename std::decay_t<decltype(vec)>::value_type;
			if constexpr (std::is_same_v<_Ty, std::int32_t>) return "int32";
			else if constexpr (std::is_same_v<_Ty, std::int64_t>) return "int64";
			else if constexpr (std::is_same_v<_Ty, double>) return "double";
			else if constexpr (std::is_same_v<_Ty, bool>) return "bool";
			else if constexpr (std::is_same_v<_Ty, char>) return "char";
			else if constexpr (std::is_same_v<_Ty, std::string>) return "string";
			return "unknown";
			}, m_data);
	}

	std::size_t GetDataCount() const 
	{
		return std::visit([](const auto& vec) { return vec.size(); }, m_data);
	}

	template<typename _Ty>
	bool IsType() const
	{
		using _TyVec = std::vector<_Ty>;
		return std::holds_alternative<_TyVec>(m_data);
	}

	unsigned int m_id{ 0 };
	std::string m_strName;
	_TyColumnData m_data;
	std::size_t m_tHash{ 0 };
};

class CDataTable
{
public:
	CDataTable() = default;
	CDataTable(const CDataTable&) = delete;
	CDataTable& operator=(const CDataTable&) = delete;
	CDataTable(CDataTable&&) = default;
	CDataTable& operator=(CDataTable&&) = default;

	template<typename _Ty>
	bool AddColumn(unsigned int nId, const std::string& strColName, std::vector<_Ty>&& data)
	{
		if ((m_name_idx.count(strColName) > 0) || (m_id_idx.count(nId) > 0))
		{
			return false;
		}

		if (!CheckColDataSize(data.size()))
		{
			return false;
		}

		m_columns.emplace_back(std::make_unique<CColumnInfo>(strColName, _TyColumnData(std::move(data))));
		m_name_idx[strColName] = m_columns.size() - 1;
		m_id_idx[nId] = m_columns.size() - 1;
		return true;
	}

	bool CheckColDataSize(std::size_t sz)
	{
		if (m_nRowCount <= 0)
		{
			m_nRowCount = sz;
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
		if ((m_name_idx.count(strColName) > 0) || (m_id_idx.count(nId) > 0))
		{
			return false;
		}

		m_columns.emplace_back(std::make_unique<CColumnInfo>(nId, strColName));
		m_name_idx[strColName] = m_columns.size() - 1;
		m_id_idx[nId] = m_columns.size() - 1;
		return true;
	}

	template<typename _Ty>
	bool SetIdxColumnData(std::size_t nIdx, std::vector<_Ty>&& data)
	{
		if ((nIdx >= m_columns.size()) || (nullptr == m_columns[nIdx]))
		{
			return false;
		}

		if (!CheckColDataSize(data.size()))
		{
			return false;
		}

		m_columns[nIdx]->m_data = std::forward<std::vector<_Ty>>(data);
		return true;
	}

	template<typename _Ty>
	bool SetColumnData(std::size_t nId, std::vector<_Ty>&& data)
	{
		if (m_id_idx.count(nId) <= 0)
		{
			return false;
		}
		return SetIdxColumnData(m_id_idx.at(nId), std::forward<std::vector<_Ty>>(data));
	}

	template<typename _Ty>
	bool SetColumnData(const std::string& strColName , std::vector<_Ty>&& data)
	{
		if (m_name_idx.count(strColName) <= 0)
		{
			return false;
		}
		return SetIdxColumnData(m_name_idx.at(strColName), std::forward<std::vector<_Ty>>(data));
	}

	std::size_t GetColumnCount() const
	{
		return m_columns.size();
	}

	std::size_t GetRowCount() const
	{
		return m_nRowCount;
	}

	bool IsHasColumn(const std::string& strName) const
	{
		return m_name_idx.count(strName) > 0;
	}

	bool IsHasColumn(unsigned int nId) const
	{
		return m_id_idx.count(nId) > 0;
	}

	std::vector<std::string> GetColumnNames() const
	{
		std::vector<std::string> names;
		names.reserve(m_columns.size());
		for (const auto& col : m_columns)
		{
			names.emplace_back(col->m_strName);
		}
		return names;
	}

	std::vector<unsigned int> GetColumnIds() const
	{
		std::vector<unsigned int> ids;
		ids.reserve(m_columns.size());
		for (const auto& col : m_columns)
		{
			ids.emplace_back(col->m_id);
		}
		return ids;
	}

	template<typename _Ty>
	std::vector<_Ty>& GetColumnData(const std::string& strName)
	{
		static std::vector<_Ty> data;
		const auto& it = m_name_idx.find(strName);
		if (it == m_name_idx.end()) 
		{
			return data;
		}

		auto& column = m_columns[it->second];
		if (!column->IsType<_Ty>())
		{
			return data;
		}

		return std::get<std::vector<_Ty>>(column->m_data);
	}

	template<typename _Ty>
	const std::vector<_Ty>& GetColumnData(const std::string& strName) const
	{
		return const_cast<CDataTable*>(this)->GetColumnData<_Ty>(strName);
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
			std::visit([&](const auto& vec) {
				row.emplace_back(vec[nIdx]);
				}, 
				col->m_data);
		}
		return row;
	}

	bool AddRow(const std::vector<_TyCellData>& row)
	{
		std::size_t nColCount = m_columns.size();
		if (row.size() != nColCount)
		{
			return false;
		}

		for (std::size_t i = 0; i < nColCount; ++i)
		{
			const auto& col = m_columns[i];
			const auto& data = row[i];

			std::visit([&](auto&& arg) {
				using _Ty = std::decay_t<decltype(arg)>;
				if (col->IsType<_Ty>()) 
				{
					std::get<std::vector<_Ty>>(col->m_data).emplace_back(arg);
				}		
				}, 
				data);
		}

		++m_nRowCount;
		return true;
	}

	template<typename _Ty>
	void SortByColumn(const std::string& strColName, bool bAsc)
	{
		auto& col_data = GetColumnData<_Ty>(strColName);
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
				[&](std::size_t a, std::size_t b) {
				return col_data[a] < col_data[b];
				});
		}
		else {
			std::sort(
				indices.begin(),
				indices.end(),
				[&](std::size_t a, std::size_t b) {
				return col_data[a] > col_data[b];
				});
		}

		ReorderRows(indices);
	}

	void Clear()
	{
		m_columns.clear();
		m_name_idx.clear();
		m_nRowCount = 0;
	}

private:
	void ReorderRows(const std::vector<std::size_t>& order) 
	{
		if (order.size() != m_nRowCount)
		{
			return;
		}

		for (auto& col : m_columns)
		{
			std::visit([&](auto&& vec) {
				using _Ty = typename std::decay_t<decltype(vec)>::value_type;
				std::vector<_Ty> new_data(m_nRowCount);

				for (std::size_t i = 0; i < m_nRowCount; ++i)
				{
					new_data[i] = vec[order[i]];
				}

				vec = std::move(new_data);
				}, 
				col->m_data);
		}
	}


	private:
		std::vector<std::unique_ptr<CColumnInfo>> m_columns;
		std::unordered_map<std::string, std::size_t> m_name_idx;
		std::unordered_map<unsigned int, std::size_t> m_id_idx;
		std::size_t m_nRowCount = 0;
};
#endif
