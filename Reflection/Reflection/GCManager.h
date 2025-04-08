#pragma once

#include <array>
#include <cassert>
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

class GCManager final
{
public:
	static void Create();
	static GCManager& Get();
	static void Destroy();

	void Collect();
	void CollectMultiThread();

	void AddObject(GCObject* object);
	const GCDebugInfo& GetLastDebugInfo() const;

private:
	GCManager() = default;
	~GCManager();
	GCManager(const GCManager&) = delete;
	GCManager& operator=(const GCManager&) = delete;

	void markFrom(GCObject* root);
	void markFromRecursive(GCObject* root);

private:
	static GCManager* mInstance;

	enum { POOL_SIZE = 1024 * 128 };

	FixedVector<GCObject*, POOL_SIZE> mGCObjects;
	GCDebugInfo mLastDebugInfo;

	std::vector<GCObject*> mTempCacheObject;
};

inline void GCManager::Create()
{
	mInstance = new GCManager();
	mInstance->mTempCacheObject.reserve(POOL_SIZE);
}

inline GCManager& GCManager::Get()
{
	assert(mInstance != nullptr);
	return *mInstance;
}

inline void GCManager::Destroy()
{
	delete mInstance;
	mInstance = nullptr;
}

inline void GCManager::AddObject(GCObject* object)
{
	mGCObjects.Add(object);
}

inline const GCDebugInfo& GCManager::GetLastDebugInfo() const
{
	return mLastDebugInfo;
}