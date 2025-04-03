#pragma once
#include <cassert>
#include <iterator>

#include "TypeInfo.h"
#include "Property.h"
#include "Method.h"

template <typename T, size_t Capacity>
class FixedVector
{
	GENERATE_TYPE_INFO(FixedVector)

public:
	using ValueType = T;
	using SizeType = size_t;

	FixedVector()
		: mElementCount(0)
		, mElements{}
	{
	}

	T& operator[](size_t Index)
	{
		assert(Index < mElementCount);
		return mElements[Index];
	}

	const T& operator[](size_t Index) const
	{
		assert(Index < mElementCount);
		return mElements[Index];
	}

	void Add(const T& Value)
	{
		assert(mElementCount < Capacity && "FixedVector capacity exceeded");
		mElements[mElementCount++] = Value;
	}

	void RemoveLast()
	{
		assert(mElementCount > 0 && "FixedVector is empty");
		--mElementCount;
	}

	void RemoveAtSwapLast(size_t Index)
	{
		assert(Index < mElementCount && "Index out of range");

		if (Index != mElementCount - 1)
		{
			std::swap(mElements[Index], mElements[mElementCount - 1]);
		}

		--mElementCount;
	}

	T& First() { return mElements[0]; }
	const T& First() const { return mElements[0]; }

	T& Last() { return mElements[mElementCount - 1]; }
	const T& Last() const { return mElements[mElementCount - 1]; }

	size_t GetSize() const { return mElementCount; }
	constexpr size_t GetCapacity() const { return Capacity; }
	bool IsEmpty() const { return mElementCount == 0; }

private:
	PROPERTY(mElements)
		T mElements[Capacity];

	PROPERTY(mElementCount)
		size_t mElementCount;
};