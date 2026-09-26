#ifndef MARY_COMPONENTS_CUICURVE_H
#define MARY_COMPONENTS_CUICURVE_H

#include "../service/hqmarket/CHQMarketService.h"

#include <QWidget>

#include <memory>
#include <vector>

namespace Ui
{
	class CUICurveClass;
}

class QwtPlot;
class QwtPlotCurve;
class QwtPlotHistogram;
class QwtPlotTradingCurve;
class QwtPlotGrid;
class QwtPlotPicker;

enum class CurveMode
{
	Intraday,
	Day,
	Week,
	Month,
	Minute5,
	Minute15,
	Minute30,
	Minute60
};

class CUICurve final : public QWidget
{
	public:
		explicit CUICurve(QWidget* pParent = nullptr);
		~CUICurve() override;

		void SetMode(CurveMode mode);
		CurveMode GetMode() const noexcept;
		bool IsMinuteMode() const noexcept;
		void SetIntradayDays(int nDays);
		int AvailableTradingDays() const;
		void SetReferencePrice(double fPrice);
		void SetBars(const std::vector<CMarketBar>& bars);
		void Clear();

	protected:
		void changeEvent(QEvent* pEvent) override;

	private:
		void ApplyPalette();
		void SchedulePaletteUpdate();
		void InitializePlots();
		void Refresh();
		std::shared_ptr<const std::vector<CMarketBar>> ModeBars() const;
		std::shared_ptr<const std::vector<CMarketBar>> DisplayBars() const;
		std::shared_ptr<const std::vector<CMarketBar>> AggregateBars(bool bMonthly) const;
		std::shared_ptr<const std::vector<CMarketBar>> AggregateMinuteBars(int nMinutes) const;

	private:
		std::unique_ptr<Ui::CUICurveClass> m_ui;
		bool m_bApplyingPalette{ false };
		bool m_bPaletteUpdatePending{ false };
		bool m_bSynchronizingScale{ false };
		CurveMode m_mode{ CurveMode::Day };
		int m_nIntradayDays{ 1 };
		double m_fReferencePrice{ 0.0 };
		std::shared_ptr<const std::vector<CMarketBar>> m_bars;
		QwtPlot* m_pPricePlot{ nullptr };
		QwtPlot* m_pVolumePlot{ nullptr };
		QwtPlotTradingCurve* m_pTradingCurve{ nullptr };
		QwtPlotCurve* m_pIntradayCurve{ nullptr };
		QwtPlotCurve* m_pAverageCurve{ nullptr };
		QwtPlotCurve* m_pMovingAverage5{ nullptr };
		QwtPlotCurve* m_pMovingAverage10{ nullptr };
		QwtPlotCurve* m_pMovingAverage20{ nullptr };
		QwtPlotCurve* m_pMovingAverage60{ nullptr };
		QwtPlotHistogram* m_pVolumeCurve{ nullptr };
		QwtPlotGrid* m_pPriceGrid{ nullptr };
		QwtPlotGrid* m_pVolumeGrid{ nullptr };
		QwtPlotPicker* m_pPicker{ nullptr };
};

#endif
