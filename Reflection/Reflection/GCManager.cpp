#include "GCManager.h"
#include "GCObject.h"

void GCManager::Collect()
{
	ClearMark();
	Mark();
	Sweep();
}

void GCManager::ClearMark()
{
	mIndexPoolArray.ForEach([](int, GCObject* object) 
		{
			object->sertMarked(false);
		});
}

void GCManager::Mark()
{
	for (GCObject* root : mRoots)
	{
		root->mark();
	}
}

void GCManager::Sweep()
{
	mIndexPoolArray.ForEach([this](int index, GCObject* obj) 
		{
			if (!obj->isMarked())
			{
				delete obj;
				mIndexPoolArray.Remove(index);
			}
		});
}