#ifndef MARY_COMPONENTS_CVIEWHQMARKET_H
#define MARY_COMPONENTS_CVIEWHQMARKET_H

#include <QWidget>
#include <QPointer>
#include <memory>
#include "../service/hqmarket/CHQMarketService.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
	class CViewHQMarketClass;
}
QT_END_NAMESPACE

enum class SessionState;

class CViewHQMarket final : public QWidget
{
	friend class CReviewRegressionTests;

  public:
	explicit CViewHQMarket(QWidget* pParent = nullptr);
	~CViewHQMarket() override;

  private:
	void ApplyTheme();
	static void HandleQuoteTable(QPointer<CViewHQMarket> safeThis, const CDataTableView& view, const CDataChangeSet& changes);
	static void HandleSectorList(QPointer<CViewHQMarket> safeThis, const CSectorListEvent& event);
	static void HandleSectorConstituents(QPointer<CViewHQMarket> safeThis, const CSectorConstituentsEvent& event);
	static void HandleSessionState(QPointer<CViewHQMarket> safeThis, SessionState state, const std::string& strMessage);
	void OnMarketTabChanged(int nIndex);
	void OnRankingCellClicked(int nRow, int nColumn);
	void HandleSectorButtonClicked();
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
