#ifndef MARY_COMPONENTS_CSERVERSETTINGSDIALOG_H
#define MARY_COMPONENTS_CSERVERSETTINGSDIALOG_H

#include "../configuration/CServerSettings.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class CServerSettingsDialogClass; }
QT_END_NAMESPACE

class CServerSettingsDialog final : public QDialog
{
public:
	explicit CServerSettingsDialog(QWidget* pParent = nullptr);
	~CServerSettingsDialog() override;

private:
	void LoadSites();
	void ShowSite(int index);
	void AddSite();
	void RemoveSite();
	void SaveSite();
	configuration::CServerSite ReadSite() const;

	Ui::CServerSettingsDialogClass* ui{nullptr};
};

#endif
