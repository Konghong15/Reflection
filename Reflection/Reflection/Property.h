#pragma once

#include <cassert>
#include <string>
#include <iostream>

#include "TypeInfo.h"

// -- ��ũ��
#define PROPERTY(Name) \
	inline static struct RegistPropertyExecutor_##Name \
	{ \
		RegistPropertyExecutor_##Name() \
		{ \
			static PropertyRegister<ThisType, decltype(Name), decltype(&ThisType::Name), &ThisType::##Name> property_register_##Name{ #Name, ThisType::StaticTypeInfo() }; \
		} \
	} regist_##Name; \


	// �������� ������ ���� ���̽� �ڵ鷯
class PropertyHandlerBase
{
	GENERATE_CLASS_TYPE_INFO(PropertyHandlerBase)

public:
	virtual ~PropertyHandlerBase() = default;

	virtual void* GetRawPointer(void* object) const = 0;
};

// Get/Set ������ ������Ű�� �������̽� Ŭ���� �ڵ鷯
template <typename T>
class IPropertyHandler : public PropertyHandlerBase
{
	GENERATE_CLASS_TYPE_INFO(IPropertyHandler)
		using ElementType = std::remove_all_extents_t<T>; // T Ÿ������ ���޵� ���� Ÿ���� ĸ���ع���

public:
	virtual ElementType& Get(void* object, size_t index = 0) const = 0;
	virtual void Set(void* object, const ElementType& value, size_t index = 0) const = 0;
};

// �ɹ� ���� �����Ϳ� ������ ĳ������ �����ϴ� �� �ڵ鷯 Ŭ����
template <typename TClass, typename T>
class PropertyHandler : public IPropertyHandler<T>
{
	GENERATE_CLASS_TYPE_INFO(PropertyHandler)
		using MemberPtr = T TClass::*; // ��ü�� �����ͷ� ��� �ֱ� ���� ������ �ڷ��� ����
	using ElementType = std::remove_all_extents_t<T>; // T Ÿ������ ���޵� ���� Ÿ���� ĸ���ع���

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

// ����ƽ Ÿ�Կ��� ����ϴ� �ڵ鷯
template <typename TClass, typename T>
class StaticPropertyHandler : public IPropertyHandler<T>
{
	GENERATE_CLASS_TYPE_INFO(StaticPropertyHandler)
		using ElementType = std::remove_all_extents_t<T>; // �迭���� base Ÿ�Ը� ������

public:
	explicit StaticPropertyHandler(T* ptr)
		: mPtr(ptr)
	{
	}

	// maybe_unused : �̰� �Ƚᵵ ��� ������ ����� �ǵ�
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
	template <typename T>
	struct ReturnValueWrapper
	{
	public:
		explicit ReturnValueWrapper(T& value)
			: mValue(&value)
		{
		}
		ReturnValueWrapper() = default;

		ReturnValueWrapper& operator=(const T& value)
		{
			*mValue = value;
			return *this;
		}

		// ����ȯ ������, T Ÿ������ ������ ��ȯ����
		operator T& ()
		{
			return *mValue;
		}

		T& Get()
		{
			return *mValue;
		}

		const T& Get() const {
			return *mValue;
		}

		friend bool operator==(const ReturnValueWrapper& lhs, const ReturnValueWrapper& rhs)
		{
			return *lhs.mValue == *rhs.mValue;
		}

		friend bool operator!=(const ReturnValueWrapper& lhs, const ReturnValueWrapper& rhs)
		{
			return !(lhs == rhs);
		}

	private:
		T* mValue = nullptr;
	};

public:
	Property(TypeInfo& owner, const PropertyInitializer& initializer)
		: mName(initializer.mName)
		, mType(initializer.mType)
		, mHandler(initializer.mHandler)
		, mPrintFunc(initializer.mPrintFunc)
	{
		owner.addProperty(this);
	}

	// ���޵� �ڽ��� ������Ƽ �ڵ鷯�� �ڷ����� ���� �˻�� �Ŀ� ó����
	template <typename T>
	ReturnValueWrapper<T> Get(void* object) const
	{
		const TypeInfo& typeInfo = mHandler.GetTypeInfo();

		if (typeInfo.IsChildOf<IPropertyHandler<T>>())
		{
			auto concreteHandler = static_cast<const IPropertyHandler<T>*>(&mHandler);
			return ReturnValueWrapper(concreteHandler->Get(object));
		}
		else
		{
			assert(false && "Property::Get<T> - invalid casting");
			return {};
		}
	}

	template <typename T>
	ReturnValueWrapper<T> Get(void* object, size_t index) const
	{
		const TypeInfo* registedElementType = mType.GetElementType();

		if (registedElementType == nullptr)
		{
			assert(false && "Property::Get<T> - invalid casting");
			return {};
		}

		const TypeInfo& elementType = TypeInfo::GetStaticTypeInfo<T>();

		if (registedElementType->IsA(elementType))
		{
			auto concreteHandler = static_cast<const IPropertyHandler<T>*>(&mHandler);
			return ReturnValueWrapper(concreteHandler->Get(object, index));
		}
		else
		{
			assert(false && "Property::Get<T> - invalid casting");
			return {};
		}
	}

	template <typename TClass, typename T>
	ReturnValueWrapper<T> Get(void* object) const {
		const TypeInfo& typeInfo = mHandler.GetTypeInfo();

		if (typeInfo.IsA<PropertyHandler<TClass, T>>())
		{
			auto concreateHandler = static_cast<const PropertyHandler<TClass, T>&>(mHandler);
			return ReturnValueWrapper(concreateHandler.Get(object));
		}
		else if (typeInfo.IsA<StaticPropertyHandler<TClass, T>>())
		{
			auto concreateHandler = static_cast<const StaticPropertyHandler<TClass, T>&>(mHandler);
			return ReturnValueWrapper(concreateHandler.Get(object));
		}
		else
		{
			assert(false && "Property::Get<TClass, T> - invalid casting");
			return {};
		}
	}

	template <typename TClass, typename T>
	ReturnValueWrapper<T> Get(void* object, size_t index) const {
		const TypeInfo& typeInfo = mHandler.GetTypeInfo();

		if (typeInfo.IsA<PropertyHandler<TClass, T>>())
		{
			auto concreateHandler = static_cast<const PropertyHandler<TClass, T>&>(mHandler);
			return ReturnValueWrapper(concreateHandler.Get(object, index));
		}
		else if (typeInfo.IsA<StaticPropertyHandler<TClass, T>>())
		{
			auto concreateHandler = static_cast<const StaticPropertyHandler<TClass, T>&>(mHandler);
			return ReturnValueWrapper(concreateHandler.Get(object, index));
		}
		else
		{
			assert(false && "Property::Get<TClass, T> - invalid casting");
			return {};
		}
	}

	template <typename T>
	void Set(void* object, const T& value) const
	{
		if (mHandler.GetTypeInfo().IsChildOf<IPropertyHandler<T>>())
		{
			auto concreateHandler = static_cast<const IPropertyHandler<T>*>(&mHandler);
			concreateHandler->Set(object, value);
		}
		else
		{
			assert(false && "Property::Set<T> - Invalied casting");
		}
	}

	template <typename TClass, typename T>
	void Set(void* object, const T& value) const {
		const TypeInfo& typeInfo = mHandler.GetTypeInfo();

		if (typeInfo.IsA<PropertyHandler<TClass, T>>())
		{
			auto concreateHandler = static_cast<const PropertyHandler<TClass, T>&>(mHandler);
			concreateHandler.Set(object, value);
		}
		else if (typeInfo.IsA<StaticPropertyHandler<TClass, T>>())
		{
			auto concreateHandler = static_cast<const StaticPropertyHandler<TClass, T>&>(mHandler);
			concreateHandler.Set(object, value);
		}
		else
		{
			assert(false && "Property::Set<TClass, T> - Invalid casting");
		}
	}

	void Print(void* object, int indent) const
	{
		std::string indentStr(indent * 4, ' ');
		std::cout
			<< indentStr
			<< "Type: " << mType.GetName()
			<< ", Name: " << mName;

		mPrintFunc(mHandler.GetRawPointer(object));
		std::cout << std::endl;
	}

	const char* GetName() const
	{
		return mName;
	}

	const TypeInfo& GetTypeInfo() const
	{
		return mType;
	}

	void* GetRawPointer(void* object) const
	{
		return mHandler.GetRawPointer(object);
	}

private:
	using PrintFuncPtr = void(*)(void*);

	const char* mName = nullptr;
	const TypeInfo& mType;
	const PropertyHandlerBase& mHandler;
	PrintFuncPtr mPrintFunc = nullptr;
};

template <typename T>
concept OstreamWritable = requires(std::ostream & os, T value) {
	{ os << value } -> std::same_as<std::ostream&>;
};

template <OstreamWritable T>
void Print(void* object)
{
	T* value = static_cast<T*>(object);
	std::cout << ", Value : " << *value;
}

template <typename T>
void Print(void*)
{
	std::cout << ", Value : None ";
}

template <typename TClass, typename T, typename TPtr, TPtr ptr>
class PropertyRegister
{
public:
	PropertyRegister(const char* name, TypeInfo& typeInfo)
	{
		using ElementType = std::remove_all_extents_t<T>;

		std::cout << "[Registering Property] "
			<< "Class: " << typeid(TClass).name()
			<< ", Member Type: " << typeid(T).name()
			<< ", ElementType: " << typeid(ElementType).name()
			<< ", Name: " << name
			<< std::endl;

		if constexpr (std::is_member_pointer_v<TPtr>)
		{
			// non-static
			static PropertyHandler<TClass, T> handler(ptr);
			static PropertyInitializer initializer = { .mName = name, .mType = TypeInfo::GetStaticTypeInfo<T>(), .mHandler = handler, .mPrintFunc = &Print<T> };
			static Property property(typeInfo, initializer);

		}
		else
		{
			// static
			static StaticPropertyHandler<TClass, T> handler(ptr);
			static PropertyInitializer initializer = { .mName = name, .mType = TypeInfo::GetStaticTypeInfo<T>(), .mHandler = handler, .mPrintFunc = &Print<T> };
			static Property property(typeInfo, initializer);
		}
	}
};
