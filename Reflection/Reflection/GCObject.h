#pragma once

#include "Property.h"

class GCObject
{
	GENERATE_TYPE_INFO(GCObject)

		friend class GCManager;

public:
	GCObject();
	virtual ~GCObject() = default;
	GCObject(const GCObject&) = default;
	GCObject& operator=(const GCObject&) = default;

	inline void SetRoot(bool root);
	inline bool IsRoot() const;

private:
	void mark();
	void markRecursive(void * object, const Property* property);

	inline void sertMarked(bool marked);
	inline bool isMarked() const;

private:
	bool mbMarked;
	bool mbRoot;
};

inline void GCObject::SetRoot(bool root)
{
	mbRoot = root;
}

inline bool GCObject::IsRoot() const
{
	return mbRoot;
}

inline void GCObject::sertMarked(bool marked)
{
	mbMarked = marked;
}

inline bool GCObject::isMarked() const
{
	return mbMarked;
}
