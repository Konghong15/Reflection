#pragma once

#include "Property.h"

class GCObject
{
	GENERATE_TYPE_INFO(GCObject)

public:
	GCObject() = default;
	virtual ~GCObject() = default;
	GCObject(const GCObject&) = default;
	GCObject& operator=(const GCObject&) = default;

private:
};