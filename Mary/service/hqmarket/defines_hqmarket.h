#ifndef MARY_SYSTEM_DEFINES_HQMARKET_H
#define MARY_SYSTEM_DEFINES_HQMARKET_H

#include "../../basic/CDatable.h"
#include "../../request/MarketTypes.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

// 单只证券的最新行情快照
struct CQuote
{
	CQuote() = default;
	CQuote(const CSecurity& security) : m_security(security)
	{
	}
	CSecurity m_security;			// 证券信息
	std::uint64_t m_nSequence{ 0 }; // 行情序列号
	double m_fLastPrice{ 0.0 };		// 最新成交价
	double m_fPreClose{ 0.0 };		// 前收盘价
	std::int64_t m_nVolume{ 0 };	// 成交量
	bool m_bStale{ false };			// 是否为延迟行情
};

enum class MarketQuoteColumn : _TyDataColumnId
{
	Security = 1,
	Name,
	LastPrice,
	Change,
	Percent,
	PreClose,
	Volume,
	Status,
	Sequence,
	ListingStatus,
	Market
};

enum class MarketBarPeriod
{
	Minute,
	Day
};

// 盘口中的单档价格和数量
struct CPriceLevel
{
	double m_fPrice{ 0.0 };		 // 委托价格
	std::int64_t m_nVolume{ 0 }; // 委托数量
};

// 单只证券的盘口深度快照
struct CMarketDepth
{
	CSecurity m_security;			   // 证券信息
	std::int64_t m_nExchangeTime{ 0 }; // 交易所时间戳
	std::vector<CPriceLevel> m_bids;   // 买盘档位
	std::vector<CPriceLevel> m_asks;   // 卖盘档位
	bool m_bStale{ false };			   // 是否为延迟行情
};

// 单个时间周期的行情 K 线
struct CMarketBar
{
	std::int64_t m_nBeginTime{ 0 }; // 周期开始时间戳
	double m_fOpen{ 0.0 };			// 开盘价
	double m_fHigh{ 0.0 };			// 最高价
	double m_fLow{ 0.0 };			// 最低价
	double m_fClose{ 0.0 };			// 收盘价
	std::int64_t m_nVolume{ 0 };	// 成交量
	std::int64_t m_nTurnover{ 0 };	// 成交额
};

// 技术指标在指定时间的计算结果
struct CIndicatorPoint
{
	std::int64_t m_nTime{ 0 }; // 指标对应时间戳
	double m_fValue{ 0.0 };	   // 指标值
	bool m_bValid{ false };	   // 指标值是否有效
};

// 历史行情查询完成事件
struct CMarketHistoryEvent
{
	std::uint64_t m_requestId{ 0 };
	std::string m_strSecurity;						  // 证券代码
	MarketBarPeriod m_period{ MarketBarPeriod::Day }; // K 线周期
	std::vector<CMarketBar> m_bars;					  // 历史 K 线数据
	std::string m_strError;							  // 查询错误信息
};

// 行情表提交后的快照和变更事件
struct CQuoteTableEvent
{
	CDataTableView m_view;	  // 行情表视图
	CDataChangeSet m_changes; // 本次变更集合
};

// 证券列表查询完成事件
struct CSecurityListEvent
{
	std::int64_t m_version{ 0 };		 // 证券列表版本号
	std::vector<CSecurity> m_securities; // 证券列表
};

struct CSectorInfo
{
	SectorType m_type{ SectorType::unknown };
	std::string m_strCode;
	std::string m_strName;
	double m_fChangePercent{ 0.0 };
	int m_nRisingCount{ 0 };
	int m_nFallingCount{ 0 };
	int m_nFlatCount{ 0 };
	int m_nMemberCount{ 0 };
	CSecurity m_leadingSecurity;
	std::string m_strLeadingName;
	std::int64_t m_nSnapshotTime{ 0 };
};

struct CSectorListEvent
{
	std::uint64_t m_nRequestId{ 0 };
	SectorType m_type{ SectorType::unknown };
	std::vector<CSectorInfo> m_sectors;
	std::string m_strError;
};

struct CSectorConstituentsEvent
{
	std::uint64_t m_nRequestId{ 0 };
	CSectorInfo m_sector;
	std::vector<CSecurity> m_securities;
	std::string m_strError;
};

// 行情接收和队列处理的运行统计
struct CMarketRuntimeMetrics
{
	std::uint64_t m_nQuoteReceived{ 0 };		// 收到的行情总数
	std::uint64_t m_nQuoteAccepted{ 0 };		// 通过序列校验的行情数
	std::uint64_t m_nQuoteDuplicates{ 0 };		// 重复行情数
	std::uint64_t m_nQuoteOutOfOrder{ 0 };		// 乱序行情数
	std::uint64_t m_nQuoteSequenceGaps{ 0 };	// 缺失的行情序列数量
	std::uint64_t m_nQuoteWithoutSequence{ 0 }; // 无有效序列号的行情数
	std::uint64_t m_nQuoteQueueOverwrites{ 0 }; // 队列中同证券行情被覆盖的次数
	std::uint64_t m_nQuoteQueueDrops{ 0 };		// 队列满后丢弃的行情数
	std::size_t m_nPendingQuoteCount{ 0 };		// 当前等待处理的行情数
	std::size_t m_nPendingQuoteLimit{ 0 };		// 等待队列容量上限
};

// 行情接收和队列处理的线程安全运行统计
struct CAtomMarketRuntimeMetrics
{
	std::atomic_uint64_t m_nLastQuoteSequence{ 0 };	   // 最近接受的行情序列号
	std::atomic_uint64_t m_nQuoteReceived{ 0 };		   // 收到的行情总数
	std::atomic_uint64_t m_nQuoteAccepted{ 0 };		   // 通过序列校验的行情数
	std::atomic_uint64_t m_nQuoteDuplicates{ 0 };	   // 重复行情数
	std::atomic_uint64_t m_nQuoteOutOfOrder{ 0 };	   // 乱序行情数
	std::atomic_uint64_t m_nQuoteSequenceGaps{ 0 };	   // 缺失的行情序列数量
	std::atomic_uint64_t m_nQuoteWithoutSequence{ 0 }; // 无有效序列号的行情数
	std::atomic_uint64_t m_nQuoteQueueOverwrites{ 0 }; // 队列中同证券行情被覆盖的次数
	std::atomic_uint64_t m_nQuoteQueueDrops{ 0 };	   // 队列满后丢弃的行情数
	std::atomic_size_t m_nPendingQuoteLimit{ 10000 };  // 等待队列容量上限
};

#endif
