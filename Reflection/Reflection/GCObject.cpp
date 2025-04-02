#include <iostream>

#include "GCObject.h"
#include "GCManager.h"
#include "GCUtility.h"

void GCObject::mark()
{
	if (isMarked())
	{
		return;
	}

	sertMarked(true);

	for (const Property* property : GetTypeInfo().GetProperties())
	{
		void* propPtr = property->GetRawPointer(this);
		markRecursive(propPtr, property);
	}
}

void GCObject::markRecursive(void* object, const Property* property)
{
	// 1. 付欧 贸府
	const TypeInfo& typeInfo = property->GetTypeInfo();

	if (typeInfo.IsPointer() && typeInfo.IsChildOf<GCObject>())
	{
		static_cast<GCObject*>(object)->sertMarked(true);
	}
	if (typeInfo.IsArray() && typeInfo.GetElementType()->IsChildOf<GCObject>())
	{
		size_t END = typeInfo.GetArrayExtent();
		const TypeInfo& elementType = *typeInfo.GetElementType();
		GCObject** gcObjectArray = static_cast<GCObject**>(object);

		for (size_t i = 0; i < END; ++i)
		{
			GCObject* gcObject = gcObjectArray[i];

			if (gcObject != nullptr)
			{
				gcObject->sertMarked(true);
			}
		}
	}

	// 2. 犁蓖 贸府
	for (const Property* property : typeInfo.GetProperties())
	{
		void* propPtr = property->GetRawPointer(object);
		markRecursive(propPtr, property);
	}
}