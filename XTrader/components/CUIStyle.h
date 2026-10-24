#ifndef XTRADER_COMPONENTS_CUISTYLE_H
#define XTRADER_COMPONENTS_CUISTYLE_H

#include <QString>
class QWidget;

namespace UIStyle
{
	QString Load(const QString& strResource);
	void Apply(QWidget& widget, const QString& strResource);
	void Refresh(QWidget& widget);
} // namespace UIStyle

#endif
