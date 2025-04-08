#pragma once

#include <array>
#include <atomic>
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

	bool IsMarked(GCObject* obj) const { return isMarked(getIndex(obj)); }
	void SetMarked(GCObject* obj) { setMarked(getIndex(obj)); }
	void ClearMarked(GCObject* obj) { clearMarked(getIndex(obj)); }

	bool IsRoot(GCObject* obj) const { return isRoot(getIndex(obj)); }
	void SetRoot(GCObject* obj, bool isRootFlag)
	{
		size_t index = getIndex(obj);
		if (isRootFlag)
			setRoot(index);
		else
			clearRoot(index);
	}

private:
	GCManager() = default;
	~GCManager();
	GCManager(const GCManager&) = delete;
	GCManager& operator=(const GCManager&) = delete;

	void markFrom(GCObject* root);

	size_t getIndex(GCObject* obj) const
	{
		for (size_t i = 0; i < mGCObjects.GetSize(); ++i)
		{
			if (mGCObjects[i] == obj)
				return i;
		}
		assert(false && "Invalid object pointer");
		return 0;
	}

	inline bool isMarked(size_t index) const
	{
		return (mMarkBitmap[index / 8].load() >> (index % 8)) & 1;
	}

	inline void setMarked(size_t index)
	{
		mMarkBitmap[index / 8].fetch_or(1 << (index % 8));
	}

	inline void clearMarked(size_t index)
	{
		mMarkBitmap[index / 8].fetch_and(~(1 << (index % 8)));
	}

	inline bool isRoot(size_t index) const
	{
		return (mRootBitmap[index / 8].load() >> (index % 8)) & 1;
	}

	inline void setRoot(size_t index)
	{
		mRootBitmap[index / 8].fetch_or(1 << (index % 8));
	}

	inline void clearRoot(size_t index)
	{
		mRootBitmap[index / 8].fetch_and(~(1 << (index % 8)));
	}

private:
	static GCManager* mInstance;

	enum { POOL_SIZE = 1024 * 128 };
	static constexpr size_t BITMAP_SIZE = (POOL_SIZE + 7) / 8;

	FixedVector<GCObject*, POOL_SIZE> mGCObjects;
	std::array<std::atomic<uint8_t>, BITMAP_SIZE> mMarkBitmap;
	std::array<std::atomic<uint8_t>, BITMAP_SIZE> mRootBitmap;
	GCDebugInfo mLastDebugInfo;
};

inline void GCManager::Create()
{
	mInstance = new GCManager();
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
