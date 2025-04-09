#include <set>
#include <algorithm>
#include <vector>
#include <chrono>
#include <windows.h>    
#include <string>       
#include <sstream>      
#include <thread>
#include <future>
#include <atomic>
#include <stack>

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
	mMaxDepth.store(0, std::memory_order_relaxed);

	using namespace std::chrono;

	auto startTime = high_resolution_clock::now(); // 전체 시간 측정 시작

	const size_t objectCount = mGCObjects.GetSize();
	size_t deletedCount = 0;
	size_t rootCount = 0;

	std::vector<GCObject*> rootObjects;
	rootObjects.reserve(128);

	//-------------------- MARK --------------------
	auto markStart = high_resolution_clock::now();

	for (size_t i = 0; i < objectCount; ++i)
	{
		mGCObjects[i]->setMarked(false);

		if (mGCObjects[i]->IsRoot())
		{
			rootObjects.push_back(mGCObjects[i]);
			++rootCount;
		}
	}

	for (GCObject* root : rootObjects)
	{
		markFrom(root);
	}

	auto markEnd = high_resolution_clock::now();
	auto markMs = duration_cast<milliseconds>(markEnd - markStart).count();
	auto markUs = duration_cast<microseconds>(markEnd - markStart).count();

	//-------------------- SWEEP --------------------
	auto sweepStart = high_resolution_clock::now();

	for (int i = static_cast<int>(objectCount) - 1; i >= 0; --i)
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

	//-------------------- 디버그 정보 --------------------
	auto endTime = high_resolution_clock::now();
	auto totalMs = duration_cast<milliseconds>(endTime - startTime).count();
	auto totalUs = duration_cast<microseconds>(endTime - startTime).count();

	const size_t remaining = mGCObjects.GetSize();

	mLastDebugInfo.DurationMs = totalMs;
	mLastDebugInfo.DurationUs = totalUs;
	mLastDebugInfo.TotalObjects = objectCount;
	mLastDebugInfo.DeletedObjects = deletedCount;
	mLastDebugInfo.RemainingObjects = remaining;
	mLastDebugInfo.RootObjectCount = rootCount;
	size_t maxDepth = mMaxDepth.load(std::memory_order_relaxed);

	std::ostringstream oss;
	oss << "[GC] Start - Mode: " << "Single-threaded\n"
		<< "[GC] Total Time: " << totalMs << " ms (" << totalUs << " μs)\n"
		<< " └─ Mark Phase:  " << markMs << " ms (" << markUs << " μs)\n"
		<< " └─ Sweep Phase: " << sweepMs << " ms (" << sweepUs << " μs)\n"
		<< "[GC] Total objects: " << objectCount << "\n"
		<< "[GC] Root objects: " << rootCount << "\n"
		<< "[GC] Deleted objects: " << deletedCount << "\n"
		<< "[GC] Remaining objects: " << remaining << "\n"
		<< "[GC] Max Depth: " << maxDepth << "\n";

	OutputDebugStringA(oss.str().c_str());
}

void GCManager::CollectMultiThread()
{
	using namespace std::chrono;

	mMaxDepth.store(0, std::memory_order_relaxed);

	struct ValidRange
	{
		size_t StartIndex;
		size_t EndIndex;
		size_t DeleteCount;
	};

	auto startTime = high_resolution_clock::now(); // 전체 시간 측정 시작

	const size_t objectCount = mGCObjects.GetSize();
	size_t deletedCount = 0;
	size_t rootCount = 0;

	const size_t threadCount = std::thread::hardware_concurrency() / 2;
	std::vector<std::future<void>> markFutures;
	std::vector<std::future<ValidRange>> sweepFutures;
	markFutures.reserve(threadCount);
	sweepFutures.reserve(threadCount);

	std::vector<GCObject*> rootObjects;
	rootObjects.reserve(128);
	//-------------------- MARK --------------------
	auto markStart = high_resolution_clock::now();

	for (size_t i = 0; i < objectCount; ++i)
	{
		mGCObjects[i]->setMarked(false);

		if (mGCObjects[i]->IsRoot())
		{
			rootObjects.push_back(mGCObjects[i]);
			++rootCount;
		}
	}

	const size_t markChunkSize = (rootObjects.size() + threadCount - 1) / threadCount;

	for (size_t t = 0; t < threadCount; ++t)
	{
		size_t begin = t * markChunkSize;

		if (begin >= rootObjects.size())
		{
			break;
		}

		size_t end = std::min<size_t>(rootObjects.size(), (t + 1) * markChunkSize);

		markFutures.emplace_back(std::async(std::launch::async, [this, &rootObjects, begin, end]() {
			for (size_t i = begin; i < end; ++i)
			{
				markFrom(rootObjects[i]);
			}
			}));
	}

	for (auto& markFuture : markFutures)
	{
		markFuture.get();
	};

	auto markEnd = high_resolution_clock::now();
	auto markMs = duration_cast<milliseconds>(markEnd - markStart).count();
	auto markUs = duration_cast<microseconds>(markEnd - markStart).count();

	//-------------------- SWEEP --------------------
	auto sweepStart = high_resolution_clock::now();

	const size_t sweepChunkSize = (objectCount + threadCount - 1) / threadCount;

	for (size_t t = 0; t < threadCount; ++t)
	{
		size_t begin = t * sweepChunkSize;

		if (begin >= objectCount)
		{
			break;
		}

		size_t end = std::min<size_t>(objectCount, (t + 1) * sweepChunkSize);

		sweepFutures.emplace_back(std::async(std::launch::async, [this, begin, end]() -> ValidRange {
			ValidRange validRange;
			validRange.StartIndex = begin;
			validRange.EndIndex = end;
			validRange.DeleteCount = 0;

			for (int i = static_cast<int>(end) - 1; i >= static_cast<int>(begin); --i)
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
				mGCObjects.Swap(i, end - 1);
				++validRange.DeleteCount;
			}

			return validRange;
			}));
	}

	size_t destIndex = 0;

	for (auto& sweepFuture : sweepFutures)
	{
		ValidRange validRange = sweepFuture.get();
		assert(sweepChunkSize >= validRange.DeleteCount);

		const size_t copyLength = validRange.EndIndex - validRange.StartIndex - validRange.DeleteCount;
		mGCObjects.MoveChunkByMemcpy(destIndex, validRange.StartIndex, copyLength);

		deletedCount += validRange.DeleteCount;
		destIndex += copyLength;
	};

	mGCObjects.ShrinkTo(mGCObjects.GetSize() - deletedCount);

	auto sweepEnd = high_resolution_clock::now();
	auto sweepMs = duration_cast<milliseconds>(sweepEnd - sweepStart).count();
	auto sweepUs = duration_cast<microseconds>(sweepEnd - sweepStart).count();

	//-------------------- 디버그 정보 --------------------
	auto endTime = high_resolution_clock::now();
	auto totalMs = duration_cast<milliseconds>(endTime - startTime).count();
	auto totalUs = duration_cast<microseconds>(endTime - startTime).count();

	const size_t remaining = mGCObjects.GetSize();

	mLastDebugInfo.DurationMs = totalMs;
	mLastDebugInfo.DurationUs = totalUs;
	mLastDebugInfo.TotalObjects = objectCount;
	mLastDebugInfo.DeletedObjects = deletedCount;
	mLastDebugInfo.RemainingObjects = remaining;
	mLastDebugInfo.RootObjectCount = rootCount;
	size_t maxDepth = mMaxDepth.load(std::memory_order_relaxed);

	std::ostringstream oss;
	oss << "[GC] Start - Mode: " << "Multi-threaded : " << threadCount << "\n"
		<< "[GC] Total Time: " << totalMs << " ms (" << totalUs << " μs)\n"
		<< " └─ Mark Phase:  " << markMs << " ms (" << markUs << " μs)\n"
		<< " └─ Sweep Phase: " << sweepMs << " ms (" << sweepUs << " μs)\n"
		<< "[GC] Total objects: " << objectCount << "\n"
		<< "[GC] Root objects: " << rootCount << "\n"
		<< "[GC] Deleted objects: " << deletedCount << "\n"
		<< "[GC] Remaining objects: " << remaining << "\n"
		<< "[GC] Max Depth: " << maxDepth << "\n";

	OutputDebugStringA(oss.str().c_str());
}

void GCManager::markFrom(GCObject* root)
{
	std::vector<std::pair<GCObject*, size_t>> stack;
	stack.reserve(POOL_SIZE);

	stack.push_back({ root, 1 });
	root->setMarked(true);

	size_t maxDepth = 1;

	while (!stack.empty())
	{
		auto [current, depth] = stack.back();
		stack.pop_back();

		maxDepth = std::max<size_t>(maxDepth, depth);
		const TypeInfo& typeInfo = current->GetTypeInfo();

		for (const Property* prop : typeInfo.GetProperties())
		{
			void* ptr = prop->GetRawPointer(current);

			if (prop->GetTypeInfo().IsChildOf<GCObject*>())
			{
				GCObject* child = *static_cast<GCObject**>(ptr);

				if (child != nullptr && child->atomicMark())
				{
					stack.push_back({ child, depth + 1 });
				}
			}
			else if (prop->GetTypeInfo().IsIterable())
			{
				auto iter = prop->CreateIteratorBegin<GCObject*>(ptr);
				auto end = prop->CreateIteratorEnd<GCObject*>(ptr);

				while (*iter != *end)
				{
					GCObject* child = *static_cast<GCObject**>(iter->Dereference());

					if (child != nullptr && child->atomicMark())
					{
						stack.push_back({ child, depth + 1 });
					}

					iter->Increment();
				}
			}
		}
	}

	size_t current = mMaxDepth.load(std::memory_order_relaxed);
	while (current < maxDepth && !mMaxDepth.compare_exchange_weak(current, maxDepth, std::memory_order_relaxed)) {}
}

void GCManager::markFromRecursive(GCObject* object)
{
	if (!object || object->isMarked())
	{
		return;
	}

	object->setMarked(true);

	const TypeInfo& typeInfo = object->GetTypeInfo();

	for (const Property* prop : typeInfo.GetProperties())
	{
		void* ptr = prop->GetRawPointer(object);

		if (prop->GetTypeInfo().IsChildOf<GCObject*>())
		{
			GCObject* child = *static_cast<GCObject**>(ptr);
			markFromRecursive(child);
		}
		else if (prop->GetTypeInfo().IsIterable())
		{
			auto iter = prop->CreateIteratorBegin<GCObject*>(ptr);
			auto end = prop->CreateIteratorEnd<GCObject*>(ptr);

			while (*iter != *end)
			{
				GCObject* child = *static_cast<GCObject**>(iter->Dereference());
				markFromRecursive(child);
				iter->Increment();
			}
		}
	}
}
