#pragma once

template <typename T, typename... Args>
T* NewGCObject(class GCManager& gcManager, Args&&... args)
{
	static_assert(std::is_base_of_v<GCObject, T>, "NewGCObject requires T to be derived from GCObject");

	T* object = new T(std::forward<Args>(args)...);
	gcManager.AddObject(object);

	return object;
}

