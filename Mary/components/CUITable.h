#ifndef MARY_COMPONENTS_CUITABLE_H
#define MARY_COMPONENTS_CUITABLE_H

#include <QList>
#include <QTableView>

class CTableSearchProxyModel;

// A presentation-only table. Models and delegates are supplied by its page.
// Event indexes refer to the displayed model, including the search filter.
class CUITable final : public QTableView
{
		Q_OBJECT
	public:
		explicit CUITable(QWidget* pParent = nullptr);
		void setModel(QAbstractItemModel* pModel) override;
		QAbstractItemModel* SourceModel() const;
		void SetSearchColumns(const QList<int>& columns);
		void Search(const QString& strText);
		void Update();
		int ResultCount() const;

	signals:
		void RowSelected(const QModelIndex& index);
		void RowClicked(const QModelIndex& index);
		void RowActivated(const QModelIndex& index);
		void SearchChanged(const QString& strText);
		void ResultsChanged(int nCount);

	private:
		void NotifyResults();
		CTableSearchProxyModel* m_searchProxy{ nullptr };
};

#endif
