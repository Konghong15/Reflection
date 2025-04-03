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
	using value_type = T;  // STL 호환을 위한 타입 별칭
	using SizeType = size_t;
	using size_type = size_t;  // STL 호환을 위한 타입 별칭
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using difference_type = std::ptrdiff_t;

	// 반복자 클래스 정의
	class iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		iterator(pointer ptr) : m_ptr(ptr) {}

		// 역참조 연산자
		reference operator*() const { return *m_ptr; }
		pointer operator->() const { return m_ptr; }

		// 증가/감소 연산자
		iterator& operator++() { ++m_ptr; return *this; }
		iterator operator++(int) { iterator tmp = *this; ++m_ptr; return tmp; }
		iterator& operator--() { --m_ptr; return *this; }
		iterator operator--(int) { iterator tmp = *this; --m_ptr; return tmp; }

		// 비교 연산자
		bool operator==(const iterator& other) const { return m_ptr == other.m_ptr; }
		bool operator!=(const iterator& other) const { return m_ptr != other.m_ptr; }

		// 랜덤 액세스 연산자
		iterator& operator+=(difference_type n) { m_ptr += n; return *this; }
		iterator operator+(difference_type n) const { return iterator(m_ptr + n); }
		friend iterator operator+(difference_type n, const iterator& it) { return iterator(it.m_ptr + n); }

		iterator& operator-=(difference_type n) { m_ptr -= n; return *this; }
		iterator operator-(difference_type n) const { return iterator(m_ptr - n); }
		difference_type operator-(const iterator& other) const { return m_ptr - other.m_ptr; }

		// 비교 연산자 (순서)
		bool operator<(const iterator& other) const { return m_ptr < other.m_ptr; }
		bool operator>(const iterator& other) const { return m_ptr > other.m_ptr; }
		bool operator<=(const iterator& other) const { return m_ptr <= other.m_ptr; }
		bool operator>=(const iterator& other) const { return m_ptr >= other.m_ptr; }

		// 첨자 연산자
		reference operator[](difference_type n) const { return m_ptr[n]; }

	private:
		pointer m_ptr;
	};

	// 상수 반복자 클래스 정의
	class const_iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = const T*;
		using reference = const T&;

		const_iterator(pointer ptr) : m_ptr(ptr) {}
		const_iterator(const iterator& it) : m_ptr(&(*it)) {}

		// 역참조 연산자
		reference operator*() const { return *m_ptr; }
		pointer operator->() const { return m_ptr; }

		// 증가/감소 연산자
		const_iterator& operator++() { ++m_ptr; return *this; }
		const_iterator operator++(int) { const_iterator tmp = *this; ++m_ptr; return tmp; }
		const_iterator& operator--() { --m_ptr; return *this; }
		const_iterator operator--(int) { const_iterator tmp = *this; --m_ptr; return tmp; }

		// 비교 연산자
		bool operator==(const const_iterator& other) const { return m_ptr == other.m_ptr; }
		bool operator!=(const const_iterator& other) const { return m_ptr != other.m_ptr; }

		// 랜덤 액세스 연산자
		const_iterator& operator+=(difference_type n) { m_ptr += n; return *this; }
		const_iterator operator+(difference_type n) const { return const_iterator(m_ptr + n); }
		friend const_iterator operator+(difference_type n, const const_iterator& it) { return const_iterator(it.m_ptr + n); }

		const_iterator& operator-=(difference_type n) { m_ptr -= n; return *this; }
		const_iterator operator-(difference_type n) const { return const_iterator(m_ptr - n); }
		difference_type operator-(const const_iterator& other) const { return m_ptr - other.m_ptr; }

		// 비교 연산자 (순서)
		bool operator<(const const_iterator& other) const { return m_ptr < other.m_ptr; }
		bool operator>(const const_iterator& other) const { return m_ptr > other.m_ptr; }
		bool operator<=(const const_iterator& other) const { return m_ptr <= other.m_ptr; }
		bool operator>=(const const_iterator& other) const { return m_ptr >= other.m_ptr; }

		// 첨자 연산자
		reference operator[](difference_type n) const { return m_ptr[n]; }

	private:
		pointer m_ptr;
	};

	// 역방향 반복자 타입 정의
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	FixedVector()
		: mElementCount(0)
		, mElements{}
	{
	}

	// 반복자 메소드
	iterator begin() { return iterator(mElements); }
	iterator end() { return iterator(mElements + mElementCount); }
	const_iterator begin() const { return const_iterator(mElements); }
	const_iterator end() const { return const_iterator(mElements + mElementCount); }
	const_iterator cbegin() const { return const_iterator(mElements); }
	const_iterator cend() const { return const_iterator(mElements + mElementCount); }

	// 역방향 반복자 메소드
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
	const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

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