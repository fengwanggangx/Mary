#include "CWindSession.h"
#include "../configuration/CServerSettings.h"

#include "../network/CTcpClient.h"
#include "../network/common_net.h"
#include "../request/request.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QSettings>

#include <algorithm>
#include <utility>
#include <vector>

CWindSession::CWindSession(QObject* pParent) : QObject(pParent)
{
	QSettings settings(QCoreApplication::applicationDirPath() + "/mary.ini", QSettings::IniFormat);
	const std::optional<configuration::CHostInfo> site = configuration::CHostMgr::InstanceRef().GetActiveHost();
	if (site.has_value())
	{
		m_host = QString::fromStdString(site->m_strHost);
		m_port = static_cast<int>(site->m_nPort);
	}
	m_user = settings.value("wind/user").toString();
	m_password = settings.value("wind/password").toString();
	m_heartbeatSeconds = std::max(1, settings.value("wind/heartbeat_seconds", 15).toInt());
	m_timeoutSeconds = std::max(1, settings.value("wind/request_timeout_seconds", 10).toInt());
	m_maxReconnectSeconds = std::max(1, settings.value("wind/max_reconnect_seconds", 30).toInt());
}

CWindSession::~CWindSession()
{
	Stop();
}

void CWindSession::Start()
{
	if (m_connectionThread.joinable())
	{
		return;
	}
	m_stopping.store(false);
	m_connectionThread = std::thread(&CWindSession::ConnectionLoop, this);
	m_maintenanceThread = std::thread(&CWindSession::MaintenanceLoop, this);
}

void CWindSession::Stop()
{
	if (!m_connectionThread.joinable() && !m_maintenanceThread.joinable())
	{
		return;
	}
	m_stopping.store(true);
	m_waitCondition.notify_all();
	{
		std::lock_guard<std::mutex> lock(m_mtx_client);
		if (nullptr != m_client)
		{
			m_client->ShutDown();
		}
	}
	if (m_maintenanceThread.joinable())
	{
		m_maintenanceThread.join();
	}
	if (m_connectionThread.joinable())
	{
		m_connectionThread.join();
	}
	FailPending("客户端已关闭");
}

bool CWindSession::IsAuthenticated() const
{
	return m_authenticated.load();
}

std::uint64_t CWindSession::SendCommand(const QString& strCommand, const QMap<QString, QString>& parameters)
{
	if (!m_connected.load())
	{
		return 0;
	}
	CRequest request;
	request.SetType(CRequest::Type::HQMARKET);
	request.SetCmd(strCommand.toStdString());
	for (auto iter = parameters.constBegin(); parameters.constEnd() != iter; ++iter)
	{
		request.SetExtraData(iter.key().toStdString(), iter.value().toStdString());
	}
	std::uint64_t id = request.GetId();
	{
		std::lock_guard<std::mutex> lock(m_mtx_pending);
		m_pending.emplace(id, PendingRequest{ strCommand, std::chrono::steady_clock::now() + std::chrono::seconds(m_timeoutSeconds) });
	}
	bool sent = false;
	{
		std::lock_guard<std::mutex> lock(m_mtx_client);
		sent = (nullptr != m_client) && m_client->SendRequest(request);
	}
	if (!sent)
	{
		std::lock_guard<std::mutex> lock(m_mtx_pending);
		m_pending.erase(id);
		return 0;
	}
	return id;
}

void CWindSession::ConnectionLoop()
{
	int reconnectSeconds = 1;
	while (!m_stopping.load())
	{
		PostState(1 == reconnectSeconds ? "正在连接" : QString("%1 秒后重连").arg(reconnectSeconds));
		std::unique_ptr<net::CTcpClient> client = std::make_unique<net::CTcpClient>(m_host.toStdString(), m_port);
		client->RegisterHandler([this](const net::CNetEvent& event)
		{
			OnNetworkEvent(event);
			return 1;
		});
		{
			std::lock_guard<std::mutex> lock(m_mtx_client);
			m_client = std::move(client);
		}
		int result = 0;
		{
			std::lock_guard<std::mutex> lock(m_mtx_client);
			result = m_client->Initialize();
		}
		if (0 == result)
		{
			m_client->Start(true);
		}
		else
		{
			PostError(QString("连接初始化失败：%1").arg(result));
		}
		m_connected.store(false);
		m_authenticated.store(false);
		FailPending("连接已断开");
		{
			std::lock_guard<std::mutex> lock(m_mtx_client);
			m_client.reset();
		}
		if (m_stopping.load())
		{
			break;
		}
		std::unique_lock<std::mutex> lock(m_mtx_wait);
		m_waitCondition.wait_for(lock, std::chrono::seconds(reconnectSeconds), [this]()
		{
			return m_stopping.load();
		});
		reconnectSeconds = (std::min)(m_maxReconnectSeconds, reconnectSeconds * 2);
	}
	PostState("已关闭");
}

void CWindSession::MaintenanceLoop()
{
	std::chrono::steady_clock::time_point nextHeartbeat = std::chrono::steady_clock::now();
	while (!m_stopping.load())
	{
		std::unique_lock<std::mutex> waitLock(m_mtx_wait);
		m_waitCondition.wait_for(waitLock, std::chrono::milliseconds(250), [this]()
		{
			return m_stopping.load();
		});
		waitLock.unlock();
		if (m_stopping.load())
		{
			break;
		}
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (m_authenticated.load() && (nextHeartbeat <= now))
		{
			SendCommand("heartbeat");
			nextHeartbeat = now + std::chrono::seconds(m_heartbeatSeconds);
		}
		std::vector<std::pair<std::uint64_t, QString>> expired;
		{
			std::lock_guard<std::mutex> lock(m_mtx_pending);
			for (auto iter = m_pending.begin(); m_pending.end() != iter;)
			{
				if (iter->second.deadline <= now)
				{
					expired.emplace_back(iter->first, iter->second.command);
					iter = m_pending.erase(iter);
				}
				else
				{
					++iter;
				}
			}
		}
		for (const auto& item : expired)
		{
			QMetaObject::invokeMethod(this, [this, item]()
			{
				emit RequestFailed(item.first, item.second, "请求超时");
			}, Qt::QueuedConnection);
		}
	}
}

void CWindSession::OnNetworkEvent(const net::CNetEvent& event)
{
	if (net::em_event::connected == event.m_event)
	{
		m_connected.store(true);
		PostState("已连接，正在认证");
		SendAuthentication();
		return;
	}
	if (net::em_event::request == event.m_event)
	{
		if (nullptr == event.m_request)
		{
			return;
		}
		std::uint64_t id = event.m_request->GetId();
		QString command = QString::fromStdString(event.m_request->GetCmd());
		{
			std::lock_guard<std::mutex> lock(m_mtx_pending);
			m_pending.erase(id);
		}
		QMap<QString, QString> result;
		for (const auto& item : event.m_request->GetReturnData())
		{
			result.insert(QString::fromStdString(item.first), QString::fromStdString(item.second));
		}
		QString error = result.value("error_message");
		if (("auth" == command) && error.isEmpty())
		{
			m_authenticated.store(true);
			PostState("已认证");
			QMetaObject::invokeMethod(this, [this]() { emit AuthenticationChanged(true); }, Qt::QueuedConnection);
		}
		QMetaObject::invokeMethod(this, [this, id, command, result, error]()
		{
			if (error.isEmpty())
			{
				emit ResponseReceived(id, command, result);
			}
			else
			{
				emit RequestFailed(id, command, error);
			}
		}, Qt::QueuedConnection);
		return;
	}
	m_connected.store(false);
	m_authenticated.store(false);
	PostState("连接已断开");
	PostError(0 == event.m_error ? "Wind 主动关闭连接" : QString("网络错误：%1").arg(event.m_error));
	QMetaObject::invokeMethod(this, [this]() { emit AuthenticationChanged(false); }, Qt::QueuedConnection);
}

void CWindSession::SendAuthentication()
{
	QMap<QString, QString> parameters;
	parameters.insert("user", m_user);
	parameters.insert("password", m_password);
	if (0 == SendCommand("auth", parameters))
	{
		PostError("认证请求发送失败");
	}
}

void CWindSession::FailPending(const QString& strReason)
{
	std::vector<std::pair<std::uint64_t, QString>> failed;
	{
		std::lock_guard<std::mutex> lock(m_mtx_pending);
		for (const auto& item : m_pending)
		{
			failed.emplace_back(item.first, item.second.command);
		}
		m_pending.clear();
	}
	for (const auto& item : failed)
	{
		QMetaObject::invokeMethod(this, [this, item, strReason]() { emit RequestFailed(item.first, item.second, strReason); }, Qt::QueuedConnection);
	}
}

void CWindSession::PostState(const QString& strState)
{
	QMetaObject::invokeMethod(this, [this, strState]() { emit ConnectionStateChanged(strState); }, Qt::QueuedConnection);
}

void CWindSession::PostError(const QString& strError)
{
	QMetaObject::invokeMethod(this, [this, strError]() { emit RecentErrorChanged(strError); }, Qt::QueuedConnection);
}
