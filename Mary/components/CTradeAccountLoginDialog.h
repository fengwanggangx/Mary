#pragma once
#include "../service/account/CTradingAccountService.h"
#include <QDialog>
#include <memory>
namespace Ui
{
	class CTradeAccountLoginDialog;
}
class CTradeAccountLoginDialog final : public QDialog
{
	public:
	explicit CTradeAccountLoginDialog(QWidget* pParent = nullptr);
	~CTradeAccountLoginDialog() override;

	protected:
	bool eventFilter(QObject* pObject, QEvent* pEvent) override;

	private:
	void Login();
	void HandleResult(const CTradingAccountService::CResult& result);
	void SetBusy(bool bBusy);
	void ShowError(const QString& strMessage);
	std::unique_ptr<Ui::CTradeAccountLoginDialog> m_ui;
	_TyCallbackId m_nToken{ 0 };
	std::uint64_t m_nRequestId{ 0 };
};
