#pragma once

#include <cassert>
#include <iostream>

#include "TypeInfo.h"

#define PROPERTY(Name) \
	inline static struct RegistPropertyExecutor_##Name \
	{ \
		RegistPropertyExecutor_##Name() \
		{ \
			static PropertyRegister<ThisType, decltype(Name), decltype(&ThisType::Name), &ThisType::##Name> property_register_##Name{ #Name, ThisType::StaticTypeInfo() }; \
		} \
	} regist_##Name; \

// non template class에서 참조를 위한 베이스 클래스
class PropertyHandlerBase
{
	GENERATE_TYPE_INFO(PropertyHandlerBase)

public:
	virtual ~PropertyHandlerBase() = default;

	virtual void* GetRawPointer(void* object) const = 0;
};

// 구현을 강제하고 템플릿 매개변수로 형변환을 위한 템플릿 인터페이스 클래스
template <typename T>
class IPropertyHandler : public PropertyHandlerBase
{
	GENERATE_TYPE_INFO(IPropertyHandler)
		using ElementType = std::remove_all_extents_t<T>;

public:
	virtual ElementType& Get(void* object, size_t index = 0) const = 0;
	virtual void Set(void* object, const ElementType& value, size_t index = 0) const = 0;
};

template <typename TClass, typename T>
class PropertyHandler : public IPropertyHandler<T>
{
	GENERATE_TYPE_INFO(PropertyHandler)
		using MemberPtr = T TClass::*;
	using ElementType = std::remove_all_extents_t<T>;

public:
	explicit PropertyHandler(MemberPtr ptr)
		: mPtr(ptr)
	{
	}

	virtual ElementType& Get(void* object, size_t index = 0) const override {
		if constexpr (std::is_array_v<T>)
		{
			return (static_cast<TClass*>(object)->*mPtr)[index];
		}
		else
		{
			return static_cast<TClass*>(object)->*mPtr;
		}
	}

	virtual void Set(void* object, const ElementType& value, size_t index = 0) const override {
		if constexpr (std::is_array_v<T>)
		{
			set((static_cast<TClass*>(object)->*mPtr)[index], value);
		}
		else
		{
			set(static_cast<TClass*>(object)->*mPtr, value);
		}
	}

	virtual void* GetRawPointer(void* object) const override
	{
		return static_cast<void*>(&(static_cast<TClass*>(object)->*mPtr));
	}

private:
	template <typename T>
	void set(T& dest, const T& src) const {
		dest = src;
	}

	template <typename KeyType, typename ValueType, typename Pred, typename Alloc>
	void set(std::map<KeyType, ValueType, Pred, Alloc>& dest, const std::map<KeyType, ValueType, Pred, Alloc>& src) const
	{
		if constexpr (std::is_copy_assignable_v<KeyType> && std::is_copy_assignable_v<ValueType>)
		{
			dest = src;
		}
	}

private:
	MemberPtr mPtr = nullptr;
};

template <typename TClass, typename T>
class StaticPropertyHandler : public IPropertyHandler<T>
{
	GENERATE_TYPE_INFO(StaticPropertyHandler)
		using ElementType = std::remove_all_extents_t<T>;

public:
	explicit StaticPropertyHandler(T* ptr)
		: mPtr(ptr)
	{
	}

	virtual ElementType& Get([[maybe_unused]] void* object, size_t index = 0) const override
	{
		if constexpr (std::is_array_v<T>)
		{
			return (*mPtr)[index];
		}
		else
		{
			return *mPtr;
		}
	}

	virtual void Set([[maybe_unused]] void* object, const ElementType& value, size_t index = 0) const override
	{
		if constexpr (std::is_array_v<T>)
		{
			(*mPtr)[index] = value;
		}
		else
		{
			*mPtr = value;
		}
	}

	virtual void* GetRawPointer([[maybe_unused]] void* object) const override
	{
		return mPtr;
	}

private:
	T* mPtr = nullptr;
};

using PrintFuncPtr = void(*)(void*);

struct PropertyInitializer
{
	const char* mName = nullptr;
	const TypeInfo& mType;
	const PropertyHandlerBase& mHandler;
	PrintFuncPtr mPrintFunc = nullptr;
};

class Property
{
public:
	Property(TypeInfo& owner, const PropertyInitializer& initializer)
		: mName(initializer.mName)
		, mType(initializer.mType)
		, mHandler(initializer.mHandler)
		, mPrintFunc(initializer.mPrintFunc)
	{
		owner.addProperty(this);
	}

	template <typename T>
	const T& Get(void* object) const
	{
		if (!mHandler.GetTypeInfo().IsChildOf<IPropertyHandler<T>>())
		{
			assert(false && "Property::Get<T> - invalid casting");
			static T dummy{};
			return dummy;
		}

		auto concreteHandler = static_cast<const IPropertyHandler<T>*>(&mHandler);
		return concreteHandler->Get(object);
	}

	template <typename T>
	void Set(void* object, const T& value) const
	{
		if (!mHandler.GetTypeInfo().IsChildOf<IPropertyHandler<T>>())
		{
			assert(false && "Property::Set<T> - invalid casting");
		}

		auto concreteHandler = static_cast<const IPropertyHandler<T>*>(&mHandler);
		concreteHandler->Set(object, value);
	}

	template <typename T>
	const T& GetAt(void* object, size_t index) const
	{
		if (!mHandler.GetTypeInfo().IsChildOf<IPropertyHandler<T>>())
		{
			assert(false && "Property::GetAt<T> - invalid casting");
			static T dummy{};
			return dummy;
		}
		if (!mType.IsArray())
		{
			assert(false && "Property::GetAt<T> - indexing non-array property");
			static T dummy{};
			return dummy;
		}
		if (index >= mType.GetArrayExtent())
		{
			assert(false && "Property::GetAt<T> - index out of bounds");
			static T dummy{};
			return dummy;
		}

		auto concreteHandler = static_cast<const IPropertyHandler<T>*>(&mHandler);
		return concreteHandler->Get(object, index);
	}

	template <typename T>
	void SetAt(void* object, const T& value, size_t index) const
	{
		if (!mHandler.GetTypeInfo().IsChildOf<IPropertyHandler<T>>())
		{
			assert(false && "Property::SetAt<T> - invalid casting");
			return;
		}
		if (!mType.IsArray())
		{
			assert(false && "Property::SetAt<T> - indexing non-array property");
			return;
		}
		if (index >= mType.GetArrayExtent())
		{
			assert(false && "Property::SetAt<T> - index out of bounds");
			return;
		}

		auto concreteHandler = static_cast<const IPropertyHandler<T>*>(&mHandler);
		concreteHandler->Set(object, value, index);
	}

	inline void PrintPropertyValue(void* object, int indent) const;
	inline void PrintProperty(int indent) const;

	inline const char* GetName() const;
	inline const TypeInfo& GetTypeInfo() const;
	inline void* GetRawPointer(void* object) const;
	
private:
	using PrintFuncPtr = void(*)(void*);

	const char* mName = nullptr;
	const TypeInfo& mType;
	const PropertyHandlerBase& mHandler;
	PrintFuncPtr mPrintFunc = nullptr;
};

inline void Property::PrintPropertyValue(void* object, int indent) const
{
	std::string indentStr(indent * 4, ' ');
	std::cout
		<< indentStr
		<< "Type: " << mType.GetName()
		<< ", Name: " << mName;

	mPrintFunc(mHandler.GetRawPointer(object));
	std::cout << std::endl;
}

inline void Property::PrintProperty(int indent) const
{
	std::string indentStr(indent * 4, ' ');

	std::cout
		<< indentStr
		<< "Type: " << mType.GetName()
		<< ", Name: " << mName << "\n";
}

inline const char* Property::GetName() const
{
	return mName;
}

inline const TypeInfo& Property::GetTypeInfo() const
{
	return mType;
}

inline void* Property::GetRawPointer(void* object) const
{
	return mHandler.GetRawPointer(object);
}

// 인스턴스의 맴버값 출력 함수(컴파일 타임에 함수 포인터 캡처함)
template <typename T>
concept OstreamWritable = requires(std::ostream & os, T value) {
	{ os << value } -> std::same_as<std::ostream&>;
};

template <typename T>
void Print(void* object)
{
	if constexpr (OstreamWritable<T>)
	{
		T* value = static_cast<T*>(object);
		std::cout << ", Value : " << *value;
	}
	else
	{
		std::cout << ", Value : None ";
	}
}

template <typename TClass, typename T, typename TPtr, TPtr ptr>
class PropertyRegister
{
public:
	PropertyRegister(const char* name, TypeInfo& typeInfo)
	{
		using ElementType = std::remove_all_extents_t<T>;

		if constexpr (std::is_member_pointer_v<TPtr>)
		{
			static PropertyHandler<TClass, T> handler(ptr);
			static PropertyInitializer initializer = { .mName = name, .mType = TypeInfo::GetStaticTypeInfo<T>(), .mHandler = handler, .mPrintFunc = &Print<T> };
			static Property property(typeInfo, initializer);

		}
		else
		{
			static StaticPropertyHandler<TClass, T> handler(ptr);
			static PropertyInitializer initializer = { .mName = name, .mType = TypeInfo::GetStaticTypeInfo<T>(), .mHandler = handler, .mPrintFunc = &Print<T> };
			static Property property(typeInfo, initializer);
		}
	}
};
