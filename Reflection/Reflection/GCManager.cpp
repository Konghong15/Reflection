#include <vector>
#include <chrono>
#include <windows.h>    
#include <string>       
#include <sstream>      
#include <thread>
#include <future>
#include <atomic>

#include "GCManager.h"
#include "GCObject.h"

GCManager* GCManager::mInstance = nullptr;

GCManager::~GCManager()
{
	const size_t OBJECT_COUNT = mGCObjects.GetSize();

	for (int i = static_cast<int>(OBJECT_COUNT) - 1; i >= 0; --i)
	{
		delete mGCObjects[i];
		mGCObjects[i] = nullptr;
		mGCObjects.RemoveLast();
	}
}

void GCManager::Collect()
{
	using namespace std::chrono;

	auto startTime = high_resolution_clock::now(); // 전체 시간 측정 시작

	const size_t OBJECT_COUNT = mGCObjects.GetSize();
	size_t deletedCount = 0;
	size_t rootCount = 0;

	std::vector<GCObject*> rootObjects;
	rootObjects.reserve(128);

	auto markStart = high_resolution_clock::now();

	//-------------------- MARK --------------------
	for (size_t i = 0; i < OBJECT_COUNT; ++i)
	{
		mGCObjects[i]->sertMarked(false);

		if (mGCObjects[i]->IsRoot())
		{
			rootObjects.push_back(mGCObjects[i]);
			++rootCount;
		}
	}
	for (GCObject* root : rootObjects)
	{
		root->mark();
	}

	auto markEnd = high_resolution_clock::now();
	auto markMs = duration_cast<milliseconds>(markEnd - markStart).count();
	auto markUs = duration_cast<microseconds>(markEnd - markStart).count();

	auto sweepStart = high_resolution_clock::now();

	//-------------------- SWEEP --------------------
	for (int i = static_cast<int>(OBJECT_COUNT) - 1; i >= 0; --i)
	{
		if (mGCObjects[i]->IsRoot())
		{
			continue;
		}
		if (mGCObjects[i]->isMarked())
		{
			continue;
		}

		delete mGCObjects[i];
		mGCObjects[i] = nullptr;
		mGCObjects.RemoveAtSwapLast(i);
		++deletedCount;
	}

	auto sweepEnd = high_resolution_clock::now();
	auto sweepMs = duration_cast<milliseconds>(sweepEnd - sweepStart).count();
	auto sweepUs = duration_cast<microseconds>(sweepEnd - sweepStart).count();

	//-------------------- 디버그 정보 관리 --------------------
	auto endTime = high_resolution_clock::now();
	auto totalMs = duration_cast<milliseconds>(endTime - startTime).count();
	auto totalUs = duration_cast<microseconds>(endTime - startTime).count();

	const size_t remaining = mGCObjects.GetSize();

	// 저장
	mLastDebugInfo = {
		totalMs,
		totalUs,
		OBJECT_COUNT,
		deletedCount,
		remaining,
		rootCount
	};

	// 출력
	std::ostringstream oss;
	oss << "[GC] Start - Mode: " << "Single-threaded" << "\n"
		<< "[GC] Total Time: " << totalMs << " ms (" << totalUs << " μs)\n"
		<< " └─ Mark Phase:  " << markMs << " ms (" << markUs << " μs)\n"
		<< " └─ Sweep Phase: " << sweepMs << " ms (" << sweepUs << " μs)\n"
		<< "[GC] Total objects: " << OBJECT_COUNT << "\n"
		<< "[GC] Root objects: " << rootCount << "\n"
		<< "[GC] Deleted objects: " << deletedCount << "\n"
		<< "[GC] Remaining objects: " << remaining << "\n";

	OutputDebugStringA(oss.str().c_str());
}


void GCManager::CollectMultiThread()
{
	using namespace std::chrono;

	auto startTime = high_resolution_clock::now(); // 전체 시간 측정 시작

	const size_t OBJECT_COUNT = mGCObjects.GetSize();
	size_t rootCount = 0;

	std::vector<GCObject*> rootObjects;
	rootObjects.reserve(128);

	//-------------------- MARK --------------------
	auto markStart = high_resolution_clock::now();

	for (size_t i = 0; i < OBJECT_COUNT; ++i)
	{
		mGCObjects[i]->sertMarked(false);

		if (mGCObjects[i]->IsRoot())
		{
			rootObjects.push_back(mGCObjects[i]);
			++rootCount;
		}
	}
	for (GCObject* root : rootObjects)
	{
		root->mark();
	}

	auto markEnd = high_resolution_clock::now();
	auto markMs = duration_cast<milliseconds>(markEnd - markStart).count();
	auto markUs = duration_cast<microseconds>(markEnd - markStart).count();

	//-------------------- SWEEP --------------------
	auto sweepStart = high_resolution_clock::now();
	const size_t THREAD_COUNT = std::thread::hardware_concurrency()/2; // 보통 4~16
	const size_t CHUNK_SIZE = OBJECT_COUNT / THREAD_COUNT;

	std::vector<std::future<size_t>> futures;

	for (size_t t = 0; t < THREAD_COUNT; ++t)
	{
		size_t begin = t * CHUNK_SIZE;
		size_t end = (t == THREAD_COUNT - 1) ? OBJECT_COUNT : (t + 1) * CHUNK_SIZE;

		futures.push_back(mThreadPool.Enqueue([this, begin, end]() -> size_t {
			size_t localDeleted = 0;

			for (size_t i = begin; i < end; ++i)
			{
				GCObject* obj = mGCObjects[i];
				if (!obj || obj->IsRoot() || obj->isMarked()) continue;

				delete obj;
				mGCObjects[i] = nullptr;
				++localDeleted;
			}
			return localDeleted;
			}));
	}

	size_t deletedCount = 0;
	for (auto& f : futures)
	{
		deletedCount += f.get(); // 각 작업 결과를 합산
	}

	// Remove nullptrs (single-threaded)
	for (int i = static_cast<int>(mGCObjects.GetSize()) - 1; i >= 0; --i)
	{
		if (mGCObjects[i] == nullptr)
		{
			mGCObjects.RemoveAtSwapLast(i);
		}
	}

	auto sweepEnd = high_resolution_clock::now();
	auto sweepMs = duration_cast<milliseconds>(sweepEnd - sweepStart).count();
	auto sweepUs = duration_cast<microseconds>(sweepEnd - sweepStart).count();

	//-------------------- 디버그 정보 관리 --------------------
	auto endTime = high_resolution_clock::now();
	auto totalMs = duration_cast<milliseconds>(endTime - startTime).count();
	auto totalUs = duration_cast<microseconds>(endTime - startTime).count();

	const size_t remaining = mGCObjects.GetSize();

	// 저장
	mLastDebugInfo = {
		totalMs,
		totalUs,
		OBJECT_COUNT,
		deletedCount,
		remaining,
		rootCount
	};

	// 출력
	std::ostringstream oss;
	oss << "[GC] Start - Mode: " << "Multi-threaded" << "\n"
		<< "[GC] Total Time: " << totalMs << " ms (" << totalUs << " μs)\n"
		<< " └─ Mark Phase:  " << markMs << " ms (" << markUs << " μs)\n"
		<< " └─ Sweep Phase: " << sweepMs << " ms (" << sweepUs << " μs)\n"
		<< "[GC] Total objects: " << OBJECT_COUNT << "\n"
		<< "[GC] Root objects: " << rootCount << "\n"
		<< "[GC] Deleted objects: " << deletedCount << "\n"
		<< "[GC] Remaining objects: " << remaining << "\n";

	OutputDebugStringA(oss.str().c_str());
}
