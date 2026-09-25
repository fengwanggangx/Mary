#include "CUICurve.h"
#include "CUIStyle.h"
#include "ui_CUICurve.h"

#include <QwtPlot.h>

#include <QDate>
#include <QDateTime>
#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QPen>
#include <QScopedValueRollback>
#include <QTimer>

#include <algorithm>
#include <limits>
#include <utility>

namespace
{
	class CBarCandleSeriesData final : public QwtSeriesData<QwtOHLCSample>
	{
		public:
			explicit CBarCandleSeriesData(const std::shared_ptr<const std::vector<CMarketBar>>& bars) : m_bars(bars)
			{
			}

			std::size_t size() const override
			{
				return nullptr == m_bars ? 0 : m_bars->size();
			}

			QwtOHLCSample sample(std::size_t index) const override
			{
				const CMarketBar& bar = (*m_bars)[index];
				return QwtOHLCSample(static_cast<double>(index), bar.m_fOpen, bar.m_fHigh, bar.m_fLow, bar.m_fClose);
			}

			QRectF boundingRect() const override
			{
				if ((nullptr == m_bars) || m_bars->empty())
				{
					return QRectF(1.0, 1.0, -2.0, -2.0);
				}
				double fLow = (std::numeric_limits<double>::max)();
				double fHigh = (std::numeric_limits<double>::lowest)();
				for (const CMarketBar& bar : *m_bars)
				{
					fLow = (std::min)(fLow, bar.m_fLow);
					fHigh = (std::max)(fHigh, bar.m_fHigh);
				}
				// Qwt trading data stores the value axis first; vertical candles transpose this rectangle.
				return QRectF(fLow, 0.0, fHigh - fLow, static_cast<double>(m_bars->size() - 1));
			}

		private:
			std::shared_ptr<const std::vector<CMarketBar>> m_bars;
	};

	class CBarLineSeriesData final : public QwtSeriesData<QPointF>
	{
		public:
			explicit CBarLineSeriesData(const std::shared_ptr<const std::vector<CMarketBar>>& bars) : m_bars(bars)
			{
			}

			std::size_t size() const override
			{
				return nullptr == m_bars ? 0 : m_bars->size();
			}

			QPointF sample(std::size_t index) const override
			{
				return QPointF(static_cast<double>(index), (*m_bars)[index].m_fClose);
			}

			QRectF boundingRect() const override
			{
				if ((nullptr == m_bars) || m_bars->empty())
				{
					return QRectF(1.0, 1.0, -2.0, -2.0);
				}
				double fLow = (std::numeric_limits<double>::max)();
				double fHigh = (std::numeric_limits<double>::lowest)();
				for (const CMarketBar& bar : *m_bars)
				{
					fLow = (std::min)(fLow, bar.m_fClose);
					fHigh = (std::max)(fHigh, bar.m_fClose);
				}
				return QRectF(0.0, fLow, static_cast<double>(m_bars->size() - 1), fHigh - fLow);
			}

		private:
			std::shared_ptr<const std::vector<CMarketBar>> m_bars;
	};

	class CBarVolumeSeriesData final : public QwtSeriesData<QwtIntervalSample>
	{
		public:
			explicit CBarVolumeSeriesData(const std::shared_ptr<const std::vector<CMarketBar>>& bars) : m_bars(bars)
			{
			}

			std::size_t size() const override
			{
				return nullptr == m_bars ? 0 : m_bars->size();
			}

			QwtIntervalSample sample(std::size_t index) const override
			{
				return QwtIntervalSample(static_cast<double>((*m_bars)[index].m_nVolume), static_cast<double>(index) - 0.35, static_cast<double>(index) + 0.35);
			}

			QRectF boundingRect() const override
			{
				if ((nullptr == m_bars) || m_bars->empty())
				{
					return QRectF(1.0, 1.0, -2.0, -2.0);
				}
				std::int64_t nMaximumVolume = 0;
				for (const CMarketBar& bar : *m_bars)
				{
					nMaximumVolume = (std::max)(nMaximumVolume, bar.m_nVolume);
				}
				return QRectF(-0.35, 0.0, static_cast<double>(m_bars->size()), static_cast<double>(nMaximumVolume));
			}

		private:
			std::shared_ptr<const std::vector<CMarketBar>> m_bars;
	};

	QDate BarDate(const CMarketBar& bar)
	{
		return QDateTime::fromMSecsSinceEpoch(bar.m_nBeginTime).date();
	}
} // namespace

CUICurve::CUICurve(QWidget* pParent) : QWidget(pParent), m_ui(std::make_unique<Ui::CUICurveClass>()), m_bars(std::make_shared<const std::vector<CMarketBar>>())
{
	m_ui->setupUi(this);
	InitializePlots();
}

CUICurve::~CUICurve() = default;

void CUICurve::SetMode(CurveMode mode)
{
	if (mode == m_mode)
	{
		return;
	}
	m_mode = mode;
	Refresh();
}

CurveMode CUICurve::GetMode() const noexcept
{
	return m_mode;
}

void CUICurve::SetBars(const std::vector<CMarketBar>& bars)
{
	std::shared_ptr<std::vector<CMarketBar>> values = std::make_shared<std::vector<CMarketBar>>(bars);
	std::sort(values->begin(), values->end(), [](const CMarketBar& left, const CMarketBar& right)
	{
		return left.m_nBeginTime < right.m_nBeginTime;
	});
	m_bars = std::move(values);
	Refresh();
}

void CUICurve::Clear()
{
	m_bars = std::make_shared<const std::vector<CMarketBar>>();
	Refresh();
}

void CUICurve::InitializePlots()
{
	m_pPricePlot = m_ui->pricePlot;
	m_pVolumePlot = m_ui->volumePlot;
	m_pPricePlot->canvas()->setObjectName("curveCanvas");
	m_pVolumePlot->canvas()->setObjectName("curveCanvas");
	m_pVolumePlot->setAxisMaxMajor(QwtAxis::YLeft, 2);
	m_pVolumePlot->setAxisMaxMinor(QwtAxis::YLeft, 0);

	m_pTradingCurve = new QwtPlotTradingCurve("K线");
	m_pTradingCurve->setSymbolStyle(QwtPlotTradingCurve::CandleStick);
	m_pTradingCurve->setSymbolPen(QColor("#d8e5ef"), 1.0);
	m_pTradingCurve->setSymbolBrush(QwtPlotTradingCurve::Increasing, QBrush(QColor("#f04455")));
	m_pTradingCurve->setSymbolBrush(QwtPlotTradingCurve::Decreasing, QBrush(QColor("#00b987")));
	m_pTradingCurve->setMinSymbolWidth(3.0);
	m_pTradingCurve->setMaxSymbolWidth(12.0);
	m_pTradingCurve->attach(m_pPricePlot);

	m_pIntradayCurve = new QwtPlotCurve("分时");
	m_pIntradayCurve->setPen(QPen(QColor("#4a90e2"), 1.4));
	m_pIntradayCurve->attach(m_pPricePlot);

	m_pVolumeCurve = new QwtPlotHistogram("成交量");
	m_pVolumeCurve->setPen(QColor("#607d8b"));
	m_pVolumeCurve->setBrush(QBrush(QColor("#455a64")));
	m_pVolumeCurve->attach(m_pVolumePlot);

	new QwtPlotPanner(m_pPricePlot->canvas());
	new QwtPlotMagnifier(m_pPricePlot->canvas());
	new QwtPlotPanner(m_pVolumePlot->canvas());
	new QwtPlotMagnifier(m_pVolumePlot->canvas());

	ApplyPalette();
}

void CUICurve::changeEvent(QEvent* pEvent)
{
	QWidget::changeEvent(pEvent);
	if ((QEvent::PaletteChange == pEvent->type()) || (QEvent::StyleChange == pEvent->type()))
	{
		SchedulePaletteUpdate();
	}
}

void CUICurve::SchedulePaletteUpdate()
{
	if (m_bPaletteUpdatePending)
	{
		return;
	}
	m_bPaletteUpdatePending = true;
	QTimer::singleShot(0, this, [this]()
	{
		m_bPaletteUpdatePending = false;
		ApplyPalette();
	});
}

void CUICurve::ApplyPalette()
{
	if (m_bApplyingPalette || (nullptr == m_pPricePlot) || (nullptr == m_pVolumePlot))
	{
		return;
	}
	QScopedValueRollback<bool> paletteGuard(m_bApplyingPalette, true);
	QPalette palette = qApp->palette();
	QColor background = palette.color(QPalette::Base);
	QColor foreground = palette.color(QPalette::Text);
	m_pTradingCurve->setSymbolPen(foreground, 1.0);
	QString strStyle = UIStyle::Load(":/styles/curve.qss").arg(palette.color(QPalette::Window).name(), foreground.name(), background.name());
	if (styleSheet() != strStyle)
	{
		setStyleSheet(strStyle);
	}
	for (QwtPlot* pPlot : { m_pPricePlot, m_pVolumePlot })
	{
		pPlot->setPalette(palette);
		pPlot->setAutoFillBackground(true);
		pPlot->setCanvasBackground(background);
		for (int nAxis = 0; nAxis < QwtAxis::AxisPositions; ++nAxis)
		{
			QwtScaleWidget* pAxis = pPlot->axisWidget(nAxis);
			pAxis->setPalette(palette);
			pAxis->setAutoFillBackground(true);
		}
		pPlot->replot();
	}
}

void CUICurve::Refresh()
{
	std::shared_ptr<const std::vector<CMarketBar>> bars = DisplayBars();

	bool bIntraday = CurveMode::Intraday == m_mode;
	m_pTradingCurve->setVisible(!bIntraday);
	m_pIntradayCurve->setVisible(bIntraday);
	m_pTradingCurve->setSamples(new CBarCandleSeriesData(bars));
	m_pIntradayCurve->setSamples(new CBarLineSeriesData(bars));
	m_pVolumeCurve->setSamples(new CBarVolumeSeriesData(bars));
	m_pPricePlot->setAxisAutoScale(QwtAxis::XBottom);
	m_pPricePlot->setAxisAutoScale(QwtAxis::YLeft);
	m_pVolumePlot->setAxisAutoScale(QwtAxis::XBottom);
	m_pVolumePlot->setAxisAutoScale(QwtAxis::YLeft);
	if (!bars->empty())
	{
		for (QwtPlot* plot : { m_pPricePlot, m_pVolumePlot })
		{
			plot->setAxisScale(QwtAxis::XBottom, -0.5, static_cast<double>(bars->size()) - 0.5);
		}
	}
	m_pPricePlot->replot();
	m_pVolumePlot->replot();
}

std::shared_ptr<const std::vector<CMarketBar>> CUICurve::DisplayBars() const
{
	if ((CurveMode::Day == m_mode) || (CurveMode::Intraday == m_mode))
	{
		return m_bars;
	}
	return AggregateBars(CurveMode::Month == m_mode);
}

std::shared_ptr<const std::vector<CMarketBar>> CUICurve::AggregateBars(bool bMonthly) const
{
	std::shared_ptr<std::vector<CMarketBar>> values = std::make_shared<std::vector<CMarketBar>>();
	for (const CMarketBar& bar : *m_bars)
	{
		QDate date = BarDate(bar);
		int nYear = date.year();
		int nPeriod = 0;
		if (bMonthly)
		{
			nPeriod = date.month();
		}
		else
		{
			nPeriod = date.weekNumber(&nYear);
		}

		bool bNewPeriod = values->empty();
		if (!bNewPeriod)
		{
			QDate previousDate = BarDate(values->back());
			int nPreviousYear = previousDate.year();
			int nPreviousPeriod = bMonthly ? previousDate.month() : previousDate.weekNumber(&nPreviousYear);
			bNewPeriod = (nYear != nPreviousYear) || (nPeriod != nPreviousPeriod);
		}
		if (bNewPeriod)
		{
			values->emplace_back(bar);
			continue;
		}

		CMarketBar& value = values->back();
		value.m_fHigh = (std::max)(value.m_fHigh, bar.m_fHigh);
		value.m_fLow = (std::min)(value.m_fLow, bar.m_fLow);
		value.m_fClose = bar.m_fClose;
		value.m_nVolume += bar.m_nVolume;
		value.m_nTurnover += bar.m_nTurnover;
	}
	return values;
}
