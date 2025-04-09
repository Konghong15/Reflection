#pragma once

#include <atomic>

#include "Property.h"

class GCManager;

class GCObject
{
	GENERATE_TYPE_INFO(GCObject)

	friend class GCManager;

public:
	GCObject() = default;
	virtual ~GCObject() = default;
	GCObject(const GCObject&) = default;
	GCObject& operator=(const GCObject&) = default;

	bool IsRoot() const { return mbRoot; }
	void SetRoot(bool v) { mbRoot = v; }

private:
	bool atomicMark()
	{
		bool expected = false;
		return mbMark.compare_exchange_strong(expected, true);
	}

	bool isMarked() const { return mbMark.load(std::memory_order_relaxed); }
	void setMarked(bool v) { mbMark.store(v, std::memory_order_relaxed); }

private:
	std::atomic<bool> mbMark = false;
	bool mbRoot = false;
};