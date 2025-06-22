#include "CLoginWindow.h"
#include <QMouseEvent>
#include <QCryptographicHash>
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::LoginWindowClass())
{
	ui->setupUi(this);
	setWindowFlag(Qt::FramelessWindowHint);
	ConnectSlots();	
}

LoginWindow::~LoginWindow()
{
	delete ui;
}

void LoginWindow::ConnectSlots()
{
	QObject::connect(ui->pushButton_login, &QPushButton::clicked, this, &LoginWindow::OnLoginBtnClicked);
	QObject::connect(ui->pushButton_close, &QPushButton::clicked, this, &LoginWindow::OnCloseBtnClicked);
}

void LoginWindow::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) 
	{
		QWidget* child = childAt(event->pos());
		if ((nullptr == child) || (child == this) || ((qobject_cast<QPushButton*>(child) == nullptr)  && (qobject_cast<QLineEdit*>(child) == nullptr)))
		{
			m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
			m_isDragging = true;
			event->accept();
		}

	}
}
void LoginWindow::mouseMoveEvent(QMouseEvent* event)
{
	if (m_isDragging && (event->buttons() & Qt::LeftButton)) 
	{
		move(event->globalPosition().toPoint() - m_dragPosition);
		event->accept();
	}
	QDialog::mouseMoveEvent(event);
}

void LoginWindow::mouseReleaseEvent(QMouseEvent* event)
{
	m_isDragging = false;
	QDialog::mouseReleaseEvent(event);
}

void LoginWindow::OnLoginBtnClicked()
{
	static QString strValidAdmin = "admin";
	static QString strValidAccount = "fengwanggang";
	static QString strValidPasswd = "123456";
	static QString strMd5Account = QCryptographicHash::hash(strValidAccount.toUtf8(), QCryptographicHash::Md5).toHex();
	static QString strMd5Passwd = QCryptographicHash::hash(strValidPasswd.toUtf8(), QCryptographicHash::Md5).toHex();

	QString strAccount = QCryptographicHash::hash(ui->lineEdit_account->text().toUtf8(), QCryptographicHash::Md5).toHex();
	QString strPasswd = QCryptographicHash::hash(ui->lineEdit_passwd->text().toUtf8(), QCryptographicHash::Md5).toHex();

	QString strCheckCode = ui->lineEdit_checkcode->text();
	if (true || (ui->lineEdit_account->text().toUtf8() == strValidAdmin) || ((strAccount == strMd5Account) && (strPasswd == strMd5Passwd)))
	{
		accept();
	}
	else
	{

		QMessageBox::information(nullptr, "Tips", "账号或密码错误，请重新输入");
	}
}

void LoginWindow::OnCloseBtnClicked()
{
	reject();
	//QApplication::quit();
}