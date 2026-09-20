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
	friend class CReviewRegressionTests;

  public:
	explicit CViewHQMarket(QWidget* pParent = nullptr);
	~CViewHQMarket() override;

  private:
	void ApplyTheme();
	void RefreshQuotes(const CDataTableView& view);
	void RequestSectors();
	void RefreshSectors(const CSectorListEvent& event);
	void RefreshConstituents(const CSectorConstituentsEvent& event);
	void SelectSector(const QString& strSectorCode);
	CHQMarketService::_TyHandlerToken m_nQuoteTableToken{ 0 };
	CHQMarketService::_TyHandlerToken m_nSectorListToken{ 0 };
	CHQMarketService::_TyHandlerToken m_nSectorConstituentsToken{ 0 };
	std::vector<CSectorInfo> m_sectors;
	QString m_strSelectedSectorCode;
	bool m_bOverviewRequested{ false };
	void changeEvent(QEvent* event) override;
	std::unique_ptr<Ui::CViewHQMarketClass> m_ui;
};

#endif
