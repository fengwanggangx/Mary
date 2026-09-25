#ifndef MARY_COMPONENTS_CVIEWASHARE_H
#define MARY_COMPONENTS_CVIEWASHARE_H

#include <QWidget>
#include <memory>

namespace Ui
{
	class CViewAShareClass;
}
class CMarketPageController;

class CViewAShare final : public QWidget
{
	public:
		explicit CViewAShare(QWidget* pParent = nullptr);
		~CViewAShare() override;

	private:
		void showEvent(QShowEvent* pEvent) override;

		std::unique_ptr<Ui::CViewAShareClass> m_ui;
		std::unique_ptr<CMarketPageController> m_controller;
		bool m_bLayoutInitialized{ false };
};

#endif
