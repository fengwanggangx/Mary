#ifndef MARY_COMPONENTS_CVIEWHQMARKET_H
#define MARY_COMPONENTS_CVIEWHQMARKET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class CViewHQMarketClass; }
QT_END_NAMESPACE

class CViewHQMarket final : public QWidget
{
public:
	explicit CViewHQMarket(QWidget* pParent = nullptr);
	~CViewHQMarket() override;

private:
	Ui::CViewHQMarketClass* ui{nullptr};
};

#endif
