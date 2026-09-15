#include "CMainWindow.h"

#include "CRiskSettings.h"
#include "CSystemSettings.h"
#include "CViewHQMarket.h"
#include "CViewStrategySettings.h"
#include "../system/CSession.h"

#include <QActionGroup>
#include <QApplication>
#include <QDateTime>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPointer>
#include <QPixmap>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QToolButton>

#include <cstddef>
#include <vector>

namespace
{
	struct CPageInfo
	{
		QString m_key;
		QString m_title;
		QString m_icon;
	};

	const std::vector<CPageInfo> s_pages
	{
		CPageInfo{ "hqmarket", "市场行情", ":/navigation/hqmarket-%1.png" },
		CPageInfo{ "strategy_settings", "策略管理", ":/navigation/strategy-settings-%1.png" },
		CPageInfo{ "risk_settings", "风控设置", ":/navigation/risk-setting-%1.png" },
		CPageInfo{ "ai_trade", "智能量化", ":/navigation/ai-trade-%1.png" },
		CPageInfo{ "system_settings", "系统设置", ":/navigation/system-settings-%1.png" }
	};

	QWidget* CreateView(const QString& strKey, QWidget* pParent)
	{
		if ("hqmarket" == strKey)
		{
			return new CViewHQMarket(pParent);
		}
		if ("strategy_settings" == strKey)
		{
			return new CViewStrategySettings(pParent);
		}
		if ("risk_settings" == strKey)
		{
			return new CRiskSettings(pParent);
		}
		if ("ai_trade" == strKey)
		{
			return new QWidget(pParent);
		}
		if ("system_settings" == strKey)
		{
			return new CSystemSettings(pParent);
		}
		return nullptr;
	}

	QString LightStyle()
	{
		return R"(
QMainWindow, #centralWidget { background: #f4f7fb; color: #162033; }
#titleBar { background: #ffffff; border-bottom: 1px solid #e4eaf2; }
#appIconLabel { background: transparent; }
#appTitleLabel { color: #162033; font-size: 15px; font-weight: 600; }
#navigationFrame { background: #ffffff; border-right: 1px solid #e4eaf2; }
QTreeWidget { background: transparent; border: none; outline: none; color: #536176; font-size: 14px; }
QTreeWidget::item { height: 42px; padding-left: 10px; }
QTreeWidget::item:selected { background: #4d82f9; color: white; border-top-left-radius: 6px; border-bottom-left-radius: 6px; }
QTabBar { background: #ffffff; border: 0; border-bottom: 1px solid #e6ebf2; padding: 0; margin: 0; }
QTabBar::tab { min-width: 72px; height: 36px; color: #65748a; background: #ffffff; border: 1px solid #e6ebf2; border-top: 0; margin-left: -1px; padding: 0 8px; }
QTabBar::tab:first { margin-left: 0; }
QTabBar::tab:selected { color: #1677ff; background: #edf5fd; border-bottom: 2px solid #1677ff; font-weight: 600; }
QFrame#shanghaiCard, QFrame#shenzhenCard, QFrame#chinextCard, QFrame#chartFrame, QFrame#depthFrame, QFrame#watchlistFrame { background: white; border: 1px solid #e4eaf2; border-radius: 7px; }
#shanghaiPrice, #shenzhenPrice, #chinextPrice { color: #f04455; font-size: 23px; font-weight: 700; }
#chartTitle, #depthTitle, #watchlistTitle { color: #1d2a3c; font-size: 14px; font-weight: 600; }
#chartPlaceholder { color: #8b98aa; background: #fafcff; border: 1px dashed #dce4ef; }
QTableWidget { background: white; alternate-background-color: #f8fafd; border: none; gridline-color: #edf1f6; color: #344258; }
QHeaderView::section { background: #f7f9fc; color: #68778c; border: none; border-bottom: 1px solid #e6ebf2; padding: 6px; }
QLineEdit { background: white; border: 1px solid #dce3ed; border-radius: 5px; padding: 6px 9px; }
QPushButton { border: 1px solid #dce3ed; border-radius: 5px; background: white; color: #536176; padding: 5px 10px; }
QPushButton:hover { border-color: #1677ff; color: #1677ff; }
#addButton { background: #1677ff; color: white; border-color: #1677ff; }
#skinButton { background: transparent; color: #536176; border: 1px solid transparent; border-radius: 6px; padding: 0; }
#skinButton:hover, #skinButton:pressed { color: #1677ff; background: #eef3f8; border-color: #dce3ed; }
#skinButton::menu-indicator { image: none; width: 0; height: 0; }
#minimizeButton, #maximizeButton, #closeButton { border: 1px solid transparent; border-radius: 6px; background: transparent; padding: 0; font-size: 15px; }
#minimizeButton:hover, #maximizeButton:hover { background: #eef3f8; border-color: #dce3ed; }
#closeButton:hover { background: #f04455; color: white; }
QMenu { background: white; color: #344258; border: 1px solid #dce3ed; border-radius: 6px; padding: 4px; }
QMenu::item { min-width: 80px; padding: 6px 16px; border-radius: 4px; }
QMenu::item:selected { background: #eef6ff; color: #1677ff; }
QMenu#skinMenu::item { padding: 6px 8px 6px 20px; }
QMenu#skinMenu::indicator { width: 14px; height: 14px; subcontrol-origin: padding; subcontrol-position: left center; left: 4px; }
QMenu#skinMenu::indicator:checked { image: url(:/navigation/check-white.png); }
QMenu#skinMenu::indicator:exclusive:checked { image: url(:/navigation/check-white.png); }
QMenu#skinMenu::indicator:unchecked { image: none; }
#globalStatusBar { background: white; border-top: 1px solid #e4eaf2; color: #69778b; }
#connectionDot { color: #18bd8b; font-size: 11px; }
)";
	}

	QString DarkStyle()
	{
		return R"(
QMainWindow, #centralWidget { background: #122130; color: #dce8f6; }
#titleBar { background: #0f1e2d; border-bottom: 1px solid #203b52; }
#appIconLabel { background: transparent; }
#appTitleLabel { color: #f2f7ff; font-size: 15px; font-weight: 600; }
#navigationFrame { background: #122130; border-right: 1px solid #203b52; }
QTreeWidget { background: transparent; border: none; outline: none; color: #a9bdd0; font-size: 14px; }
QTreeWidget::item { height: 42px; padding-left: 10px; }
QTreeWidget::item:selected { background: #3a6ef7; color: white; border-top-left-radius: 6px; border-bottom-left-radius: 6px; }
QTabBar { background: #0f1e2d; border: 0; border-bottom: 1px solid #203b52; padding: 0; margin: 0; }
QTabBar::tab { min-width: 72px; height: 36px; color: #9eb3c7; background: #0f1e2d; border: 1px solid #203b52; border-top: 0; margin-left: -1px; padding: 0 8px; }
QTabBar::tab:first { margin-left: 0; }
QTabBar::tab:selected { color: white; background: #1e3e81; border-bottom: 2px solid #1683ff; font-weight: 600; }
QFrame#shanghaiCard, QFrame#shenzhenCard, QFrame#chinextCard, QFrame#chartFrame, QFrame#depthFrame, QFrame#watchlistFrame { background: #132130; border: 1px solid #203b52; border-radius: 7px; }
#shanghaiPrice, #shenzhenPrice, #chinextPrice { color: #ff4d61; font-size: 23px; font-weight: 700; }
#chartTitle, #depthTitle, #watchlistTitle { color: #edf6ff; font-size: 14px; font-weight: 600; }
#chartPlaceholder { color: #718ba0; background: #12202f; border: 1px dashed #203b52; }
QTableWidget { background: #0e192b; alternate-background-color: #122130; border: none; gridline-color: #203b52; color: #c8d8e7; }
QHeaderView::section { background: #14283a; color: #8fa8bc; border: none; border-bottom: 1px solid #203b52; padding: 6px; }
QLineEdit { background: #0f1e2d; border: 1px solid #203b52; border-radius: 5px; color: #dce8f6; padding: 6px 9px; }
QPushButton { border: 1px solid #203b52; border-radius: 5px; background: #122130; color: #a9bdd0; padding: 5px 10px; }
QPushButton:hover { border-color: #1683ff; color: white; }
#addButton { background: #1683ff; color: white; border-color: #1683ff; }
#skinButton { background: transparent; color: #b8cad9; border: 1px solid transparent; border-radius: 6px; padding: 0; }
#skinButton:hover, #skinButton:pressed { color: white; background: #14283a; border-color: #203b52; }
#skinButton::menu-indicator { image: none; width: 0; height: 0; }
#minimizeButton, #maximizeButton, #closeButton { border: 1px solid transparent; border-radius: 6px; background: transparent; padding: 0; color: #b8cad9; font-size: 15px; }
#minimizeButton:hover, #maximizeButton:hover { background: #14283a; border-color: #203b52; }
#closeButton:hover { background: #f04455; color: white; }
QMenu { background: #122130; color: #c8d8e7; border: 1px solid #203b52; border-radius: 6px; padding: 4px; }
QMenu::item { min-width: 80px; padding: 6px 16px; border-radius: 4px; }
QMenu::item:selected { background: #0d4d83; color: white; }
QMenu#skinMenu::item { padding: 6px 8px 6px 20px; }
QMenu#skinMenu::indicator { width: 14px; height: 14px; subcontrol-origin: padding; subcontrol-position: left center; left: 4px; }
QMenu#skinMenu::indicator:checked { image: url(:/navigation/check-black.png); }
QMenu#skinMenu::indicator:exclusive:checked { image: url(:/navigation/check-black.png); }
QMenu#skinMenu::indicator:unchecked { image: none; }
#globalStatusBar { background: #0f1e2d; border-top: 1px solid #203b52; color: #8fa8bc; }
#connectionDot { color: #20d39b; font-size: 11px; }
)";
	}
}

CMainWindow::CMainWindow(QWidget* pParent) : QMainWindow(pParent), ui(new Ui::CMainWindowClass())
{
	ui->setupUi(this);
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
	setWindowIcon(QIcon(":/branding/mary-app-icon.png"));
	UIInitialized();
	ConnectSlots();
}

CMainWindow::~CMainWindow()
{
	CSession::InstanceRef().SetStateCallback({ });
	delete ui;
}

void CMainWindow::UIInitialized()
{
	ui->titleBar->installEventFilter(this);
	ui->appIconLabel->setPixmap(QPixmap(":/branding/mary-app-icon.png").scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	QMenu* pSkinMenu = new QMenu(ui->skinButton);
	pSkinMenu->setObjectName("skinMenu");
	QActionGroup* pThemeGroup = new QActionGroup(pSkinMenu);
	pThemeGroup->setExclusive(true);
	m_pLightThemeAction = pSkinMenu->addAction("浅色模式");
	m_pDarkThemeAction = pSkinMenu->addAction("深色模式");
	m_pLightThemeAction->setCheckable(true);
	m_pDarkThemeAction->setCheckable(true);
	m_pLightThemeAction->setIconVisibleInMenu(false);
	m_pDarkThemeAction->setIconVisibleInMenu(false);
	pThemeGroup->addAction(m_pLightThemeAction);
	pThemeGroup->addAction(m_pDarkThemeAction);
	ui->skinButton->setMenu(pSkinMenu);
	ui->skinButton->setPopupMode(QToolButton::InstantPopup);
	ui->skinButton->setIconSize(QSize(18, 18));
	ui->skinButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	ui->skinButton->setToolTip("皮肤");
	ui->minimizeButton->setIconSize(QSize(16, 16));
	ui->minimizeButton->setToolTip("最小化");
	ui->maximizeButton->setIconSize(QSize(16, 16));
	ui->closeButton->setIconSize(QSize(16, 16));
	ui->closeButton->setToolTip("关闭");
	UpdateWindowButtonIcons();
	ui->treeWidget->clear();
	ui->treeWidget->setIndentation(0);
	ui->treeWidget->setIconSize(QSize(20, 20));
	for (std::size_t nIndex = 0; nIndex < s_pages.size(); ++nIndex)
	{
		const CPageInfo& info = s_pages[nIndex];
		QWidget* pView = CreateView(info.m_key, ui->stackedWidget);
		if (nullptr == pView)
		{
			continue;
		}
		ui->stackedWidget->addWidget(pView);
		QTreeWidgetItem* pItem = new QTreeWidgetItem(ui->treeWidget);
		pItem->setText(0, info.m_title);
		pItem->setData(0, Qt::UserRole, static_cast<int>(nIndex));
	}
	QTreeWidgetItem* pFirstItem = ui->treeWidget->topLevelItem(0);
	if (nullptr != pFirstItem)
	{
		ui->treeWidget->setCurrentItem(pFirstItem);
		ui->stackedWidget->setCurrentIndex(0);
	}
	ApplyTheme(false);
	UpdateClock();
	QTimer* pTimer = new QTimer(this);
	connect(pTimer, &QTimer::timeout, this, &CMainWindow::UpdateClock);
	pTimer->start(1000);

	QPointer<CMainWindow> safeThis(this);
	CSession::InstanceRef().SetStateCallback([safeThis](SessionState state, const std::string& strMessage)
	{
		QMetaObject::invokeMethod(safeThis.data(), [safeThis, state, strMessage]()
		{
			if (!safeThis.isNull())
			{
				safeThis->UpdateConnectionState(static_cast<int>(state), QString::fromStdString(strMessage));
			}
		}, Qt::QueuedConnection);
	});
	UpdateConnectionState(static_cast<int>(CSession::InstanceRef().GetState()), QString());
}

void CMainWindow::ConnectSlots()
{
	connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &CMainWindow::OnItemSelectChanged);
	connect(m_pLightThemeAction, &QAction::triggered, this, &CMainWindow::OnLightTheme);
	connect(m_pDarkThemeAction, &QAction::triggered, this, &CMainWindow::OnDarkTheme);
	connect(ui->minimizeButton, &QPushButton::clicked, this, &QWidget::showMinimized);
	connect(ui->maximizeButton, &QPushButton::clicked, this, [this]()
	{
		isMaximized() ? showNormal() : showMaximized();
	});
	connect(ui->closeButton, &QPushButton::clicked, this, &QWidget::close);
}

void CMainWindow::OnItemSelectChanged()
{
	QTreeWidgetItem* pItem = ui->treeWidget->currentItem();
	if (nullptr == pItem)
	{
		return;
	}
	int nIndex = pItem->data(0, Qt::UserRole).toInt();
	if ((0 <= nIndex) && (ui->stackedWidget->count() > nIndex))
	{
		ui->stackedWidget->setCurrentIndex(nIndex);
	}
}

void CMainWindow::OnLightTheme()
{
	ApplyTheme(false);
}

void CMainWindow::OnDarkTheme()
{
	ApplyTheme(true);
}

void CMainWindow::ApplyTheme(bool bDark)
{
	m_bDarkTheme = bDark;
	qApp->setStyleSheet(bDark ? DarkStyle() : LightStyle());
	m_pLightThemeAction->setChecked(!bDark);
	m_pDarkThemeAction->setChecked(bDark);
	UpdateWindowButtonIcons();
	QString strTheme = bDark ? "black" : "white";
	for (int nItem = 0; nItem < ui->treeWidget->topLevelItemCount(); ++nItem)
	{
		QTreeWidgetItem* pItem = ui->treeWidget->topLevelItem(nItem);
		int nIndex = pItem->data(0, Qt::UserRole).toInt();
		if ((0 <= nIndex) && (static_cast<std::size_t>(nIndex) < s_pages.size()))
		{
			pItem->setIcon(0, QIcon(s_pages[nIndex].m_icon.arg(strTheme)));
		}
	}
}

void CMainWindow::UpdateClock()
{
	ui->clockLabel->setText(QDateTime::currentDateTime().toString("HH:mm:ss"));
}

void CMainWindow::UpdateWindowButtonIcons()
{
	bool bMaximized = isMaximized();
	QString strTheme = m_bDarkTheme ? "black" : "white";
	ui->skinButton->setIcon(QIcon(QString(":/navigation/skin-setting-%1.png").arg(strTheme)));
	ui->minimizeButton->setIcon(QIcon(QString(":/window/min-%1.png").arg(strTheme)));
	ui->maximizeButton->setIcon(QIcon(QString(bMaximized ? ":/window/restore-%1.png" : ":/window/max-%1.png").arg(strTheme)));
	ui->closeButton->setIcon(QIcon(QString(":/window/close-%1.png").arg(strTheme)));
	ui->maximizeButton->setToolTip(bMaximized ? "还原" : "最大化");
}

void CMainWindow::UpdateConnectionState(int nState, const QString& strMessage)
{
	SessionState state = static_cast<SessionState>(nState);
	bool bReady = SessionState::Ready == state;
	ui->connectionDot->setStyleSheet(bReady ? "color: #20c997;" : "color: #f0ad4e;");
	if (bReady)
	{
		ui->connectionLabel->setText("交易连接正常");
		return;
	}
	ui->connectionLabel->setText(strMessage.isEmpty() ? "交易连接中断" : strMessage);
}

void CMainWindow::changeEvent(QEvent* pEvent)
{
	QMainWindow::changeEvent(pEvent);
	if (QEvent::WindowStateChange == pEvent->type())
	{
		UpdateWindowButtonIcons();
	}
}

bool CMainWindow::eventFilter(QObject* pObject, QEvent* pEvent)
{
	if (ui->titleBar != pObject)
	{
		return QMainWindow::eventFilter(pObject, pEvent);
	}
	if (QEvent::MouseButtonPress == pEvent->type())
	{
		QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
		if (Qt::LeftButton == pMouseEvent->button())
		{
			m_bDragging = true;
			m_dragPosition = pMouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
			return true;
		}
	}
	else if (QEvent::MouseMove == pEvent->type())
	{
		QMouseEvent* pMouseEvent = static_cast<QMouseEvent*>(pEvent);
		if (m_bDragging && !isMaximized())
		{
			move(pMouseEvent->globalPosition().toPoint() - m_dragPosition);
			return true;
		}
	}
	else if (QEvent::MouseButtonRelease == pEvent->type())
	{
		m_bDragging = false;
	}
	else if (QEvent::MouseButtonDblClick == pEvent->type())
	{
		isMaximized() ? showNormal() : showMaximized();
		return true;
	}
	return QMainWindow::eventFilter(pObject, pEvent);
}
