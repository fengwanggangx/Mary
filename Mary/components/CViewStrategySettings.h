#ifndef MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H
#define MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class CViewStrategySettingsClass; }
QT_END_NAMESPACE

class CViewStrategySettings final : public QWidget
{
public:
	explicit CViewStrategySettings(QWidget* pParent = nullptr);
	~CViewStrategySettings() override;

private:
	Ui::CViewStrategySettingsClass* ui{nullptr};
};

#endif
