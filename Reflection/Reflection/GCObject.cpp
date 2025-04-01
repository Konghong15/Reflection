#include "GCObject.h"
#include "GCManager.h"
#include "GCUtility.h"

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
		
		if (propertyTypeInfo.IsArray() && propertyTypeInfo.GetElementType()->IsChildOf<GCObject>())
		{
			for (size_t i = 0; i < propertyTypeInfo.GetArrayExtent(); ++i)
			{
				GCObject* neighbor = property->Get<GCObject*>(this, i);

				if (neighbor != nullptr)
				{
					neighbor->mark();
				}
			}
		}

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
