#ifndef MARY_COMPONENTS_CSERVERSITEDIALOG_H
#define MARY_COMPONENTS_CSERVERSITEDIALOG_H

#include "../configuration/CServerSettings.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class CServerSiteDialogClass; }
QT_END_NAMESPACE

class CServerSiteDialog final : public QDialog
{
public:
	explicit CServerSiteDialog(const configuration::CServerSite& site, bool readOnly, QWidget* pParent = nullptr);
	~CServerSiteDialog() override;

	const configuration::CServerSite& GetSite() const;

private:
	void AcceptSite();

	Ui::CServerSiteDialogClass* ui{nullptr};
	configuration::CServerSite m_site;
};

#endif
