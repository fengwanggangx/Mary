#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>

	using CallbackId = std::uint64_t;

	template <typename Event>
	class CallbackRegistry final
	{
	public:
		using Callback = std::function<void(const Event&)>;

		CallbackId Subscribe(Callback callback)
		{
			std::lock_guard<std::mutex> lock(m_mtx_callbacks);
			CallbackId id = ++m_nextId;
			m_callbacks.emplace(id, std::move(callback));
			return id;
		}

		void Unsubscribe(CallbackId id)
		{
			std::lock_guard<std::mutex> lock(m_mtx_callbacks);
			m_callbacks.erase(id);
		}

		void Notify(const Event& event)
		{
			std::unordered_map<CallbackId, Callback> callbacks;
			{
				std::lock_guard<std::mutex> lock(m_mtx_callbacks);
				callbacks = m_callbacks;
			}
			for (const auto& item : callbacks)
			{
				if (item.second)
				{
					item.second(event);
				}
			}
		}

	private:
		std::mutex m_mtx_callbacks;
		CallbackId m_nextId{0};
		std::unordered_map<CallbackId, Callback> m_callbacks;
	};
