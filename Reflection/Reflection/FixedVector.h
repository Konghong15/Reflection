#pragma once
#include <cassert>
#include <iterator>

#include "TypeInfo.h"
#include "Property.h"
#include "Method.h"

template <typename T, size_t N>
class FixedVector
{
	GENERATE_CLASS_TYPE_INFO(FixedVector)

public:
	using value_type = T;
	using size_type = size_t;

	FixedVector()
		: mSize(0)
		, mData{}
	{
	}

	T& operator[](size_t index)
	{
		assert(index < mSize);
		return mData[index];
	}

	const T& operator[](size_t index) const
	{
		assert(index < mSize);
		return mData[index];
	}

	METHOD(push_back)
		void push_back(const T& value)
	{
		assert(mSize < N && "FixedVector capacity exceeded");
		mData[mSize++] = value;
	}

	void pop_back()
	{
		assert(mSize > 0 && "FixedVector is empty");
		--mSize;
	}

	T& front() { return mData[0]; }
	const T& front() const { return mData[0]; }

	T& back() { return mData[mSize - 1]; }
	const T& back() const { return mData[mSize - 1]; }

	size_t size() const { return mSize; }
	constexpr size_t capacity() const { return N; }
	bool empty() const { return mSize == 0; }

private:
	PROPERTY(mData)
		T mData[N];

	PROPERTY(mSize)
		size_t mSize;
};
