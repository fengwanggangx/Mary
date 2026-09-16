#ifndef MARY_COMPONENTS_CUICURVE_H
#define MARY_COMPONENTS_CUICURVE_H

#include "../system/CHQMarketService.h"

#include <QWidget>

#include <memory>
#include <vector>

class QwtPlot;
class QwtPlotCurve;
class QwtPlotHistogram;
class QwtPlotTradingCurve;

enum class CurveMode
{
	Intraday,
	Day,
	Week,
	Month
};

class CUICurve final : public QWidget
{
public:
	explicit CUICurve(QWidget* pParent = nullptr);
	~CUICurve() override;

	void SetMode(CurveMode mode);
	CurveMode GetMode() const noexcept;
	void SetBars(const std::vector<CMarketBar>& bars);
	void Clear();

private:
	void InitializePlots();
	void Refresh();
	std::shared_ptr<const std::vector<CMarketBar>> DisplayBars() const;
	std::shared_ptr<const std::vector<CMarketBar>> AggregateBars(bool bMonthly) const;

private:
	CurveMode m_mode{ CurveMode::Day };
	std::shared_ptr<const std::vector<CMarketBar>> m_bars;
	QwtPlot* m_pPricePlot{ nullptr };
	QwtPlot* m_pVolumePlot{ nullptr };
	QwtPlotTradingCurve* m_pTradingCurve{ nullptr };
	QwtPlotCurve* m_pIntradayCurve{ nullptr };
	QwtPlotHistogram* m_pVolumeCurve{ nullptr };
};

#endif
