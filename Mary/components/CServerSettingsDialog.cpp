#include "CServerSettingsDialog.h"
#include "CServerSiteDialog.h"
#include "ui_CServerSettingsDialog.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QPushButton>
#include <QTcpSocket>
#include <QUuid>

#include <limits>

namespace
{
	QString MakeSiteText(const configuration::CServerSite& site)
	{
		return QString("%1 (%2:%3)").arg(site.name, site.windHost).arg(site.windPort);
	}
}

CServerSettingsDialog::CServerSettingsDialog(QWidget* pParent) : QDialog(pParent), ui(new Ui::CServerSettingsDialogClass())
{
	ui->setupUi(this);
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
	QString activeSiteId = configuration::CServerSettings::GetActiveSiteId();
	ui->siteCombo->clear();
	for (const configuration::CServerSite& site : configuration::CServerSettings::LoadSites())
	{
		if (site.enabled)
		{
			ui->siteCombo->addItem(MakeSiteText(site), site.id);
		}
	}
	int activeIndex = ui->siteCombo->findData(activeSiteId);
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
	configuration::CServerSite site = configuration::CServerSettings::LoadSite(ui->siteCombo->currentData().toString());
	if (site.id.isEmpty())
	{
		return;
	}
	CServerSiteDialog dialog(site, true, this);
	dialog.exec();
}

void CServerSettingsDialog::SelectFastestSite()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	qint64 bestElapsed = std::numeric_limits<qint64>::max();
	QString bestSiteId;
	for (const configuration::CServerSite& site : configuration::CServerSettings::LoadSites())
	{
		if (!site.enabled || site.windHost.isEmpty() || 0 >= site.windPort)
		{
			continue;
		}

		QTcpSocket socket;
		QElapsedTimer timer;
		timer.start();
		socket.connectToHost(site.windHost, static_cast<quint16>(site.windPort));
		if (socket.waitForConnected(1500))
		{
			qint64 elapsed = timer.elapsed();
			if (elapsed < bestElapsed)
			{
				bestElapsed = elapsed;
				bestSiteId = site.id;
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
	configuration::CServerSite site;
	site.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	site.enabled = true;
	CServerSiteDialog dialog(site, false, this);
	if (QDialog::Accepted != dialog.exec())
	{
		return;
	}
	const configuration::CServerSite& newSite = dialog.GetSite();
	if (!configuration::CServerSettings::SaveSite(newSite))
	{
		QMessageBox::warning(this, "新增失败", "无法写入 ini/system.ini。");
		return;
	}
	LoadSites();
	ui->siteCombo->setCurrentIndex(ui->siteCombo->findData(newSite.id));
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
	if (!configuration::CServerSettings::RemoveSite(siteId))
	{
		QMessageBox::warning(this, "删除失败", "无法删除当前站点。");
		return;
	}
	LoadSites();
}

void CServerSettingsDialog::AcceptSelection()
{
	QString siteId = ui->siteCombo->currentData().toString();
	if (siteId.isEmpty() || !configuration::CServerSettings::SetActiveSiteId(siteId))
	{
		QMessageBox::warning(this, "保存失败", "请选择一个有效站点。");
		return;
	}
	accept();
}
