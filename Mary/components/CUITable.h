#ifndef MARY_COMPONENTS_CUITABLE_H
#define MARY_COMPONENTS_CUITABLE_H

#include <QWidget>

#include <cstdint>
#include <string>
#include <vector>

struct CQuote;
struct CSecurity;
struct CMarketDepth;
struct CMarketBar;
struct CDataChangeSet;
class CDataSnapshot;
enum class MarketBarPeriod;
enum class CurveMode;
class CUICurve;
class CDataTableModel;
class CMarketFilterProxyModel;
class QTableView;
class QModelIndex;

QT_BEGIN_NAMESPACE
namespace Ui { class CUITableClass; }
QT_END_NAMESPACE

class CUITable final : public QWidget
{
	Q_OBJECT

public:
	explicit CUITable(QWidget* pParent = nullptr);
	~CUITable() override;

private slots:
	void OnFilterChanged(const QString& strText);
	void OnMarketFilterChanged(int nIndex);
	void OnStatusFilterChanged(int nIndex);

private:
	void InitializeUI();
	void BindService();
	void HandleDepth(const CMarketDepth& depth);
	void HandleHistory(const std::string& strSecurity, MarketBarPeriod period, const std::vector<CMarketBar>& bars, const std::string& strError);
	void HandleQuoteTable(const CDataSnapshot& snapshot, const CDataChangeSet& changes);
	void OnCurrentRowChanged(const QModelIndex& current, const QModelIndex& previous);
	CSecurity GetSecurity(const QModelIndex& index) const;
	QString GetName(const QModelIndex& index) const;
	void RequestHistory(CurveMode mode);

private:
	Ui::CUITableClass* ui{ nullptr };
	CUICurve* m_pCurve{ nullptr };
	QTableView* m_pWatchlistTable{ nullptr };
	CDataTableModel* m_pWatchlistModel{ nullptr };
	CMarketFilterProxyModel* m_pWatchlistProxy{ nullptr };
	std::uint64_t m_quoteTableHandlerToken{ 0 };
	std::uint64_t m_depthHandlerToken{ 0 };
	std::uint64_t m_historyHandlerToken{ 0 };
};

#endif
