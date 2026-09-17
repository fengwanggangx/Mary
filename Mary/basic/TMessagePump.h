#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

using _TyCallbackId = std::uint64_t;

template <typename _TyParam>
class TMessagePump final
{
public:
	using _TyCallback = std::function<void(const _TyParam&)>;

	_TyCallbackId Subscribe(_TyCallback&& callback)
	{
		std::lock_guard<std::shared_mutex> lock(m_mtx_callbacks);
		_TyCallbackId id = ++m_nextId;
		m_callbacks.emplace(id, std::move(callback));
		return id;
	}

	void Unsubscribe(_TyCallbackId id)
	{
		std::lock_guard<std::shared_mutex> lock(m_mtx_callbacks);
		m_callbacks.erase(id);
	}

	void Notify(const _TyParam& value) const
	{
		std::vector<_TyCallback> callbacks;
		{
			std::shared_lock<std::shared_mutex> lock(m_mtx_callbacks);
			callbacks.reserve(m_callbacks.size());
			for (const auto& item : m_callbacks)
			{
				callbacks.emplace_back(item.second);
			}
		}
		for (const _TyCallback& callback : callbacks)
		{
			if (callback)
			{
				callback(value);
			}
		}
	}

private:
	mutable std::shared_mutex m_mtx_callbacks;
	_TyCallbackId m_nextId{ 0 };
	std::unordered_map<_TyCallbackId, _TyCallback> m_callbacks;
};
