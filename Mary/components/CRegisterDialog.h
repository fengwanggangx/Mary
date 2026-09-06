#pragma once

#include "ui_CRegisterDialog.h"
#include "../auth/CLoginService.h"

#include <QDialog>

class CRegisterDialog final : public QDialog
{
	Q_OBJECT

public:
	explicit CRegisterDialog(QWidget* parent = nullptr);
	~CRegisterDialog() override;

	void SetAccount(const QString& account);
	QString Account() const;

private slots:
	void Register();

private:
	Ui::CRegisterDialogClass m_ui;
	_TyCallbackId m_callbackId{ 0 };
};
