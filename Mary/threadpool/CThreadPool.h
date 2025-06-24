#ifndef __CThreadPool_H__
#define __CThreadPool_H__

#include <iostream>
#include <queue>
#include <thread>
#include <functional>
#include <future>
#include <mutex>
#include "defines.h"
#include "./common/ISingleton.h"


class CThreadPool final : public ISingleton<CThreadPool>
{
	DECLARE_SINGLE_DFAULT_DELETE(CThreadPool)

private:
	explicit CThreadPool(std::size_t threads);//只有核心线程
	explicit CThreadPool(pool_type ty);//指定线程池类型
	explicit CThreadPool(std::size_t threads, std::size_t assistants);//任务量过多有辅助线程

public:
	void ShutDown();
	void PrintTask();

	template<class _Fx, class... _Args>
	auto PushTask(task_priority level, std::size_t priority, _Fx&& f, _Args&&... args) -> 
		std::future<typename std::invoke_result_t<_Fx, _Args...>>
	{
		using return_type = typename std::invoke_result_t<_Fx, _Args...>;
		auto task = std::make_shared<std::packaged_task<return_type()>>(
			[f = std::forward<_Fx>(f), args = std::make_tuple(std::forward<_Args>(args)...)]() mutable {
				return std::apply(std::move(f), std::move(args));
			}
		);
		std::future<return_type> ret = task->get_future();
		RegisterTask(level, priority, std::move(task));
		return ret;
	}



// 	template<class _Fx>
// 	auto PushTask(task_priority level, std::size_t priority, _Fx&& f) ->
// 		std::future<typename std::invoke_result_t<_Fx>>
// 	{
// 		using return_type = typename std::invoke_result_t<_Fx>;
// 		auto task = std::make_shared< std::packaged_task<return_type()> >(
// 			[f = std::forward<_Fx>(f)]() mutable {
// 				return f();
// 			}
// 		);
// 		std::future<return_type> ret = task->get_future();
// 		RegisterTask(level, priority, std::move(task));
// 		return ret;
// 	}

private:
 	template<typename _Task>
	bool RegisterTask(task_priority level, size_t priority, _Task&& task)
	{
		if (m_stop.load())
		{
			return false;
		}
		{
			std::unique_lock<std::mutex> lck(m_task_mutex);
			m_task_queue.emplace(level, priority, std::forward<_Task>(task));
			++m_nTask;
		}

		m_task_cond.notify_one();
		if ((m_nAssistants > 0) && ShouldNewAssistantThread())//增加辅助线程
		{
			BuildNewThread(1);
		}
		return true;
	}

private:
	void ThreadProc();
	bool ShouldNewAssistantThread();
	void BuildNewThread(std::size_t threads);
	void SetWorkersCount(std::size_t nCores, std::size_t nAssistants);

private:
	std::condition_variable	m_task_cond;
	std::vector<std::thread>	m_thread_workers;
	std::atomic_size_t		m_nWorkers{ 0 };
	std::mutex	m_task_mutex;
	std::priority_queue<Task, std::vector<Task>, TaskComparator>	m_task_queue;
	std::atomic_size_t		m_nTask{ 0 };


private:
	std::atomic_bool		m_stop{ false };		//线程退出
	std::atomic_size_t		m_busy_workers{ 0 };	//忙碌线程数
	std::atomic_size_t		m_nCores{ 0 };			//核心线程数
	std::atomic_size_t		m_nAssistants{ 0 };		//辅助线程数
	std::atomic_size_t		m_nMaxWorkers{ 0 };		//最大线程数
};

#endif