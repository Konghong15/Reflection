#pragma once

#include <unordered_set>

#include "FixedVector.h"

class GCObject;

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

	void AddObject(GCObject* object)
	{
		mGCObjects.Add(object);
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

	enum { POOL_SIZE = 1024 * 10 };
	FixedVector<GCObject*, POOL_SIZE> mGCObjects;
};