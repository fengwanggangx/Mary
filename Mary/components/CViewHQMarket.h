#ifndef MARY_COMPONENTS_CVIEWHQMARKET_H
#define MARY_COMPONENTS_CVIEWHQMARKET_H

#include <QWidget>
#include <memory>
#include "../service/hqmarket/CHQMarketService.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
	class CViewHQMarketClass;
}
QT_END_NAMESPACE

class CViewHQMarket final : public QWidget
{
	public:
		explicit CViewHQMarket(QWidget* pParent = nullptr);
		~CViewHQMarket() override;

	private:
		void ApplyTheme();
		void RefreshQuotes(const CDataTableView& view);
		CHQMarketService::_TyHandlerToken m_nQuoteTableToken{ 0 };
		void changeEvent(QEvent* event) override;
		std::unique_ptr<Ui::CViewHQMarketClass> m_ui;
};

#endif
