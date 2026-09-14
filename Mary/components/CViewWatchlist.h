#ifndef MARY_COMPONENTS_CVIEWWATCHLIST_H
#define MARY_COMPONENTS_CVIEWWATCHLIST_H

#include <QWidget>

struct CMarketQuoteSnapshot;

QT_BEGIN_NAMESPACE
namespace Ui { class CViewWatchlistClass; }
QT_END_NAMESPACE

class CViewWatchlist final : public QWidget
{
	Q_OBJECT

public:
	explicit CViewWatchlist(QWidget* pParent = nullptr);
	~CViewWatchlist() override;

private slots:
	void OnCurrentRowChanged(int nCurrentRow, int nCurrentColumn, int nPreviousRow, int nPreviousColumn);
	void OnFilterChanged(const QString& strText);

private:
	void InitializeUI();
	void InitializeWatchlist();
	void BindService();
	void HandleQuote(const CMarketQuoteSnapshot& snapshot);
	int FindSecurityRow(const QString& strSecurity) const;

private:
	Ui::CViewWatchlistClass* ui{ nullptr };
};

#endif
