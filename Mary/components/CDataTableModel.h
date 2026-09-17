#ifndef MARY_COMPONENTS_CDATATABLEMODEL_H
#define MARY_COMPONENTS_CDATATABLEMODEL_H

#include "../basic/CDatable.h"

#include <QAbstractTableModel>

class CDataTableModel final : public QAbstractTableModel
{
public:
	explicit CDataTableModel(QObject* pParent = nullptr);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int nRole = Qt::DisplayRole) const override;
	QVariant headerData(int nSection, Qt::Orientation orientation, int nRole = Qt::DisplayRole) const override;
	_TyDataRowId GetRowId(int nRow) const;
	const CDataTableView& GetView() const noexcept;
	void SetView(CDataTableView view, const CDataChangeSet& changes);

private:
	QVariant ToVariant(const _TyDataValue& value) const;

private:
	CDataTableView m_view;
};

#endif
