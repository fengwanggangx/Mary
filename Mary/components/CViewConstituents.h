#ifndef MARY_COMPONENTS_CVIEWCONSTITUENTS_H
#define MARY_COMPONENTS_CVIEWCONSTITUENTS_H

#include <QWidget>
#include <memory>
#include <vector>

struct CSectorInfo;
struct CSecurity;

namespace Ui
{
	class CViewConstituentsClass;
}
class CMarketPageController;

class CViewConstituents final : public QWidget
{
  public:
	explicit CViewConstituents(QWidget* pParent = nullptr);
	~CViewConstituents() override;
	void SetSector(const CSectorInfo& sector, const std::vector<CSecurity>& securities);

  private:
	void ApplyTheme();
	void changeEvent(QEvent* pEvent) override;

	std::unique_ptr<Ui::CViewConstituentsClass> m_ui;
	std::unique_ptr<CMarketPageController> m_controller;
	QString m_strSector;
};

#endif
