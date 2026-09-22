#pragma once

#include <QDialog>
#include <memory>
#include "../system/CLoginService.h"
#include "../database/CAccountRepository.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
	class LoginWindowClass;
};
QT_END_NAMESPACE

class LoginWindow : public QDialog
{
	Q_OBJECT

  public:
	LoginWindow(QWidget* pParent = nullptr);
	~LoginWindow();

  protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

  private slots:
	void OnLoginBtnClicked();
	void OnRegisterBtnClicked();
	void OnCloseBtnClicked();
	void OnSettingsBtnClicked();
	void OnAccountChanged(int nIndex);
	void OnAccountEdited(const QString& strAccount);

  private:
	void ConnectSlots();
	void LoadAccounts();
	void FillPassword(int nIndex);
	void SavePendingAccount();
	void DeleteAccount(int nIndex);
	void ClearPendingCredentials();

  private:
	std::unique_ptr<Ui::LoginWindowClass> m_ui;

  private:
	QPoint m_dragPosition;
	bool m_isDragging{ false };
	_TyCallbackId m_loginCallbackId{ 0 };
	CAccountRepository m_accountRepository;
	std::vector<CStoredAccount> m_accounts;
	std::string m_strPendingAccount;
	std::string m_strPendingPassword;
};
