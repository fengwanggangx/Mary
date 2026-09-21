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
	static void OnQuoteTableUpdate(QPointer<CViewHQMarket> pInstance, const CDataTableView& view, const CDataChangeSet& changes);
	static void OnSectorListUpdate(QPointer<CViewHQMarket> pInstance, const CSectorListEvent& ev);
	static void OnSectorConstituentsUpdate(QPointer<CViewHQMarket> pInstance, const CSectorConstituentsEvent& ev);
	static void OnSessionStateChanged(QPointer<CViewHQMarket> pInstance, SessionState state, const std::string& strMessage);

private:
	void OnTabChanged(int nIndex);
	void OnRankingCellClicked(int nRow, int nColumn);

  private:
	void ApplyTheme();
	void OnSectorButtonClicked();
	void RefreshQuotes(const CDataTableView& view);
	void RequestSectors();
	void RefreshSectors(const CSectorListEvent& ev);
	void RefreshConstituents(const CSectorConstituentsEvent& ev);
	void SelectSector(const QString& strSectorCode);
	void changeEvent(QEvent* pEvent) override;

private:
	CHQMarketService::_TyHandlerToken m_nQuoteTableToken{ 0 };
	CHQMarketService::_TyHandlerToken m_nSectorListToken{ 0 };
	CHQMarketService::_TyHandlerToken m_nSectorConstituentsToken{ 0 };
	std::vector<CSectorInfo> m_sectors;
	QString m_strSelectedSectorCode;
	bool m_bOverviewRequested{ false };
	std::unique_ptr<Ui::CViewHQMarketClass> m_ui;
};

#endif
