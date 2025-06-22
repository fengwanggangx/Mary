#pragma once

#include <QDialog>
#include "ui_CLoginWindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LoginWindowClass; };
QT_END_NAMESPACE

class LoginWindow : public QDialog
{
	Q_OBJECT

public:
	LoginWindow(QWidget *parent = nullptr);
	~LoginWindow();

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
	void OnLoginBtnClicked();
	void OnCloseBtnClicked();

private:
	void ConnectSlots();

private:
	Ui::LoginWindowClass *ui;

private:
	QPoint m_dragPosition;
	bool m_isDragging{ false };
};
