#include "CViewStrategySettings.h"

#include "../server/CStrategyService.h"
#include "CStrategyEditDialog.h"
#include "ui_CViewStrategySettings.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QMetaObject>
#include <QPointer>
#include <QTableWidgetItem>

CViewStrategySettings::CViewStrategySettings(QWidget* pParent) : QWidget(pParent), ui(new Ui::CViewStrategySettingsClass())
{
	ui->setupUi(this);
	InitializeUI();
	BindService();
	OnRefresh();
}

CViewStrategySettings::~CViewStrategySettings()
{
	CStrategyService::InstanceRef().SetQueryHandler({ });
	CStrategyService::InstanceRef().SetOperationHandler({ });
	delete ui;
}

void CViewStrategySettings::InitializeUI()
{
	ui->strategyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->strategyTable->horizontalHeader()->setStretchLastSection(true);
	connect(ui->refreshButton, &QPushButton::clicked, this, &CViewStrategySettings::OnRefresh);
	connect(ui->addButton, &QPushButton::clicked, this, &CViewStrategySettings::OnAdd);
	connect(ui->modifyButton, &QPushButton::clicked, this, &CViewStrategySettings::OnModify);
	connect(ui->deleteButton, &QPushButton::clicked, this, &CViewStrategySettings::OnDelete);
	connect(ui->searchEdit, &QLineEdit::textChanged, this, &CViewStrategySettings::OnFilterChanged);
	connect(ui->statusCombo, &QComboBox::currentIndexChanged, this, &CViewStrategySettings::OnFilterChanged);
}

void CViewStrategySettings::BindService()
{
	CStrategyService& service = CStrategyService::InstanceRef();
	service.Initialize();
	QPointer<CViewStrategySettings> safeThis(this);
	service.SetQueryHandler([safeThis](const CStrategyService::StrategyList& strategies, const std::string& strError)
	{
		QMetaObject::invokeMethod(safeThis.data(), [safeThis, strategies, strError]()
		{
			if (!safeThis.isNull())
			{
				safeThis->HandleQueryResult(strategies, strError);
			}
		}, Qt::QueuedConnection);
	});
	service.SetOperationHandler([safeThis](const std::string& strCommand, bool bSuccess, const request::StrategyInfo& strategy, const std::string& strError)
	{
		QMetaObject::invokeMethod(safeThis.data(), [safeThis, strCommand, bSuccess, strategy, strError]()
		{
			if (!safeThis.isNull())
			{
				safeThis->HandleOperationResult(strCommand, bSuccess, strategy, strError);
			}
		}, Qt::QueuedConnection);
	});
}

void CViewStrategySettings::OnRefresh()
{
	SetBusy(true, "正在加载策略...");
	if (!CStrategyService::InstanceRef().QueryStrategies())
	{
		SetBusy(false, "策略查询请求发送失败");
	}
}

void CViewStrategySettings::OnAdd()
{
	CStrategyEditDialog dialog(this);
	request::StrategyInfo strategy;
	strategy.set_strategy_type("moving_average");
	strategy.set_event_queue_limit(4096);
	strategy.set_enabled(true);
	dialog.SetStrategy(strategy);
	if (QDialog::Accepted != dialog.exec())
	{
		return;
	}
	SetBusy(true, "正在新增策略...");
	if (!CStrategyService::InstanceRef().AddStrategy(dialog.GetStrategy()))
	{
		SetBusy(false, "新增请求发送失败");
	}
}

void CViewStrategySettings::OnModify()
{
	const request::StrategyInfo* pStrategy = GetSelectedStrategy();
	if (nullptr == pStrategy)
	{
		QMessageBox::information(this, "修改策略", "请先选择一条策略。");
		return;
	}
	CStrategyEditDialog dialog(this);
	dialog.setWindowTitle("修改策略");
	dialog.SetStrategy(*pStrategy);
	if (QDialog::Accepted != dialog.exec())
	{
		return;
	}
	SetBusy(true, "正在修改策略...");
	if (!CStrategyService::InstanceRef().ModifyStrategy(dialog.GetStrategy()))
	{
		SetBusy(false, "修改请求发送失败");
	}
}

void CViewStrategySettings::OnDelete()
{
	const request::StrategyInfo* pStrategy = GetSelectedStrategy();
	if (nullptr == pStrategy)
	{
		QMessageBox::information(this, "删除策略", "请先选择一条策略。");
		return;
	}
	QString strPrompt = QString("确定删除策略“%1”吗？此操作不可撤销。").arg(QString::fromStdString(pStrategy->strategy_name()));
	if (QMessageBox::Yes != QMessageBox::question(this, "删除策略", strPrompt, QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
	{
		return;
	}
	SetBusy(true, "正在删除策略...");
	if (!CStrategyService::InstanceRef().DeleteStrategy(pStrategy->strategy_id()))
	{
		SetBusy(false, "删除请求发送失败");
	}
}

void CViewStrategySettings::OnFilterChanged()
{
	RefreshTable();
}

void CViewStrategySettings::RefreshTable()
{
	QString strKeyword = ui->searchEdit->text().trimmed();
	int statusFilter = ui->statusCombo->currentIndex();
	ui->strategyTable->setRowCount(0);
	for (std::size_t index = 0; index < m_strategies.size(); ++index)
	{
		const request::StrategyInfo& strategy = m_strategies[index];
		QString strId = QString::number(strategy.strategy_id());
		QString strName = QString::fromStdString(strategy.strategy_name());
		if (!strKeyword.isEmpty() && !strId.contains(strKeyword, Qt::CaseInsensitive) && !strName.contains(strKeyword, Qt::CaseInsensitive))
		{
			continue;
		}
		if (((1 == statusFilter) && !strategy.enabled()) || ((2 == statusFilter) && strategy.enabled()))
		{
			continue;
		}
		QStringList subscriptions;
		for (const request::StrategySubscription& subscription : strategy.subscriptions())
		{
			subscriptions.emplace_back(QString("%1.%2 · %3").arg(QString::fromStdString(subscription.security()), QString::fromStdString(subscription.exchange()), QString::fromStdString(subscription.channel())));
		}
		int row = ui->strategyTable->rowCount();
		ui->strategyTable->insertRow(row);
		ui->strategyTable->setItem(row, 0, new QTableWidgetItem(strId));
		ui->strategyTable->setItem(row, 1, new QTableWidgetItem(strName));
		ui->strategyTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(strategy.strategy_type())));
		ui->strategyTable->setItem(row, 3, new QTableWidgetItem(subscriptions.join("\n")));
		ui->strategyTable->setItem(row, 4, new QTableWidgetItem(strategy.enabled() ? "已启用" : "已停用"));
		ui->strategyTable->setItem(row, 5, new QTableWidgetItem(strategy.auto_start() ? "是" : "否"));
		ui->strategyTable->item(row, 0)->setData(Qt::UserRole, static_cast<qulonglong>(index));
	}
	ui->countLabel->setText(QString("共 %1 条策略").arg(ui->strategyTable->rowCount()));
}

void CViewStrategySettings::SetBusy(bool bBusy, const QString& strMessage)
{
	ui->refreshButton->setEnabled(!bBusy);
	ui->addButton->setEnabled(!bBusy);
	ui->modifyButton->setEnabled(!bBusy);
	ui->deleteButton->setEnabled(!bBusy);
	ui->statusLabel->setText(strMessage);
}

const request::StrategyInfo* CViewStrategySettings::GetSelectedStrategy() const
{
	int row = ui->strategyTable->currentRow();
	if ((0 > row) || (nullptr == ui->strategyTable->item(row, 0)))
	{
		return nullptr;
	}
	std::size_t index = static_cast<std::size_t>(ui->strategyTable->item(row, 0)->data(Qt::UserRole).toULongLong());
	return m_strategies.size() > index ? &m_strategies[index] : nullptr;
}

void CViewStrategySettings::HandleQueryResult(const std::vector<request::StrategyInfo>& strategies, const std::string& strError)
{
	SetBusy(false, strError.empty() ? "策略列表已更新" : QString::fromStdString(strError));
	if (!strError.empty())
	{
		QMessageBox::warning(this, "查询失败", QString::fromStdString(strError));
		return;
	}
	m_strategies = strategies;
	RefreshTable();
}

void CViewStrategySettings::HandleOperationResult(const std::string& strCommand, bool bSuccess, const request::StrategyInfo&, const std::string& strError)
{
	SetBusy(false);
	if (!bSuccess)
	{
		QMessageBox::warning(this, "操作失败", QString::fromStdString(strError));
		return;
	}
	QString strMessage = "操作成功";
	if ("strategy_add" == strCommand)
	{
		strMessage = "策略新增成功";
	}
	else if ("strategy_modify" == strCommand)
	{
		strMessage = "策略修改成功";
	}
	else if ("strategy_delete" == strCommand)
	{
		strMessage = "策略删除成功";
	}
	ui->statusLabel->setText(strMessage);
	OnRefresh();
}
