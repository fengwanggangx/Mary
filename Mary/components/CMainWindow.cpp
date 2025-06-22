#include "CMainWindow.h"
#include <QTreeWidgetItem>
#include "components/CViewSettings.h"
#include "components/CViewProducts.h"
#include "components/CViewProductDateInfo.h"

struct CTreeItemInfo
{
	CTreeItemInfo(int nId, const QString& strText, const QString& strIcon, std::vector<CTreeItemInfo>&& children) :
		m_nId(nId), m_strText(strText), m_strIcon(strIcon), m_children(std::move(children)) { }

	CTreeItemInfo(int nId, const QString& strText, const QString& strIcon) :
		m_nId(nId), m_strText(strText), m_strIcon(strIcon) { }

	int m_nId{ -1 };
	QString m_strText;
	QString m_strIcon;
	std::vector<CTreeItemInfo> m_children;

	static QTreeWidgetItem* BuildItem(const CTreeItemInfo& info)
	{
		QTreeWidgetItem* pItem = new QTreeWidgetItem();
		pItem->setText(0, info.m_strText);
		pItem->setIcon(0, QIcon(info.m_strIcon));

		pItem->setText(1, QString::number(info.m_nId));
		if (info.m_children.empty())
		{
			return pItem;
		}
		for (const auto& child : info.m_children)
		{
			pItem->addChild(BuildItem(child));
		}
		return pItem;
	}
};




CMainWindow::CMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CMainWindowClass())
{
    ui->setupUi(this);
	UIInitialized();
	ConnectSlots();
}

void CMainWindow::UIInitialized()
{
	static std::unordered_map<int, CTreeItemInfo> s_tree
	{
		{ 0, { 0, "系统设置", ":/images/setting.png" } },
		{ 1, { 1, "产品管理", ":/images/setting.png" } },
		{ 2, { -1, "脱脂12", ":/images/setting.png", { { 2, "201502", "", {{ 2, "201602", "" }, { 2, "201802", "" }} }, { 2, "201503", "" }} } }
	};

	ui->treeWidget->setIndentation(10);
	for (const auto& item : s_tree)
	{
		ui->treeWidget->addTopLevelItem(CTreeItemInfo::BuildItem(item.second));
	}
	ui->stackedWidget->addWidget(new CViewSettings);
	ui->stackedWidget->addWidget(new CViewProducts);
	ui->stackedWidget->addWidget(new CViewProductDateInfo);
}

void CMainWindow::ConnectSlots()
{
	QObject::connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &CMainWindow::OnItemSelectChanged);
}

void CMainWindow::OnItemSelectChanged()
{
	QList<QTreeWidgetItem*> selects = ui->treeWidget->selectedItems();
	QTreeWidgetItem* pItem = selects.first();
	if (nullptr == pItem)
	{
		return;
	}
	QString strText = pItem->text(0);
	QString strTabIndex = pItem->text(1);
	int nIdx = strTabIndex.toInt();
	if (!strText.isEmpty() && (nIdx >= 0))
	{
		ui->stackedWidget->setCurrentIndex(strTabIndex.toInt());
	}
}

CMainWindow::~CMainWindow()
{
    delete ui;
}
