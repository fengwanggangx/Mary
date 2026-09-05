#pragma once

#include <cstdint>
#include <functional>
#include <shared_mutex>
#include <unordered_map>

	using _TyCallbackId = std::uint64_t;

	template <typename _TyParam>
	class CallbackRegistry final
	{
	public:
		using _TyCallback = std::function<void(const _TyParam&)>;

		_TyCallbackId Subscribe(_TyCallback callback)
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

		void Notify(const _TyParam& v) const
		{
			std::unordered_map<_TyCallbackId, _TyCallback> callbacks;
			{
				std::shared_lock<std::shared_mutex> lock(m_mtx_callbacks);
				callbacks = m_callbacks;
			}
			for (const auto& item : callbacks)
			{
				if (item.second)
				{
					item.second(v);
				}
			}
		}

	private:
		mutable std::shared_mutex m_mtx_callbacks;
		_TyCallbackId m_nextId{0};
		std::unordered_map<_TyCallbackId, _TyCallback> m_callbacks;
	};
