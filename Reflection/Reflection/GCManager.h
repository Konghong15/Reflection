#pragma once

#include <unordered_set>

#include "FixedVector.h"

class GCObject;

struct GCDebugInfo
{
	int64_t DurationMs = 0;
	int64_t DurationUs = 0;
	size_t TotalObjects = 0;
	size_t DeletedObjects = 0;
	size_t RemainingObjects = 0;
	size_t RootObjectCount = 0;
};

class GCManager
{
public:
	static void Create()
	{
		mInstance = new GCManager();
	}

	static GCManager& Get()
	{
		assert(mInstance != nullptr);
		return *mInstance;
	}

	static void Destroy()
	{
		delete mInstance;
		mInstance = nullptr;
	}

public:
	void Collect();
	void CollectMultiThread();

	void AddObject(GCObject* object)
	{
		mGCObjects.Add(object);
	}

	const GCDebugInfo& GetLastDebugInfo() const
	{
		return mLastDebugInfo;
	}

private:
	GCManager() = default;
	~GCManager()
	{
		const size_t OBJECT_COUNT = mGCObjects.GetSize();

		for (int i = static_cast<int>(OBJECT_COUNT) - 1; i >= 0; --i)
		{
			delete mGCObjects[i];
			mGCObjects[i] = nullptr;
			mGCObjects.RemoveLast();
		}
	}
	GCManager(const GCManager&) = delete;
	GCManager& operator=(const GCManager&) = delete;

private:
	static GCManager* mInstance;

	enum { POOL_SIZE = 1024 * 128 };
	FixedVector<GCObject*, POOL_SIZE> mGCObjects;
	GCDebugInfo mLastDebugInfo;
};