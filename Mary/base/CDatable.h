#ifndef __CDataTable_H__
#define __CDataTable_H__

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <unordered_map>
#include <stdexcept>


using _TyCellData = std::variant<std::int32_t, std::int64_t, double, bool, char, std::string>;
using _TyColumnData = std::variant<std::vector<std::int32_t>, std::vector<std::int64_t>, std::vector<double>, std::vector<bool>, std::vector<char>, std::vector<std::string>>;

struct CColumnInfo
{
	unsigned int m_id{ 0 };
	std::string m_strName;
	_TyColumnData m_data;

	std::size_t m_tHash{ 0 };

	CColumnInfo(unsigned int nID, const std::string& strColName,  _TyColumnData&& data) : m_id(nID), m_strName(strColName), m_data(std::move(data)), m_tHash(std::hash<std::string>{}(get_type_name()))
	{
	}

	// 获取类型名称
	std::string get_type_name() const
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

	// 获取列大小
	std::size_t size() const 
	{
		return std::visit([](const auto& vec) { return vec.size(); }, m_data);
	}

	// 检查是否可以安全地转换为指定类型
	template<typename _Ty>
	bool IsType() const
	{
		using _TyVec = std::vector<_Ty>;
		return std::holds_alternative<_TyVec>(data);
	}
};

class CDataTable
{
public:
	CDataTable() = default;
	CDataTable(const CDataTable&) = delete;
	CDataTable& operator=(const CDataTable&) = delete;
	CDataTable(CDataTable&&) = default;
	CDataTable& operator=(CDataTable&&) = default;

	// 添加列
	template<typename _Ty>
	bool add_column(const std::string& strColName, std::vector<_Ty>&& data)
	{
		if (m_name_idx.count(strColName))
		{
			return false;
		}

		// 检查数据大小是否与现有行计数一致
		if (!m_columns.empty() && data.size() != m_nRowCount)
		{
			return false;
		}

		m_columns.emplace_back(std::make_unique<CColumnInfo>(strColName, _TyColumnData(std::move(data))));
		m_name_idx[strColName] = m_columns.size() - 1;

		if (m_columns.size() == 1)
		{
			m_nRowCount = m_columns[0]->size();
		}
	}

	// 获取列数
	std::size_t GetColumnCount() const
	{
		return m_columns.size();
	}

	// 获取行数
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

	// 获取列数据（类型安全）
	template<typename _Ty>
	std::vector<_Ty>& GetColumnData(const std::string& strName)
	{
		const auto& it = m_name_idx.find(strName);
		if (it == m_name_idx.end()) 
		{
			return {};
		}

		auto& column = m_columns[it->second];
		if (!column->IsType<_Ty>())
		{
			return {};
		}

		return std::get<std::vector<_Ty>>(column->m_data);
	}

	template<typename _Ty>
	const std::vector<_Ty>& GetColumnData(const std::string& strName) const
	{
		return const_cast<CDataTable*>(this)->GetColumnData<_Ty>(strName);
	}

	// 获取行数据（以变体向量形式）
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
			std::visit([&](const auto& vec){
				row.emplace_back(vec[nIdx]);
				}, 
				col->m_data);
		}

		return row;
	}

	// 添加一行数据
	bool AddRow(const std::vector<_TyCellData>& row_data)
	{
		if (row_data.size() != m_columns.size())
		{
			return false;
		}

		for (std::size_t i = 0; i < m_columns.size(); ++i)
		{
			const auto& col = m_columns[i];
			const auto& data = row_data[i];

			std::visit([&](auto&& arg) {
				using _Ty = std::decay_t<decltype(arg)>;
				if (col->IsType<_Ty>()) 
				{
					std::get<std::vector<_Ty>>(col->m_data).emplace_back(arg);
				}			
				}, 
				data);
		}

		m_nRowCount++;
	}

	// 按列排序（高性能实现）
	template<typename _Ty>
	void SortByColumn(const std::string& strName, bool ascending = true)
	{
		auto& col_data = GetColumnData<_Ty>(strName);
		std::vector<std::size_t> indices(m_nRowCount);

		// 创建索引序列
		for (std::size_t i = 0; i < m_nRowCount; ++i)
		{
			indices[i] = i;
		}

		// 根据列数据排序索引
		if (ascending)
		{
			std::sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
				return col_data[a] < col_data[b];
				});
		}
		else {
			std::sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
				return col_data[a] > col_data[b];
				});
		}

		// 重排所有列
		reorder_rows(indices);
	}

	void Clear()
	{
		m_columns.clear();
		m_name_idx.clear();
		m_nRowCount = 0;
	}

private:
	// 根据索引重排行
	void reorder_rows(const std::vector<std::size_t>& new_order) 
	{
		if (new_order.size() != m_nRowCount)
		{
			throw std::invalid_argument("New order size does not match row count");
		}

		// 为每列创建新数据
		for (auto& col : m_columns)
		{
			std::visit([&](auto&& vec) {
				using _Ty = typename std::decay_t<decltype(vec)>::value_type;
				std::vector<_Ty> new_data(m_nRowCount);

				for (std::size_t i = 0; i < m_nRowCount; ++i)
				{
					new_data[i] = vec[new_order[i]];
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
