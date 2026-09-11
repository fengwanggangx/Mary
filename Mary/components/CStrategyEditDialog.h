#ifndef MARY_COMPONENTS_CSTRATEGYEDITDIALOG_H
#define MARY_COMPONENTS_CSTRATEGYEDITDIALOG_H

#include "../request/request.pb.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui
{
	class CStrategyEditDialogClass;
}
QT_END_NAMESPACE

class CStrategyEditDialog final : public QDialog
{
	Q_OBJECT

  public:
	explicit CStrategyEditDialog(QWidget* pParent = nullptr);
	~CStrategyEditDialog() override;
	void SetStrategy(const request::StrategyInfo& strategy);
	request::StrategyInfo GetStrategy() const;

  private slots:
	void OnAddParameter();
	void OnRemoveParameter();
	void OnAddSubscription();
	void OnRemoveSubscription();
	void OnAccept();

  private:
	void InitializeUI();

  private:
	Ui::CStrategyEditDialogClass* ui{ nullptr };
	std::uint64_t m_strategyId{ 0 };
};

#endif
