#include "CDataTableModel.h"

#include <QVariant>

#include <algorithm>
#include <limits>
#include <type_traits>

CDataTableModel::CDataTableModel(QObject* pParent) : QAbstractTableModel(pParent)
{
}

int CDataTableModel::rowCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : static_cast<int>(m_view.GetRowCount());
}

int CDataTableModel::columnCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : static_cast<int>(m_view.GetColumnCount());
}

QVariant CDataTableModel::data(const QModelIndex& index, int nRole) const
{
	if (!index.isValid() || (Qt::DisplayRole != nRole) || (0 > index.row()) || (0 > index.column()))
	{
		return QVariant();
	}
	const std::vector<CDataColumnSchema>& schema = m_view.GetSchema();
	if ((m_view.GetRowCount() <= static_cast<std::size_t>(index.row())) || (schema.size() <= static_cast<std::size_t>(index.column())))
	{
		return QVariant();
	}
	_TyDataValue value;
	if (!m_view.GetValue(static_cast<_TyDataRowIndex>(index.row()), schema[static_cast<std::size_t>(index.column())].m_id, value))
	{
		return QVariant();
	}
	return ToVariant(value);
}

QVariant CDataTableModel::headerData(int nSection, Qt::Orientation orientation, int nRole) const
{
	if ((Qt::DisplayRole != nRole) || (Qt::Horizontal != orientation) || (0 > nSection))
	{
		return QVariant();
	}
	const std::vector<CDataColumnSchema>& schema = m_view.GetSchema();
	return schema.size() <= static_cast<std::size_t>(nSection) ? QVariant() : QVariant(QString::fromStdString(schema[static_cast<std::size_t>(nSection)].m_strName));
}

_TyDataRowId CDataTableModel::GetRowId(int nRow) const
{
	const std::vector<_TyDataRowId>& rowIds = m_view.GetRowIds();
	return (0 > nRow) || (rowIds.size() <= static_cast<std::size_t>(nRow)) ? 0 : rowIds[static_cast<std::size_t>(nRow)];
}

const CDataTableView& CDataTableModel::GetView() const noexcept
{
	return m_view;
}

void CDataTableModel::SetView(CDataTableView view, const CDataChangeSet& changes)
{
	_TyDataVersion nPreviousVersion = m_view.GetVersion();
	if (!m_view.IsValid() || changes.m_bStructureChanged || (m_view.GetColumnCount() != view.GetColumnCount()) || (m_view.GetRowCount() != view.GetRowCount()))
	{
		beginResetModel();
		m_view = std::move(view);
		endResetModel();
		return;
	}
	m_view = std::move(view);
	if ((0 != nPreviousVersion) && (nPreviousVersion + 1 < changes.m_version))
	{
		if ((0 < rowCount()) && (0 < columnCount()))
		{
			emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1), { Qt::DisplayRole });
		}
		return;
	}
	if (changes.m_changedCells.empty())
	{
		return;
	}
	int nMinRow = (std::numeric_limits<int>::max)();
	int nMaxRow = -1;
	int nMinColumn = (std::numeric_limits<int>::max)();
	int nMaxColumn = -1;
	const std::vector<CDataColumnSchema>& schema = m_view.GetSchema();
	for (const CDataCellChange& change : changes.m_changedCells)
	{
		_TyDataRowIndex row = 0;
		if (!m_view.FindRow(change.m_rowId, row))
		{
			continue;
		}
		for (std::size_t column = 0; column < schema.size(); ++column)
		{
			if (change.m_columnId == schema[column].m_id)
			{
				nMinRow = (std::min)(nMinRow, static_cast<int>(row));
				nMaxRow = (std::max)(nMaxRow, static_cast<int>(row));
				nMinColumn = (std::min)(nMinColumn, static_cast<int>(column));
				nMaxColumn = (std::max)(nMaxColumn, static_cast<int>(column));
				break;
			}
		}
	}
	if ((0 <= nMaxRow) && (0 <= nMaxColumn))
	{
		emit dataChanged(index(nMinRow, nMinColumn), index(nMaxRow, nMaxColumn), { Qt::DisplayRole });
	}
}

QVariant CDataTableModel::ToVariant(const _TyDataValue& value) const
{
	return std::visit([](const auto& item) -> QVariant
	{
		using _TyValue = std::decay_t<decltype(item)>;
		if constexpr (std::is_same_v<_TyValue, std::string>)
		{
			return QString::fromStdString(item);
		}
		else
		{
			return QVariant::fromValue(item);
		}
	}, value);
}
