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
	return parent.isValid() ? 0 : static_cast<int>(m_snapshot.GetRowCount());
}

int CDataTableModel::columnCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : static_cast<int>(m_snapshot.GetColumnCount());
}

QVariant CDataTableModel::data(const QModelIndex& index, int nRole) const
{
	if (!index.isValid() || (Qt::DisplayRole != nRole) || (0 > index.row()) || (0 > index.column()))
	{
		return QVariant();
	}
	const std::vector<CDataColumnSchema>& schema = m_snapshot.GetSchema();
	if ((m_snapshot.GetRowCount() <= static_cast<std::size_t>(index.row())) || (schema.size() <= static_cast<std::size_t>(index.column())))
	{
		return QVariant();
	}
	_TyDataValue value;
	if (!m_snapshot.GetValue(static_cast<_TyDataRowIndex>(index.row()), schema[static_cast<std::size_t>(index.column())].m_id, value))
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
	const std::vector<CDataColumnSchema>& schema = m_snapshot.GetSchema();
	return schema.size() <= static_cast<std::size_t>(nSection) ? QVariant() : QVariant(QString::fromStdString(schema[static_cast<std::size_t>(nSection)].m_strName));
}

_TyDataRowId CDataTableModel::GetRowId(int nRow) const
{
	const std::vector<_TyDataRowId>& rowIds = m_snapshot.GetRowIds();
	return (0 > nRow) || (rowIds.size() <= static_cast<std::size_t>(nRow)) ? 0 : rowIds[static_cast<std::size_t>(nRow)];
}

const CDataSnapshot& CDataTableModel::GetSnapshot() const noexcept
{
	return m_snapshot;
}

void CDataTableModel::SetSnapshot(CDataSnapshot snapshot, const CDataChangeSet& changes)
{
	if (!m_snapshot.IsValid() || changes.m_bStructureChanged || (m_snapshot.GetColumnCount() != snapshot.GetColumnCount()) || (m_snapshot.GetRowCount() != snapshot.GetRowCount()))
	{
		beginResetModel();
		m_snapshot = std::move(snapshot);
		endResetModel();
		return;
	}
	m_snapshot = std::move(snapshot);
	if (changes.m_changedCells.empty())
	{
		return;
	}
	int nMinRow = (std::numeric_limits<int>::max)();
	int nMaxRow = -1;
	int nMinColumn = (std::numeric_limits<int>::max)();
	int nMaxColumn = -1;
	const std::vector<CDataColumnSchema>& schema = m_snapshot.GetSchema();
	for (const CDataCellChange& change : changes.m_changedCells)
	{
		_TyDataRowIndex row = 0;
		if (!m_snapshot.FindRow(change.m_rowId, row))
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
