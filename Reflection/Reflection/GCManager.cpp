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
		markFromRecursive(root);
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

	mLastDebugInfo = {
		totalMs,
		totalUs,
		objectCount,
		deletedCount,
		remaining,
		rootCount
	};

	std::ostringstream oss;
	oss << "[GC] Start - Mode: " << "Single-threaded" << "\n"
		<< "[GC] Total Time: " << totalMs << " ms (" << totalUs << " μs)\n"
		<< " └─ Mark Phase:  " << markMs << " ms (" << markUs << " μs)\n"
		<< " └─ Sweep Phase: " << sweepMs << " ms (" << sweepUs << " μs)\n"
		<< "[GC] Total objects: " << objectCount << "\n"
		<< "[GC] Root objects: " << rootCount << "\n"
		<< "[GC] Deleted objects: " << deletedCount << "\n"
		<< "[GC] Remaining objects: " << remaining << "\n";

	OutputDebugStringA(oss.str().c_str());
}

void GCManager::CollectMultiThread()
{
	using namespace std::chrono;

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

	const size_t threadCount = std::thread::hardware_concurrency();
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

	const size_t markCunkSize = (rootObjects.size() + threadCount - 1) / threadCount;

	for (size_t t = 0; t < threadCount; ++t)
	{
		size_t begin = t * markCunkSize;
		size_t end = std::min<size_t>(rootObjects.size(), (t + 1) * markCunkSize);

		markFutures.emplace_back(std::async(std::launch::async, [this, &rootObjects, begin, end]() {
			for (size_t i = begin; i < end; ++i)
			{
				markFromRecursive(rootObjects[i]);
			}
			}));
	}

	for (auto& f : markFutures)
	{
		f.get();
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
		deletedCount += validRange.DeleteCount;
		const size_t copyLength = validRange.EndIndex - validRange.StartIndex - validRange.DeleteCount;
		mGCObjects.MoveChunkByMemcpy(destIndex, validRange.StartIndex, copyLength);
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

	mLastDebugInfo = {
		totalMs,
		totalUs,
		objectCount,
		deletedCount,
		remaining,
		rootCount
	};

	std::ostringstream oss;
	oss << "[GC] Start - Mode: " << "Multi-threaded : " << threadCount << "\n"
		<< "[GC] Total Time: " << totalMs << " ms (" << totalUs << " μs)\n"
		<< " └─ Mark Phase:  " << markMs << " ms (" << markUs << " μs)\n"
		<< " └─ Sweep Phase: " << sweepMs << " ms (" << sweepUs << " μs)\n"
		<< "[GC] Total objects: " << objectCount << "\n"
		<< "[GC] Root objects: " << rootCount << "\n"
		<< "[GC] Deleted objects: " << deletedCount << "\n"
		<< "[GC] Remaining objects: " << remaining << "\n";

	OutputDebugStringA(oss.str().c_str());
}

void GCManager::markFrom(GCObject* root)
{
	std::vector<GCObject*>& stack = mTempCacheObject;
	stack.push_back(root);

	while (!stack.empty())
	{
		GCObject* current = stack.back();
		stack.pop_back();

		if (current->isMarked())
		{
			continue;
		}

		current->setMarked(true);

		const TypeInfo& typeInfo = current->GetTypeInfo();

		for (const Property* prop : typeInfo.GetProperties())
		{
			void* ptr = prop->GetRawPointer(current);

			if (prop->GetTypeInfo().IsChildOf<GCObject*>())
			{
				GCObject* child = *static_cast<GCObject**>(ptr);

				if (child)
				{
					stack.push_back(child);
				}
			}
			else if (prop->GetTypeInfo().IsIterable())
			{
				auto iter = prop->CreateIteratorBegin<GCObject*>(ptr);
				auto end = prop->CreateIteratorEnd<GCObject*>(ptr);

				while (*iter != *end)
				{
					GCObject* child = *static_cast<GCObject**>(iter->Dereference());

					if (child)
					{
						stack.push_back(child);
					}

					iter->Increment();
				}
			}
		}
	}
	stack.clear();
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
