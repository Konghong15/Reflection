#pragma once

#include "Property.h"

class GCObject
{
	GENERATE_CLASS_TYPE_INFO(GCObject)

		friend class GCManager;

public:
	GCObject()
		: mMarked(false)
	{
	}
	virtual ~GCObject() = default;

private:
	void mark();
	void markRecursive(void * object, const Property* property);

	void sertMarked(bool marked)
	{
		mMarked = marked;
	}

	bool isMarked() const
	{
		return mMarked;
	}

private:
	bool mMarked;
};