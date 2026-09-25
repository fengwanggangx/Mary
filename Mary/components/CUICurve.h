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

	protected:
		void changeEvent(QEvent* pEvent) override;

	private:
		void ApplyPalette();
		void SchedulePaletteUpdate();
		void InitializePlots();
		void Refresh();
		std::shared_ptr<const std::vector<CMarketBar>> DisplayBars() const;
		std::shared_ptr<const std::vector<CMarketBar>> AggregateBars(bool bMonthly) const;

	private:
		std::unique_ptr<Ui::CUICurveClass> m_ui;
		bool m_bApplyingPalette{ false };
		bool m_bPaletteUpdatePending{ false };
		CurveMode m_mode{ CurveMode::Day };
		std::shared_ptr<const std::vector<CMarketBar>> m_bars;
		QwtPlot* m_pPricePlot{ nullptr };
		QwtPlot* m_pVolumePlot{ nullptr };
		QwtPlotTradingCurve* m_pTradingCurve{ nullptr };
		QwtPlotCurve* m_pIntradayCurve{ nullptr };
		QwtPlotHistogram* m_pVolumeCurve{ nullptr };
};

#endif
