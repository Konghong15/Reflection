#include "GCObject.h"
#include "GCManager.h"

void GCObject::mark()
{
	const TypeInfo& typeInfo = GetTypeInfo();

	for (const Property* property : typeInfo.GetProperties())
	{
		if (mMarked)
		{
			return;
		}

		mMarked = true;

		const TypeInfo& propertyTypeInfo = property->GetTypeInfo();

		if (!propertyTypeInfo.IsPointer())
		{
			continue;
		}
		if (!propertyTypeInfo.IsChildOf<GCObject>())
		{
			continue;
		}

		GCObject* neighbor = property->Get<GCObject*>(this);

		if (neighbor != nullptr)
		{
			neighbor->mark();
		}
	}
}
