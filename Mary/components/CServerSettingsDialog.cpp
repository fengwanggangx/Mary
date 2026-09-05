#include "CServerSettingsDialog.h"
#include "CServerSiteDialog.h"
#include "ui_CServerSettingsDialog.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QPushButton>
#include <QTcpSocket>
#include <limits>
#include <exception>

namespace
{
	QString MakeSiteText(const configuration::CHostInfo& site)
	{
		return QString("%1 (%2:%3)").arg(QString::fromStdString(site.m_strName), QString::fromStdString(site.m_strHost)).arg(site.m_nPort);
	}
}

CServerSettingsDialog::CServerSettingsDialog(QWidget* pParent) : QDialog(pParent), ui(new Ui::CServerSettingsDialogClass())
{
	ui->setupUi(this);
	configuration::CHostMgr::InstanceRef().Initialize();
	connect(ui->siteCombo, &QComboBox::currentIndexChanged, this, &CServerSettingsDialog::UpdateButtons);
	connect(ui->viewButton, &QPushButton::clicked, this, &CServerSettingsDialog::ViewSite);
	connect(ui->autoFastestCheck, &QCheckBox::toggled, this, [this](bool checked)
	{
		if (checked)
		{
			SelectFastestSite();
		}
	});
	connect(ui->addButton, &QPushButton::clicked, this, &CServerSettingsDialog::AddSite);
	connect(ui->removeButton, &QPushButton::clicked, this, &CServerSettingsDialog::RemoveSite);
	connect(ui->okButton, &QPushButton::clicked, this, &CServerSettingsDialog::AcceptSelection);
	connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
	LoadSites();
}

CServerSettingsDialog::~CServerSettingsDialog()
{
	delete ui;
}

void CServerSettingsDialog::LoadSites()
{
	ui->siteCombo->clear();
	for (const auto& v : configuration::CHostMgr::InstanceRef().GetHosts())
	{
		const auto& site = v.second;
		if (site.m_bEnabled)
		{
			ui->siteCombo->addItem(MakeSiteText(site), QString::fromStdString(site.m_strKey));
		}
	}
	int activeIndex = 0;
	ui->siteCombo->setCurrentIndex(0 <= activeIndex ? activeIndex : 0);
	UpdateButtons();
}

void CServerSettingsDialog::UpdateButtons()
{
	bool hasSite = 0 <= ui->siteCombo->currentIndex();
	ui->viewButton->setEnabled(hasSite);
	ui->removeButton->setEnabled(hasSite);
	ui->autoFastestCheck->setEnabled(0 < ui->siteCombo->count());
	ui->okButton->setEnabled(hasSite);
}

void CServerSettingsDialog::ViewSite()
{
	const QString key = ui->siteCombo->currentData().toString();
	for (const auto& v : configuration::CHostMgr::InstanceRef().GetHosts())
	{
		const auto& site = v.second;
		if (key.toStdString() == site.m_strKey)
		{
			CServerSiteDialog dialog(site, true, this);
			dialog.exec();
			return;
		}
	}
}

void CServerSettingsDialog::SelectFastestSite()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	qint64 bestElapsed = std::numeric_limits<qint64>::max();
	QString bestSiteId;
	for (const auto v : configuration::CHostMgr::InstanceRef().GetHosts())
	{
		const auto& site = v.second;
		if (!site.m_bEnabled || site.m_strHost.empty() || 0 >= site.m_nPort)
		{
			continue;
		}

		QTcpSocket socket;
		QElapsedTimer timer;
		timer.start();
		socket.connectToHost(QString::fromStdString(site.m_strHost), static_cast<quint16>(site.m_nPort));
		if (socket.waitForConnected(1500))
		{
			qint64 elapsed = timer.elapsed();
			if (elapsed < bestElapsed)
			{
				bestElapsed = elapsed;
				bestSiteId = QString::fromStdString(site.m_strKey);
			}
			socket.abort();
		}
	}
	QApplication::restoreOverrideCursor();

	int bestIndex = ui->siteCombo->findData(bestSiteId);
	if (0 > bestIndex)
	{
		QMessageBox::warning(this, "选择最快", "没有可连接的站点，请检查地址和端口。");
		return;
	}
	ui->siteCombo->setCurrentIndex(bestIndex);
	QMessageBox::information(this, "选择最快", QString("已选择 %1，连接耗时 %2 毫秒。").arg(ui->siteCombo->currentText()).arg(bestElapsed));
}

void CServerSettingsDialog::AddSite()
{
	configuration::CHostInfo site;
	site.m_strKey = "host" + std::to_string(configuration::CHostMgr::InstanceRef().GetHosts().size() + 1);
	site.m_bEnabled = true;
	CServerSiteDialog dialog(site, false, this);
	if (QDialog::Accepted != dialog.exec())
	{
		return;
	}
	const configuration::CHostInfo& newSite = dialog.GetSite();
	try
	{
		configuration::CHostMgr::InstanceRef().Add(newSite);
	}
	catch (const std::exception&)
	{
		QMessageBox::warning(this, "新增失败", "无法写入 ini/system.ini。");
		return;
	}
	LoadSites();
	ui->siteCombo->setCurrentIndex(ui->siteCombo->findData(QString::fromStdString(newSite.m_strKey)));
}

void CServerSettingsDialog::RemoveSite()
{
	QString siteId = ui->siteCombo->currentData().toString();
	if (siteId.isEmpty())
	{
		return;
	}
	QMessageBox messageBox(QMessageBox::Question, "删除站点", "确定删除当前站点？", QMessageBox::Yes | QMessageBox::No, this);
	messageBox.button(QMessageBox::Yes)->setText("是");
	messageBox.button(QMessageBox::No)->setText("否");
	if (QMessageBox::Yes != messageBox.exec())
	{
		return;
	}
	try
	{
		configuration::CHostMgr::InstanceRef().Remove(siteId.toStdString());
	}
	catch (const std::exception&)
	{
		QMessageBox::warning(this, "删除失败", "无法删除当前站点。");
		return;
	}
	LoadSites();
}

void CServerSettingsDialog::AcceptSelection()
{
	if (ui->siteCombo->currentData().toString().isEmpty())
	{
		QMessageBox::warning(this, "保存失败", "请选择一个有效站点。");
		return;
	}
	accept();
}
