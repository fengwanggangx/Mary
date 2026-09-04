#include "CServerSettingsDialog.h"
#include "ui_CServerSettingsDialog.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>

CServerSettingsDialog::CServerSettingsDialog(QWidget* pParent) : QDialog(pParent), ui(new Ui::CServerSettingsDialogClass())
{
	ui->setupUi(this);
	connect(ui->siteCombo, &QComboBox::currentIndexChanged, this, &CServerSettingsDialog::ShowSite);
	connect(ui->addButton, &QPushButton::clicked, this, &CServerSettingsDialog::AddSite);
	connect(ui->removeButton, &QPushButton::clicked, this, &CServerSettingsDialog::RemoveSite);
	connect(ui->saveButton, &QPushButton::clicked, this, &CServerSettingsDialog::SaveSite);
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
	QString activeSiteId = configuration::CServerSettings::GetActiveSiteId();
	for (const configuration::CServerSite& site : configuration::CServerSettings::LoadSites())
	{
		ui->siteCombo->addItem(site.name, site.id);
	}
	int activeIndex = ui->siteCombo->findData(activeSiteId);
	ui->siteCombo->setCurrentIndex(0 <= activeIndex ? activeIndex : 0);
	ui->removeButton->setEnabled(0 < ui->siteCombo->count());
	ShowSite(ui->siteCombo->currentIndex());
}

void CServerSettingsDialog::ShowSite(int index)
{
	configuration::CServerSite site = configuration::CServerSettings::LoadSite(ui->siteCombo->itemData(index).toString());
	ui->nameEdit->setText(site.name);
	ui->windHostEdit->setText(site.windHost);
	ui->windPortSpin->setValue(0 < site.windPort ? site.windPort : 9877);
	ui->hqMarketHostEdit->setText(site.hqMarketHost);
	ui->hqMarketPortSpin->setValue(0 < site.hqMarketPort ? site.hqMarketPort : 9901);
	ui->enabledCheck->setChecked(site.enabled);
}

void CServerSettingsDialog::AddSite()
{
	bool accepted = false;
	QString siteId = QInputDialog::getText(this, "新增站点", "站点 ID", QLineEdit::Normal, QString(), &accepted).trimmed();
	if (!accepted || siteId.isEmpty())
	{
		return;
	}
	if (!QRegularExpression("^[A-Za-z0-9_-]+$").match(siteId).hasMatch())
	{
		QMessageBox::warning(this, "新增失败", "站点 ID 只能包含字母、数字、下划线和减号。");
		return;
	}
	if (!configuration::CServerSettings::LoadSite(siteId).id.isEmpty())
	{
		QMessageBox::warning(this, "新增失败", "站点 ID 已存在。");
		return;
	}
	configuration::CServerSite site;
	site.id = siteId;
	site.name = siteId;
	site.windPort = 9877;
	site.hqMarketPort = 9901;
	configuration::CServerSettings::SaveSite(site);
	LoadSites();
	ui->siteCombo->setCurrentIndex(ui->siteCombo->findData(siteId));
}

void CServerSettingsDialog::RemoveSite()
{
	QString siteId = ui->siteCombo->currentData().toString();
	if (siteId.isEmpty())
	{
		return;
	}
	if (QMessageBox::Yes != QMessageBox::question(this, "删除站点", "确定删除当前站点？"))
	{
		return;
	}
	configuration::CServerSettings::RemoveSite(siteId);
	LoadSites();
}

configuration::CServerSite CServerSettingsDialog::ReadSite() const
{
	configuration::CServerSite site;
	site.id = ui->siteCombo->currentData().toString();
	site.name = ui->nameEdit->text().trimmed();
	site.windHost = ui->windHostEdit->text().trimmed();
	site.windPort = ui->windPortSpin->value();
	site.hqMarketHost = ui->hqMarketHostEdit->text().trimmed();
	site.hqMarketPort = ui->hqMarketPortSpin->value();
	site.enabled = ui->enabledCheck->isChecked();
	return site;
}

void CServerSettingsDialog::SaveSite()
{
	configuration::CServerSite site = ReadSite();
	if (site.id.isEmpty() || site.name.isEmpty() || site.windHost.isEmpty() || site.hqMarketHost.isEmpty())
	{
		QMessageBox::warning(this, "保存失败", "站点名称和服务器地址不能为空。");
		return;
	}
	if (!configuration::CServerSettings::SaveSite(site) || !configuration::CServerSettings::SetActiveSiteId(site.id))
	{
		QMessageBox::warning(this, "保存失败", "无法写入 ini/system.ini。");
		return;
	}
	accept();
}
