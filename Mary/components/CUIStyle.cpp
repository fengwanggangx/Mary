#include "CUIStyle.h"

#include <QDebug>
#include <QFile>
#include <QStyle>
#include <QWidget>

namespace UIStyle
{
	QString Load(const QString& strResource)
	{
		QFile file(strResource);
		if (!file.open(QIODevice::ReadOnly))
		{
			qWarning() << "Cannot load QSS:" << strResource;
			return QString();
		}
		return QString::fromUtf8(file.readAll());
	}

	void Apply(QWidget& widget, const QString& strResource)
	{
		QString strStyle = Load(strResource);
		if (widget.styleSheet() != strStyle)
		{
			widget.setStyleSheet(strStyle);
		}
	}

	void Refresh(QWidget& widget)
	{
		widget.style()->unpolish(&widget);
		widget.style()->polish(&widget);
		widget.update();
		for (const auto& child : widget.findChildren<QWidget*>())
		{
			child->style()->unpolish(child);
			child->style()->polish(child);
			child->update();
		}
	}
} // namespace UIStyle
