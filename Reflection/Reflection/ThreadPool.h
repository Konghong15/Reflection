#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool
{
public:
	ThreadPool(size_t threadCount = std::thread::hardware_concurrency())
		: mStop(false)
	{
		for (size_t i = 0; i < threadCount; ++i)
		{
			mWorkers.emplace_back([this]() {
				while (true)
				{
					std::function<void()> task;

					{
						std::unique_lock<std::mutex> lock(mQueueMutex);
						mCondition.wait(lock, [this] {
							return mStop || !mTasks.empty();
							});

						if (mStop && mTasks.empty())
							return;

						task = std::move(mTasks.front());
						mTasks.pop();
					}

					task();
				}
				});
		}
	}

	~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(mQueueMutex);
			mStop = true;
		}
		mCondition.notify_all();
		for (auto& thread : mWorkers)
			thread.join();
	}

	template <typename Func, typename... Args>
	auto Enqueue(Func&& func, Args&&... args)
		-> std::future<std::invoke_result_t<Func, Args...>>
	{
		using ReturnType = std::invoke_result_t<Func, Args...>;

		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
		);

		std::future<ReturnType> result = task->get_future();

		{
			std::unique_lock<std::mutex> lock(mQueueMutex);
			mTasks.emplace([task]() { (*task)(); });
		}

		mCondition.notify_one();
		return result;
	}

private:
	std::vector<std::thread> mWorkers;
	std::queue<std::function<void()>> mTasks;
	std::mutex mQueueMutex;
	std::condition_variable mCondition;
	std::atomic<bool> mStop;
};