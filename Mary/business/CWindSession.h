#ifndef MARY_BUSINESS_CWINDSESSION_H
#define MARY_BUSINESS_CWINDSESSION_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace net
{
	class CTcpClient;
	struct CNetEvent;
}

class CWindSession final : public QObject
{
	Q_OBJECT

public:
	explicit CWindSession(QObject* pParent = nullptr);
	~CWindSession() override;
	void Start();
	void Stop();
	std::uint64_t SendCommand(const QString& strCommand, const QMap<QString, QString>& parameters = {});
	bool IsAuthenticated() const;

signals:
	void ConnectionStateChanged(const QString& strState);
	void AuthenticationChanged(bool authenticated);
	void ResponseReceived(qulonglong id, const QString& strCommand, const QMap<QString, QString>& result);
	void RequestFailed(qulonglong id, const QString& strCommand, const QString& strError);
	void RecentErrorChanged(const QString& strError);

private:
	struct PendingRequest
	{
		QString command;
		std::chrono::steady_clock::time_point deadline;
	};

	void ConnectionLoop();
	void MaintenanceLoop();
	void OnNetworkEvent(const net::CNetEvent& event);
	void SendAuthentication();
	void FailPending(const QString& strReason);
	void PostState(const QString& strState);
	void PostError(const QString& strError);

	std::atomic_bool m_stopping{ false };
	std::atomic_bool m_connected{ false };
	std::atomic_bool m_authenticated{ false };
	std::thread m_connectionThread;
	std::thread m_maintenanceThread;
	mutable std::mutex m_mtx_client;
	std::unique_ptr<net::CTcpClient> m_client;
	std::mutex m_mtx_pending;
	std::unordered_map<std::uint64_t, PendingRequest> m_pending;
	std::mutex m_mtx_wait;
	std::condition_variable m_waitCondition;
	QString m_host;
	int m_port{ 0 };
	QString m_user;
	QString m_password;
	int m_heartbeatSeconds{ 15 };
	int m_timeoutSeconds{ 10 };
	int m_maxReconnectSeconds{ 30 };
};

#endif
