#ifndef MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H
#define MARY_COMPONENTS_CVIEWSTRATEGYSETTINGS_H

#include <QWidget>

#include "../request/request.pb.h"

#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui
{
	class CViewStrategySettingsClass;
}
QT_END_NAMESPACE

class CViewStrategySettings final : public QWidget
{
	Q_OBJECT

  public:
	explicit CViewStrategySettings(QWidget* pParent = nullptr);
	~CViewStrategySettings() override;

  private slots:
	void OnRefresh();
	void OnAdd();
	void OnModify();
	void OnDelete();
	void OnFilterChanged();

  private:
	void InitializeUI();
	void BindService();
	void RefreshTable();
	void SetBusy(bool bBusy, const QString& strMessage = QString());
	const request::StrategyInfo* GetSelectedStrategy() const;
	void HandleQueryResult(const std::vector<request::StrategyInfo>& strategies, const std::string& strError);
	void HandleOperationResult(const std::string& strCommand, bool bSuccess, const request::StrategyInfo& strategy, const std::string& strError);

  private:
	Ui::CViewStrategySettingsClass* ui{ nullptr };
	std::vector<request::StrategyInfo> m_strategies;
};

#endif
