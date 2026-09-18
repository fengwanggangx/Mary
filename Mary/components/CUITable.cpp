#include "CUITable.h"

#include <QItemSelectionModel>
#include <QSortFilterProxyModel>

class CTableSearchProxyModel final : public QSortFilterProxyModel
{
	public:
		explicit CTableSearchProxyModel(QObject* pParent) : QSortFilterProxyModel(pParent)
		{
		}
		QString m_strSearch;
		QList<int> m_searchColumns;

		void Refresh()
		{
			invalidateFilter();
		}

	protected:
		bool filterAcceptsRow(int nRow, const QModelIndex& parent) const override
		{
			if (m_strSearch.isEmpty())
			{
				return true;
			}
			if (m_searchColumns.isEmpty())
			{
				for (int nColumn = 0; sourceModel()->columnCount(parent) > nColumn; ++nColumn)
				{
					if (sourceModel()->index(nRow, nColumn, parent).data().toString().contains(m_strSearch, Qt::CaseInsensitive))
					{
						return true;
					}
				}
			}
			else
			{
				for (const auto& nColumn : m_searchColumns)
				{
					if ((0 <= nColumn) && (sourceModel()->columnCount(parent) > nColumn) && sourceModel()->index(nRow, nColumn, parent).data().toString().contains(m_strSearch, Qt::CaseInsensitive))
					{
						return true;
					}
				}
			}
			return false;
		}
};

CUITable::CUITable(QWidget* pParent) : QTableView(pParent), m_searchProxy(new CTableSearchProxyModel(this))
{
	QTableView::setModel(m_searchProxy);
	connect(selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this](const QModelIndex& current)
	{
		emit RowSelected(current);
	});
	connect(this, &QTableView::clicked, this, &CUITable::RowClicked);
	connect(this, &QTableView::activated, this, &CUITable::RowActivated);
	connect(m_searchProxy, &QAbstractItemModel::modelReset, this, &CUITable::NotifyResults);
	connect(m_searchProxy, &QAbstractItemModel::rowsInserted, this, &CUITable::NotifyResults);
	connect(m_searchProxy, &QAbstractItemModel::rowsRemoved, this, &CUITable::NotifyResults);
	connect(m_searchProxy, &QAbstractItemModel::dataChanged, this, &CUITable::NotifyResults);
}

void CUITable::setModel(QAbstractItemModel* pModel)
{
	if (m_searchProxy == pModel)
	{
		return;
	}
	m_searchProxy->setSourceModel(pModel);
	NotifyResults();
}

QAbstractItemModel* CUITable::SourceModel() const
{
	return m_searchProxy->sourceModel();
}

void CUITable::SetSearchColumns(const QList<int>& columns)
{
	m_searchProxy->m_searchColumns = columns;
	Update();
}

void CUITable::Search(const QString& strText)
{
	QString strSearch = strText.trimmed();
	if (strSearch != m_searchProxy->m_strSearch)
	{
		m_searchProxy->m_strSearch = strSearch;
		Update();
		emit SearchChanged(strSearch);
	}
}

void CUITable::Update()
{
	m_searchProxy->Refresh();
	NotifyResults();
}

int CUITable::ResultCount() const
{
	return m_searchProxy->rowCount();
}

void CUITable::NotifyResults()
{
	emit ResultsChanged(ResultCount());
}
