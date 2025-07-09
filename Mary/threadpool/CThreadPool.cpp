#include "CThreadPool.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#else
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

std::size_t GetSystemCPUCount()
{
	std::size_t s_cores = std::thread::hardware_concurrency();
	if (s_cores <= 0)
	{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		s_cores = systemInfo.dwNumberOfProcessors;
#else
		s_cores = get_nprocs_conf();
#endif
	}
	return s_cores;
}

std::size_t CalcBestPerformanceThreads(pool_type mode)
{
	//CPU密集型:CPU数 + 1
	//IO密集型:CPU数 * 2
	//CPU数 * (1 + 线程等待时间/线程运行时间)
	std::size_t nCpus = GetSystemCPUCount();

	if (mode == pool_type::em_more_calc)
	{
		return (nCpus + 1);
	}
	else if (mode == pool_type::em_more_io)
	{
		return (2 * nCpus);
	}
	else if (mode == pool_type::em_more_complex)
	{
		return int(1.5 * nCpus + 1);
	}
	return nCpus;
}


CThreadPool::CThreadPool(std::size_t threads)
{
	SetWorkersCount(threads, 0);
	BuildNewThread(m_nCores);
}

CThreadPool::CThreadPool(std::size_t threads, std::size_t assistants)
{
	SetWorkersCount(threads, assistants);
	BuildNewThread(m_nCores);
}

CThreadPool::CThreadPool(pool_type ty)
{
	SetWorkersCount(CalcBestPerformanceThreads(ty), (m_nCores - 1) / 2);
	BuildNewThread(m_nCores);
}

CThreadPool::~CThreadPool()
{
	ShutDown();
}

void CThreadPool::SetWorkersCount(std::size_t nCores, std::size_t nAssistants)
{
	m_nCores = nCores;
	m_nAssistants = nAssistants;
	m_nMaxWorkers = m_nCores + m_nAssistants;
}

void CThreadPool::ShutDown()
{
	{
		std::unique_lock<std::mutex> lck(m_task_mutex);
		if (m_stop)
		{
			return;
		}
		m_stop = true;
	}
	m_cond.notify_all();
}

void CThreadPool::ThreadProc()
{
	while (true)
	{
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lck(m_task_mutex);
			m_cond.wait(lck, [this] {
				return m_stop || !m_task_queue.empty(); 
				});

			if (m_stop && m_task_queue.empty())
			{
				return;
			}

			if (!m_task_queue.empty())
			{
				++m_busy_workers;
				task = std::move(const_cast<Task&>(m_task_queue.top()).m_task);
				m_task_queue.pop();
				--m_nTasks;
			}
		}
		if (nullptr != task)
		{
			try
			{
				task();
			}
			catch (const std::exception&)
			{

			}
			--m_busy_workers;
		}
	}
}

void CThreadPool::PrintTask()
{
	std::unique_lock<std::mutex> lck(m_task_mutex);
	while (!m_task_queue.empty())
	{
		std::cout << int(m_task_queue.top().m_level) << "  |   " << m_task_queue.top().m_priority << std::endl;
		m_task_queue.pop();
		--m_nTasks;
	}
}

bool CThreadPool::ShouldNewAssistantThread()
{
	if (m_nAssistants <= 0)
	{
		return false;
	}
	if (m_busy_workers < m_nCores)//忙碌线程未到最大
	{
		return false;
	}
	if (m_nWorkers >= m_nMaxWorkers)
	{
		return false;
	}
	std::size_t nTasks = m_nTasks.load();
	std::size_t nWorkers = m_nWorkers.load();
	return  (nTasks > (nWorkers * 2.5));
}

void CThreadPool::BuildNewThread(std::size_t threads)
{
	std::unique_lock<std::mutex> lck(m_worker_mutex);
	if (m_nWorkers >= m_nMaxWorkers)
	{
		return;
	}

	for (std::size_t i = 0; i < threads; ++i)
	{
		m_workers.emplace_back(&CThreadPool::ThreadProc, this);
		if (m_workers.size() >= m_nMaxWorkers)
		{
			break;
		}
	}
	std::size_t sz = m_workers.size();
	m_nWorkers.store(sz);
}