#pragma once

template <typename T, typename... Args>
T* NewGCObject(Args&&... args)
{
	static_assert(std::is_base_of_v<GCObject, T>, "NewGCObject requires T to be derived from GCObject");

	T* obj = new T(std::forward<Args>(args)...);
	GCManager::Get().AddObject(obj);
	return obj;
}

struct GCMarkUtils
{
	template <typename T>
	static void MarkValue(T& value)
	{
		if constexpr (std::is_base_of_v<GCObject, T>)
		{
			value.mark();
		}
		else if constexpr (std::is_pointer_v<T> && std::is_base_of_v<GCObject, std::remove_pointer_t<T>>)
		{
			if (value)
			{
				value->mark();
			}
		}
		else if constexpr (is_vector_of_gcobject_v<T>)
		{
			for (auto* item : value)
			{
				if (item)
				{
					item->mark();
				}
			}
		}
	}
};
