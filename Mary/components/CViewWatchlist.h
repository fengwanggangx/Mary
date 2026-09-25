#ifndef MARY_COMPONENTS_CVIEWWATCHLIST_H
#define MARY_COMPONENTS_CVIEWWATCHLIST_H

#include <QWidget>
#include <memory>

namespace Ui
{
	class CViewWatchlistClass;
}
class CMarketPageController;

class CViewWatchlist final : public QWidget
{
	public:
		explicit CViewWatchlist(QWidget* pParent = nullptr);
		~CViewWatchlist() override;

	private:
		void showEvent(QShowEvent* pEvent) override;

		std::unique_ptr<Ui::CViewWatchlistClass> m_ui;
		std::unique_ptr<CMarketPageController> m_controller;
		bool m_bLayoutInitialized{ false };
};

#endif
