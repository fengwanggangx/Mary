#include "CServerSiteDialog.h"
#include "ui_CServerSiteDialog.h"

#include <QIntValidator>
#include <QMessageBox>
#include <QPushButton>

CServerSiteDialog::CServerSiteDialog(const CHostInfo& site, bool readOnly, QWidget* pParent) : QDialog(pParent), ui(new Ui::CServerSiteDialogClass()), m_site(site)
{
	ui->setupUi(this);
	setWindowTitle(readOnly ? "查看站点" : "新增站点");
	ui->nameEdit->setText(QString::fromStdString(site.m_strName));
	ui->hostEdit->setText(QString::fromStdString(site.m_strHost));
	ui->portEdit->setText(QString::number(0 < site.m_nPort ? site.m_nPort : 8601));
	ui->portEdit->setValidator(new QIntValidator(1, 65535, ui->portEdit));
	ui->nameEdit->setReadOnly(readOnly);
	ui->hostEdit->setReadOnly(readOnly);
	ui->portEdit->setReadOnly(readOnly);
	ui->buttonBox->setVisible(!readOnly);
	if (readOnly)
	{
		ui->rootLayout->activate();
		setFixedHeight(ui->rootLayout->sizeHint().height() + ui->portEdit->sizeHint().height());
	}
	ui->buttonBox->button(QDialogButtonBox::Save)->setText("保存");
	ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("退出");
	connect(ui->buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &CServerSiteDialog::AcceptSite);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

CServerSiteDialog::~CServerSiteDialog()
{
	delete ui;
}

const CHostInfo& CServerSiteDialog::GetSite() const
{
	return m_site;
}

void CServerSiteDialog::AcceptSite()
{
	QString name = ui->nameEdit->text().trimmed();
	QString host = ui->hostEdit->text().trimmed();
	if (name.isEmpty() || host.isEmpty())
	{
		QMessageBox::warning(this, "保存失败", "主站名称和地址不能为空。");
		return;
	}

	bool portValid = false;
	int port = ui->portEdit->text().toInt(&portValid);
	if (!portValid || 1 > port || 65535 < port)
	{
		QMessageBox::warning(this, "端口错误", "端口必须是 1 到 65535 之间的数字。");
		return;
	}

	m_site.m_strName = name.toStdString();
	m_site.m_strHost = host.toStdString();
	m_site.m_nPort = static_cast<unsigned int>(port);
	accept();
}
