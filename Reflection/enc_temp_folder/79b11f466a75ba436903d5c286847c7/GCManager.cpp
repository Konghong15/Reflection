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

	//-------------------- MARK --------------------
	auto markStart = high_resolution_clock::now();

	std::vector<GCObject*> rootObjects;
	rootObjects.reserve(128);

	for (size_t i = 0; i < objectCount; ++i)
	{
		clearMarked(i);

		if (isRoot(i))
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
		if (isRoot(i))
		{
			continue;
		}
		if (isMarked(i))
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
}


void GCManager::markFrom(GCObject* root)
{
	std::stack<GCObject*> stack;
	stack.push(root);

	while (!stack.empty())
	{
		GCObject* current = stack.top();
		stack.pop();

		size_t index = getIndex(current);
		
		if (isMarked(index))
		{
			continue;
		}

		setMarked(index);

		const TypeInfo& typeInfo = current->GetTypeInfo();

		for (const Property* prop : typeInfo.GetProperties())
		{
			void* ptr = prop->GetRawPointer(current);

			if (prop->GetTypeInfo().IsChildOf<GCObject*>())
			{
				GCObject* child = *static_cast<GCObject**>(ptr);
				
				if (child)
				{
					stack.push(child);
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
						stack.push(child);
					}

					iter->Increment();
				}
			}
		}
	}
}
