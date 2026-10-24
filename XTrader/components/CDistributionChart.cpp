#include "CDistributionChart.h"

#include <QPainter>
#include <QStringList>
#include <algorithm>

CDistributionChart::CDistributionChart(QWidget* pParent) : QWidget(pParent)
{
}

void CDistributionChart::SetValues(const QVector<int>& values)
{
	m_values = values;
	update();
}

const QColor& CDistributionChart::GetRisingColor() const
{
	return m_risingColor;
}

const QColor& CDistributionChart::GetFallingColor() const
{
	return m_fallingColor;
}

void CDistributionChart::SetRisingColor(const QColor& color)
{
	m_risingColor = color;
	update();
}

void CDistributionChart::SetFallingColor(const QColor& color)
{
	m_fallingColor = color;
	update();
}

void CDistributionChart::paintEvent(QPaintEvent*)
{
	if (m_values.isEmpty())
	{
		return;
	}
	QPainter painter(this);
	int nMaximum = (std::max)(3, *std::max_element(m_values.begin(), m_values.end()));
	nMaximum = ((nMaximum + 2) / 3) * 3;
	painter.setRenderHint(QPainter::Antialiasing);
	int nLabelWidth = painter.fontMetrics().horizontalAdvance(QString::number(nMaximum)) + 4;
	QRect area = rect().adjusted(nLabelWidth + 3, 10, -8, -25);
	painter.setPen(QPen(palette().color(QPalette::Mid), 0.5));
	for (int nLine = 0; 4 > nLine; ++nLine)
	{
		int nY = area.bottom() - area.height() * nLine / 3;
		painter.drawLine(area.left(), nY, area.right(), nY);
		painter.drawText(QRect(0, nY - 8, nLabelWidth, 16), Qt::AlignRight, QString::number(nLine * nMaximum / 3));
	}
	double fStep = area.width() / static_cast<double>(m_values.size());
	for (int nIndex = 0; m_values.size() > nIndex; ++nIndex)
	{
		painter.fillRect(QRectF(area.left() + nIndex * fStep + 2, area.bottom() - m_values[nIndex] * area.height() / static_cast<double>(nMaximum), fStep - 4, m_values[nIndex] * area.height() / static_cast<double>(nMaximum)), 6 > nIndex ? m_fallingColor : m_risingColor);
	}
	painter.setPen(palette().color(QPalette::Text));
	QStringList labels{ "<-5%", "-3%", "-1%", "0", "+1%", "+3%", ">5%" };
	for (int nIndex = 0; labels.size() > nIndex; ++nIndex)
	{
		painter.drawText(QRectF(area.left() + nIndex * area.width() / 7.0, area.bottom() + 5, area.width() / 7.0, 18), Qt::AlignCenter, labels[nIndex]);
	}
}
