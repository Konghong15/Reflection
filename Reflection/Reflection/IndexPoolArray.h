#pragma once

#include <array>
#include <queue>
#include <cstddef>
#include <cassert>

template <typename T, size_t MaxSize>
class IndexPoolArray
{
public:
	IndexPoolArray()
	{
		for (int i = 0; i < MaxSize; ++i)
		{
			mFreeIndices.push(i);
			mObjects[i] = nullptr;
		}
	}

	int Add(T obj)
	{
		if (mFreeIndices.empty())
			return -1;

		int index = mFreeIndices.front();
		mFreeIndices.pop();
		mObjects[index] = obj;
		return index;
	}

	void Remove(int index)
	{
		assert(IsValidIndex(index) && "Invalid index to remove");
		mObjects[index] = nullptr;
		mFreeIndices.push(index);
	}

	T Get(int index) const
	{
		if (!IsValidIndex(index)) return nullptr;
		return mObjects[index];
	}

	bool IsValidIndex(int index) const
	{
		return index >= 0 && index < MaxSize && mObjects[index] != nullptr;
	}

	template <typename Func>
	void ForEach(Func&& func)
	{
		for (int i = 0; i < MaxSize; ++i)
		{
			if (mObjects[i] != nullptr)
			{
				func(i, mObjects[i]);
			}
		}
	}

	bool IsFull() const 
	{
		return mFreeIndices.empty(); 
	}
	bool IsEmpty() const
	{ 
		return mFreeIndices.size() == MaxSize; 
	}
	size_t GetFreeCount() const
	{
		return mFreeIndices.size();
	}

private:
	std::array<T, MaxSize> mObjects;
	std::queue<int> mFreeIndices;
};