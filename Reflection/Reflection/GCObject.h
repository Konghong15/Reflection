#pragma once

#include "Property.h"

class GCObject
{
	GENERATE_TYPE_INFO(GCObject)

		friend class GCManager;

public:
	GCObject()
		: mbMarked(false)
		, mbRoot(false)
	{
	}
	virtual ~GCObject() = default;

	void SetRoot(bool root)
	{
		mbRoot = root;
	}

	bool IsRoot() const
	{
		return mbRoot;
	}

private:
	void mark();
	void markRecursive(void * object, const Property* property);

	void sertMarked(bool marked)
	{
		mbMarked = marked;
	}

	bool isMarked() const
	{
		return mbMarked;
	}

private:
	bool mbMarked;
	bool mbRoot;
};