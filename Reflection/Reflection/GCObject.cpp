#include <iostream>

#include "GCObject.h"
#include "GCManager.h"
#include "GCUtility.h"

GCObject::GCObject()
	: mbMarked(false)
	, mbRoot(false)
{
}

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
	if (typeInfo.IsIterable() && typeInfo.GetIteratorElementType()->IsChildOf<GCObject>())
	{
		std::unique_ptr<IteratorWrapperBase> beginIter = property->CreateIteratorBegin<GCObject*>(object);
		std::unique_ptr<IteratorWrapperBase> endIter = property->CreateIteratorEnd<GCObject*>(object);

		while (beginIter && endIter && *beginIter != *endIter)
		{
			GCObject* gcObject = *static_cast<GCObject**>(beginIter->Dereference());

			if (gcObject != nullptr)
			{
				gcObject->sertMarked(true);
			}

			beginIter->Increment();
		}
	}

	// 2. 犁蓖 贸府
	for (const Property* property : typeInfo.GetProperties())
	{
		void* propPtr = property->GetRawPointer(object);
		markRecursive(propPtr, property);
	}
}