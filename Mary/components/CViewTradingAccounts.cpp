#include "CViewTradingAccounts.h"
#include "CTradeAccountLoginDialog.h"
#include <QInputDialog>
#include "../service/strategy/CStrategyService.h"
#include "../system/CSession.h"
#include <QAbstractTableModel>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTabBar>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
namespace
{
	QString AccountState(const request::TradeAccountRecord& info)
	{
		switch (info.state())
		{
		case request::ACCOUNT_CONNECTED:
			return QStringLiteral("已连接");
		case request::ACCOUNT_ERROR:
			return QStringLiteral("连接异常");
		default:
			return QStringLiteral("未连接");
		}
	}
	QString AccountAction(const std::string& command)
	{
		if ("account_add" == command)
		{
			return "新增账户";
		}
		if ("account_modify" == command)
		{
			return "修改账户";
		}
		if ("account_connect" == command)
		{
			return "连接账户";
		}
		if ("account_disconnect" == command)
		{
			return "断开账户";
		}
		if ("account_query" == command)
		{
			return "查询账户";
		}
		if ("account_snapshot_query" == command)
		{
			return "查询资金";
		}
		if ("account_logs_query" == command)
		{
			return "查询日志";
		}
		return "账户操作";
	}
	QString Money(std::int64_t cents)
	{
		return QLocale().toString(static_cast<double>(cents) / 100.0, 'f', 2);
	}
	QLabel* AddCard(QHBoxLayout* layout, QWidget* parent, const QString& title)
	{
		// Qt parent ownership releases the frame, its layout and labels together.
		QFrame* frame = new QFrame(parent);
		frame->setProperty("summaryCard", true);
		frame->setFixedHeight(80);
		QVBoxLayout* content = new QVBoxLayout(frame);
		content->setContentsMargins(8, 1, 8, 1);
		content->setSpacing(1);
		content->addWidget(new QLabel(title, frame));
		QLabel* value = new QLabel("--", frame);
		value->setProperty("strategyMetric", true);
		content->addWidget(value);
		layout->addWidget(frame, 1);
		return value;
	}
	void SetupTable(QTableWidget* table, const QStringList& headers)
	{
		table->setColumnCount(headers.size());
		table->setHorizontalHeaderLabels(headers);
		table->setEditTriggers(QAbstractItemView::NoEditTriggers);
		table->setSelectionBehavior(QAbstractItemView::SelectRows);
		table->setSelectionMode(QAbstractItemView::SingleSelection);
		table->verticalHeader()->hide();
		table->setShowGrid(false);
		table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	}
} // namespace
class CTradingAccountModel final : public QAbstractTableModel
{
	public:
	explicit CTradingAccountModel(QObject* parent) : QAbstractTableModel(parent)
	{
	}
	int rowCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
	}
	int columnCount(const QModelIndex& parent = QModelIndex()) const override
	{
		return parent.isValid() ? 0 : 6;
	}
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override
	{
		if ((Qt::Horizontal != orientation) || (Qt::DisplayRole != role) || (0 > section) || (6 <= section))
		{
			return QVariant();
		}
		return QStringList{ "账户名称", "类型", "交易通道", "资金账号", "连接状态", "关联策略" }[section];
	}
	QVariant data(const QModelIndex& index, int role) const override
	{
		if (!index.isValid() || (0 > index.row()) || (m_rows.size() <= static_cast<std::size_t>(index.row())))
		{
			return QVariant();
		}
		const request::TradeAccountRecord& info = m_rows[index.row()];
		if (Qt::ToolTipRole == role)
		{
			return QString::fromStdString(info.last_error());
		}
		if (Qt::DisplayRole != role)
		{
			return QVariant();
		}
		switch (index.column())
		{
		case 0:
			return QString::fromStdString(info.name());
		case 1:
			return request::ACCOUNT_SIMULATED == info.account_type() ? "模拟" : "实盘";
		case 2:
			return request::ACCOUNT_SIMULATED == info.account_type() ? "Wind 模拟服务" : "未接入";
		case 3:
			return QString::fromStdString(info.number());
		case 4:
			return AccountState(info);
		case 5:
			return info.strategy_count();
		default:
			return QVariant();
		}
	}
	void SetRows(const std::vector<request::TradeAccountRecord>& rows)
	{
		beginResetModel();
		m_rows = rows;
		endResetModel();
	}

	private:
	std::vector<request::TradeAccountRecord> m_rows;
};
CViewTradingAccounts::CViewTradingAccounts(QWidget* parent) : QWidget(parent)
{
	setObjectName("tradingAccountsPage");
	QVBoxLayout* root = new QVBoxLayout(this);
	root->setContentsMargins(14, 12, 14, 12);
	root->setSpacing(8);
	QHBoxLayout* summary = new QHBoxLayout();
	summary->setSpacing(10);
	root->addLayout(summary);
	QStringList titles{ "账户总数", "已连接", "关联策略", "异常账户" };
	for (int index = 0; 4 > index; ++index)
	{
		m_summary[index] = AddCard(summary, this, titles[index]);
	}
	QHBoxLayout* actions = new QHBoxLayout();
	root->addLayout(actions);
	m_add = new QPushButton("+ 登录账户", this);
	m_add->setObjectName("addButton");
	m_edit = new QPushButton("编辑", this);
	m_connect = new QPushButton("连接", this);
	m_disconnect = new QPushButton("断开", this);
	for (QPushButton* button : { m_add, m_edit, m_connect, m_disconnect })
	{
		actions->addWidget(button);
	}
	actions->addSpacing(12);
	m_search = new QLineEdit(this);
	m_search->setPlaceholderText("搜索账户名称或账号");
	actions->addWidget(m_search, 1);
	m_type = new QComboBox(this);
	m_type->addItems({ "全部类型", "模拟", "实盘" });
	actions->addWidget(m_type);
	m_notice = new QLabel(this);
	m_notice->setWordWrap(true);
	m_notice->hide();
	root->addWidget(m_notice);
	QSplitter* splitter = new QSplitter(Qt::Vertical, this);
	root->addWidget(splitter, 1);
	m_table = new QTableView(splitter);
	m_model = new CTradingAccountModel(this);
	m_table->setModel(m_model);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setShowGrid(false);
	m_table->verticalHeader()->hide();
	m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_table->setMinimumHeight(100);
	QWidget* detail = new QWidget(splitter);
	QVBoxLayout* detailLayout = new QVBoxLayout(detail);
	detailLayout->setContentsMargins(0, 0, 0, 0);
	QHBoxLayout* heading = new QHBoxLayout();
	m_heading = new QLabel("请选择交易账户", detail);
	m_updated = new QLabel(detail);
	heading->addWidget(m_heading, 1);
	heading->addWidget(m_updated);
	detailLayout->addLayout(heading);
	QHBoxLayout* funds = new QHBoxLayout();
	funds->setSpacing(10);
	detailLayout->addLayout(funds);
	QStringList fundTitles{ "总资产", "可用资金", "持仓市值", "今日盈亏" };
	for (int index = 0; 4 > index; ++index)
	{
		m_funds[index] = AddCard(funds, detail, fundTitles[index]);
	}
	m_frozen = new QLabel("冻结资金  --", detail);
	detailLayout->addWidget(m_frozen);
	m_details = new QTabWidget(detail);
	m_details->setObjectName("accountDetailsTabs");
	m_details->tabBar()->setDrawBase(false);
	m_details->tabBar()->setExpanding(false);
	m_basic = new QLabel(m_details);
	m_basic->setWordWrap(true);
	m_basic->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	m_basic->setMargin(10);
	m_basic->setTextFormat(Qt::RichText);
	m_details->addTab(m_basic, "基本信息");
	m_strategies = new QTableWidget(m_details);
	SetupTable(m_strategies, { "实例名称", "策略模板", "交易标的" });
	m_details->addTab(m_strategies, "关联策略");
	m_logs = new QTableWidget(m_details);
	SetupTable(m_logs, { "时间", "操作用户", "操作", "结果" });
	m_details->addTab(m_logs, "连接日志");
	detailLayout->addWidget(m_details, 1);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 1);
	connect(m_details, &QTabWidget::currentChanged, this, [this]()
	{
		Refresh();
		if ((1 == m_details->currentIndex()) && CSession::InstanceRef().IsAuthenticated())
		{
			CStrategyService::InstanceRef().QueryStrategies();
		}
	});
	connect(m_search, &QLineEdit::textChanged, this, [this]()
	{
		RefreshRows();
	});
	connect(m_type, &QComboBox::currentIndexChanged, this, [this]()
	{
		RefreshRows();
	});
	connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]()
	{
		RefreshSelection();
	});
	connect(m_add, &QPushButton::clicked, this, [this]()
	{
		EditAccount(true);
	});
	connect(m_edit, &QPushButton::clicked, this, [this]()
	{
		EditAccount(false);
	});
	connect(m_connect, &QPushButton::clicked, this, [this]()
	{
		Operate("account_connect");
	});
	connect(m_disconnect, &QPushButton::clicked, this, [this]()
	{
		Operate("account_disconnect");
	});
	connect(m_strategies, &QTableWidget::cellDoubleClicked, this, [this](int row, int)
	{
		if ((0 <= row) && (nullptr != m_strategies->item(row, 0)) && (nullptr != m_navigate))
		{
			m_navigate(m_strategies->item(row, 0)->data(Qt::UserRole).toULongLong());
		}
	});
	QPointer<CViewTradingAccounts> safeThis(this);
	m_accountToken = CTradingAccountService::InstanceRef().Subscribe([safeThis](const CTradingAccountService::CResult& result)
	{
		if (!safeThis.isNull())
		{
			QMetaObject::invokeMethod(safeThis.data(), [safeThis, result]()
			{
				if (!safeThis.isNull())
				{
					safeThis->HandleResult(result);
				}
			}, Qt::QueuedConnection);
		}
	});
	m_strategyToken = CStrategyService::InstanceRef().AddQueryHandler([safeThis](const CStrategyService::_TyStrategyList&, const std::string&)
	{
		if (!safeThis.isNull())
		{
			QMetaObject::invokeMethod(safeThis.data(), [safeThis]()
			{
				if (!safeThis.isNull())
				{
					safeThis->RefreshStrategies();
				}
			}, Qt::QueuedConnection);
		}
	});
	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, [this]()
	{
		if (isVisible())
		{
			Refresh();
		}
	});
	timer->start(2000);
	RefreshButtons();
}
CViewTradingAccounts::~CViewTradingAccounts()
{
	CTradingAccountService::InstanceRef().Unsubscribe(m_accountToken);
	CStrategyService::InstanceRef().RemoveQueryHandler(m_strategyToken);
}
void CViewTradingAccounts::SetStrategyNavigation(std::function<void(std::uint64_t)>&& handler)
{
	m_navigate = std::move(handler);
}
void CViewTradingAccounts::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	Refresh();
	if (CSession::InstanceRef().IsAuthenticated())
	{
		CStrategyService::InstanceRef().QueryStrategies();
	}
}
void CViewTradingAccounts::Refresh()
{
	RefreshButtons();
	if (!CSession::InstanceRef().IsAuthenticated() || !CTradingAccountService::InstanceRef().IsSupported())
	{
		return;
	}
	CTradingAccountService& service = CTradingAccountService::InstanceRef();
	service.Send("account_query");
	if (0 != m_selectedId)
	{
		std::uint64_t id = service.Send("account_snapshot_query", m_selectedId);
		if (0 != id)
		{
			m_snapshotId = id;
		}
		id = 2 == m_details->currentIndex() ? service.Send("account_logs_query", m_selectedId) : 0;
		if (0 != id)
		{
			m_logsId = id;
		}
	}
}
void CViewTradingAccounts::RefreshRows()
{
	m_visible.clear();
	QString search = m_search->text().trimmed();
	for (const auto& info : m_accounts)
	{
		if ((!search.isEmpty() && !QString::fromStdString(info.name()).contains(search, Qt::CaseInsensitive) && !QString::fromStdString(info.number()).contains(search, Qt::CaseInsensitive)) || ((1 == m_type->currentIndex()) && (request::ACCOUNT_SIMULATED != info.account_type())) || ((2 == m_type->currentIndex()) && (request::ACCOUNT_PRODUCTION != info.account_type())))
		{
			continue;
		}
		m_visible.emplace_back(info);
	}
	QSignalBlocker blocker(m_table->selectionModel());
	m_model->SetRows(m_visible);
	int selectedRow = -1;
	for (std::size_t index = 0; index < m_visible.size(); ++index)
	{
		if (m_selectedId == m_visible[index].account_id())
		{
			selectedRow = static_cast<int>(index);
			break;
		}
	}
	if (0 <= selectedRow)
	{
		m_table->selectRow(selectedRow);
	}
	else
	{
		m_table->clearSelection();
		m_table->setCurrentIndex(QModelIndex());
	}
	RefreshSelection();
}
const request::TradeAccountRecord* CViewTradingAccounts::SelectedAccount() const
{
	for (const auto& info : m_visible)
	{
		if (m_selectedId == info.account_id())
		{
			return &info;
		}
	}
	return nullptr;
}
void CViewTradingAccounts::ClearDetails()
{
	m_snapshotId = 0;
	m_logsId = 0;
	m_heading->setText("请选择交易账户");
	m_basic->clear();
	m_updated->clear();
	m_frozen->setText("冻结资金  --");
	for (QLabel* value : m_funds)
	{
		value->setText("--");
	}
	m_logs->setRowCount(0);
	m_strategies->setRowCount(0);
}
void CViewTradingAccounts::RefreshSelection()
{
	QModelIndexList rows = m_table->selectionModel()->selectedRows();
	std::uint64_t nextId = (rows.isEmpty() || (0 > rows.front().row()) || (m_visible.size() <= static_cast<std::size_t>(rows.front().row()))) ? 0 : m_visible[rows.front().row()].account_id();
	bool changed = nextId != m_selectedId;
	if (changed)
	{
		ClearDetails();
		m_selectedId = nextId;
	}
	const request::TradeAccountRecord* info = SelectedAccount();
	if (nullptr != info)
	{
		m_heading->setText(QString::fromStdString(info->name()) + "  ·  " + (request::ACCOUNT_SIMULATED == info->account_type() ? "模拟" : "实盘") + "  ·  " + AccountState(*info));
		m_basic->setText(QString("<table width='100%' cellspacing='10'><tr><td>账户名称</td><td>%1</td><td>交易通道</td><td>Wind 模拟服务</td></tr><tr><td>账户类型</td><td>%2</td><td>运行位置</td><td>Wind 服务主站</td></tr><tr><td>资金账号</td><td>%3</td><td>创建时间</td><td>%4</td></tr></table>").arg(QString::fromStdString(info->name()).toHtmlEscaped(), request::ACCOUNT_SIMULATED == info->account_type() ? "模拟交易" : "实盘交易", QString::fromStdString(info->number()).toHtmlEscaped(), QString::fromStdString(info->created_at()).toHtmlEscaped()));
		if (request::ACCOUNT_CONNECTED != info->state())
		{
			for (QLabel* value : m_funds)
			{
				value->setText("--");
			}
			m_frozen->setText("冻结资金  --");
			m_updated->clear();
		}
		m_heading->setToolTip(QString::fromStdString(info->last_error()));
	}
	RefreshStrategies();
	RefreshButtons();
	if (changed && (0 != m_selectedId))
	{
		Refresh();
	}
}
void CViewTradingAccounts::RefreshButtons()
{
	bool available = CSession::InstanceRef().IsAuthenticated() && CTradingAccountService::InstanceRef().IsSupported() && (0 == m_operationId);
	const request::TradeAccountRecord* info = SelectedAccount();
	m_add->setEnabled(available);
	m_edit->setEnabled(available && (nullptr != info));
	m_connect->setEnabled(available && (nullptr != info) && (request::ACCOUNT_CONNECTED != info->state()));
	m_disconnect->setEnabled(available && (nullptr != info) && (request::ACCOUNT_CONNECTED == info->state()));
	QString reason = !CSession::InstanceRef().IsAuthenticated() ? "等待登录或重新连接 Wind" : (!CTradingAccountService::InstanceRef().IsSupported() ? "当前 Wind 不支持账户管理" : "");
	for (QPushButton* button : { m_add, m_edit, m_connect, m_disconnect })
	{
		button->setToolTip(reason);
	}
	if (!reason.isEmpty())
	{
		m_notice->setText(reason);
		m_notice->show();
	}
}
void CViewTradingAccounts::HandleResult(const CTradingAccountService::CResult& result)
{
	if (result.m_generation != CTradingAccountService::InstanceRef().GetGeneration())
	{
		return;
	}
	if ("account_session_reset" == result.m_command)
	{
		m_accounts.clear();
		m_operationId = 0;
		ClearDetails();
		RefreshRows();
		for (QLabel* value : m_summary)
		{
			value->setText("--");
		}
		if (isVisible())
		{
			Refresh();
		}
		return;
	}
	if ((("account_snapshot_query" == result.m_command) && ((result.m_accountId != m_selectedId) || (result.m_requestId != m_snapshotId))) || (("account_logs_query" == result.m_command) && ((result.m_accountId != m_selectedId) || (result.m_requestId != m_logsId))))
	{
		return;
	}
	if (result.m_requestId == m_operationId)
	{
		m_operationId = 0;
		if (result.m_error.empty() && ("account_add" == result.m_command) && result.m_data.has_account())
		{
			ClearDetails();
			m_selectedId = result.m_data.account().account_id();
		}
		if (!result.m_error.empty())
		{
			QMessageBox::warning(this, "账户操作失败", QString::fromStdString(result.m_error));
		}
		Refresh();
	}
	if (!result.m_error.empty())
	{
		m_notice->setText(QString::fromStdString(result.m_error));
		m_notice->show();
		if (("account_snapshot_query" == result.m_command) && (result.m_requestId == m_snapshotId))
		{
			for (QLabel* value : m_funds)
			{
				value->setText("--");
			}
			m_frozen->setText("冻结资金  --");
			m_updated->clear();
		}
		RefreshButtons();
		return;
	}
	if ("account_query" == result.m_command)
	{
		m_accounts = CTradingAccountService::InstanceRef().GetAccounts();
		int connected = 0;
		int strategies = 0;
		int errors = 0;
		for (const auto& info : m_accounts)
		{
			connected += request::ACCOUNT_CONNECTED == info.state() ? 1 : 0;
			errors += request::ACCOUNT_ERROR == info.state() ? 1 : 0;
			strategies += info.strategy_count();
		}
		m_summary[0]->setText(QString::number(m_accounts.size()));
		m_summary[1]->setText(QString::number(connected));
		m_summary[2]->setText(QString::number(strategies));
		m_summary[3]->setText(QString::number(errors));
		m_notice->hide();
		RefreshRows();
	}
	else if ((result.m_accountId == m_selectedId) && (0 != m_selectedId))
	{
		if ((result.m_requestId == m_snapshotId) && result.m_data.has_account_snapshot())
		{
			const request::TradingAccountSnapshot& snapshot = result.m_data.account_snapshot();
			if ((snapshot.account_id() != m_selectedId) || (nullptr == SelectedAccount()) || (request::ACCOUNT_CONNECTED != SelectedAccount()->state()))
			{
				return;
			}
			m_funds[0]->setText(snapshot.available_snapshot() ? Money(snapshot.balance()) : "--");
			m_funds[1]->setText(snapshot.available_snapshot() ? Money(snapshot.available()) : "--");
			m_frozen->setText("冻结资金  " + (snapshot.available_snapshot() ? Money(snapshot.frozen()) : "--"));
			m_updated->setText(snapshot.available_snapshot() ? "数据更新 " + QDateTime::fromMSecsSinceEpoch(snapshot.server_time_ms()).toString("HH:mm:ss") : "");
		}
		if ((result.m_requestId == m_logsId) && result.m_data.has_account_logs() && (m_selectedId == result.m_data.account_logs().account_id()))
		{
			m_logs->setRowCount(0);
			for (const auto& log : result.m_data.account_logs().logs())
			{
				int row = m_logs->rowCount();
				m_logs->insertRow(row);
				QStringList values{ QString::fromStdString(log.time()), QString::fromStdString(log.actor()), AccountAction(log.action()), QString::fromStdString(log.message()) };
				for (int column = 0; 4 > column; ++column)
				{
					m_logs->setItem(row, column, new QTableWidgetItem(values[column]));
				}
			}
		}
	}
	RefreshButtons();
}
void CViewTradingAccounts::RefreshStrategies()
{
	m_strategies->setRowCount(0);
	for (const auto& strategy : CStrategyService::InstanceRef().GetStrategies())
	{
		if ((0 == m_selectedId) || (m_selectedId != (0 == strategy.account_id() ? 1 : strategy.account_id())))
		{
			continue;
		}
		int row = m_strategies->rowCount();
		m_strategies->insertRow(row);
		QTableWidgetItem* name = new QTableWidgetItem(QString::fromStdString(strategy.strategy_name()));
		name->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(strategy.strategy_id()));
		m_strategies->setItem(row, 0, name);
		m_strategies->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(strategy.strategy_type())));
		QStringList securities;
		for (const auto& subscription : strategy.subscriptions())
		{
			securities.append(QString::fromStdString(subscription.security()));
		}
		m_strategies->setItem(row, 2, new QTableWidgetItem(securities.join(", ")));
	}
}
void CViewTradingAccounts::Operate(const std::string& command)
{
	if (0 != m_operationId)
	{
		return;
	}
	m_operationId = CTradingAccountService::InstanceRef().Send(command, m_selectedId);
	if (0 == m_operationId)
	{
		QMessageBox::warning(this, "请求未发送", "请检查连接状态，或等待当前请求完成");
	}
	RefreshButtons();
}
void CViewTradingAccounts::EditAccount(bool bNew)
{
	if (bNew)
	{
		CTradeAccountLoginDialog dialog(this);
		if (QDialog::Accepted == dialog.exec())
		{
			Refresh();
		}
		return;
	}
	const request::TradeAccountRecord* selected = SelectedAccount();
	if (nullptr == selected)
	{
		return;
	}
	std::uint64_t nAccountId = selected->account_id();
	bool bAccepted = false;
	QString strName = QInputDialog::getText(this, "编辑账户", "账户名称", QLineEdit::Normal, QString::fromStdString(selected->name()), &bAccepted).trimmed();
	if (!bAccepted)
	{
		return;
	}
	if (strName.isEmpty() || (128 < strName.toUtf8().size()))
	{
		QMessageBox::warning(this, "账户名称无效", "请输入名称，长度不超过 128 字节");
		return;
	}
	m_operationId = CTradingAccountService::InstanceRef().Send("account_modify", nAccountId, { { "name", strName.toStdString() } });
	if (0 == m_operationId)
	{
		QMessageBox::warning(this, "请求未发送", "请检查连接状态，或等待当前请求完成");
	}
	RefreshButtons();
}
