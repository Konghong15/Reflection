#pragma once

#include <unordered_set>

#include "IndexPoolArray.h"

class GCObject;

class GCManager
{
public:
	static GCManager& Get()
	{
		static GCManager instance;
		return instance;
	}

	void Collect();
	void ClearMark();
	void Mark();
	void Sweep();

	void AddRoot(GCObject* object)
	{
		mRoots.insert(object);
	}
	void RemoveRoot(GCObject* object)
	{
		mRoots.erase(object);
	}

	void AddObject(GCObject* object)
	{
		mIndexPoolArray.Add(object);
	}

private:
	GCManager() = default;
	GCManager(const GCManager&) = delete;
	GCManager& operator=(const GCManager&) = delete;

private:
	enum { POOL_SIZE = 1024 * 10 };
	IndexPoolArray<GCObject*, POOL_SIZE> mIndexPoolArray;
	std::unordered_set<GCObject*> mRoots;
};