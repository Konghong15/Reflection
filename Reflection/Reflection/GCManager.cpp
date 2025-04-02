#include <vector>
#include <chrono>
#include <windows.h>    
#include <string>       
#include <sstream>      

#include "GCManager.h"
#include "GCObject.h"

GCManager* GCManager::mInstance = nullptr;

void GCManager::Collect()
{
	using namespace std::chrono;

	auto startTime = high_resolution_clock::now(); // 시간 측정 시작

	const size_t OBJECT_COUNT = mGCObjects.GetSize();
	size_t deletedCount = 0;

	// 1. mark
	std::vector<GCObject*> rootObjects;
	rootObjects.reserve(128);

	for (size_t i = 0; i < OBJECT_COUNT; ++i)
	{
		mGCObjects[i]->sertMarked(false);

		if (mGCObjects[i]->IsRoot())
		{
			rootObjects.push_back(mGCObjects[i]);
		}
	}
	for (GCObject* root : rootObjects)
	{
		root->mark();
	}

	// 2. sweep
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

	auto endTime = high_resolution_clock::now();
	auto durationMs = duration_cast<milliseconds>(endTime - startTime).count();
	auto durationUs = duration_cast<microseconds>(endTime - startTime).count();

	const size_t remaining = mGCObjects.GetSize();

	std::ostringstream oss;
	oss << "[GC] Time: " << durationMs << " ms (" << durationUs << " μs)\n"
		<< "[GC] Total objects: " << OBJECT_COUNT << "\n"
		<< "[GC] Deleted objects: " << deletedCount << "\n"
		<< "[GC] Remaining objects: " << remaining << "\n";

	OutputDebugStringA(oss.str().c_str());
}
