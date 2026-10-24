#include "../XTrader/components/CSinglePixelBorder.h"
#include <QApplication>
#include <QLineEdit>
#include <QImage>
#include <QDebug>
int main(int nArgc, char** pArgv)
{
	QApplication app(nArgc, pArgv);
	QWidget window;
	window.resize(260, 100);
	QLineEdit edit(&window);
	edit.setGeometry(7, 9, 200, 40);
	edit.setFrame(false);
	edit.setStyleSheet("QLineEdit { border: none; background: white; } QWidget#singlePixelBorder { border: none; background: transparent; color: #cbd7e8; selection-background-color: #2563eb; }");
	CSinglePixelBorder::Install(edit);
	window.show();
	app.processEvents();
	for (int nOffset = 0; 4 > nOffset; ++nOffset)
	{
		edit.setGeometry(7 + nOffset, 9 + nOffset, 200 + nOffset, 40 + nOffset);
		for (int nState = 0; 2 > nState; ++nState)
		{
			if (0 == nState)
			{
				edit.clearFocus();
			}
			else
			{
				edit.setFocus();
			}
			app.processEvents();
			// Capture the parent, preserving fractional child offsets in the backing store.
			QImage image = window.grab().toImage();
			qreal fScale = edit.devicePixelRatioF();
			int nLeft = qCeil(edit.x() * fScale);
			int nTop = qCeil(edit.y() * fScale);
			int nRight = qFloor((edit.x() + edit.width()) * fScale) - 1;
			int nBottom = qFloor((edit.y() + edit.height()) * fScale) - 1;
			QColor color(0 == nState ? "#cbd7e8" : "#2563eb");
			int nCenterX = (nLeft + nRight) / 2;
			int nCenterY = (nTop + nBottom) / 2;
			if ((color != image.pixelColor(nCenterX, nTop)) || (color == image.pixelColor(nCenterX, nTop + 1)) || (color != image.pixelColor(nCenterX, nBottom)) || (color == image.pixelColor(nCenterX, nBottom - 1)) || (color != image.pixelColor(nLeft, nCenterY)) || (color == image.pixelColor(nLeft + 1, nCenterY)) || (color != image.pixelColor(nRight, nCenterY)) || (color == image.pixelColor(nRight - 1, nCenterY)))
			{
				image.save("SinglePixelBorderFailure.png");
				qCritical() << "Border pixel test failed" << fScale << nState << nOffset;
				return 1;
			}
		}
	}

	qInfo() << "Four edges are exactly one physical pixel, normal and focused; DPR=" << edit.devicePixelRatioF();
	return 0;
}
