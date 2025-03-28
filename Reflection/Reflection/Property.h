#pragma once

#include <cassert>
#include <string>

#include "TypeInfo.h"

// -- ��ũ��
#define PROPERTY(Name) \
	inline static struct RegistPropertyExecutor_#Name \
	{ \
		RegistPropertyExecutor_##Name() \
		{ \
			static PropertyRegister<ThisType, decltype(##Name), decltype(&ThisType::##Name), &ThisType::##Name> property_register_##Name{ #Name, ThisType::StaticTypeInfo() }; \
		} \
	} regist_##Name;

	// �������� ������ ���� ���̽� �ڵ鷯
class PropertyHandlerBase
{
	GENERATE_CLASS_TYPE_INFO(PropertyHandlerBase)

public:
	virtual ~PropertyHandlerBase() = default;
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

private:
	T* mPtr = nullptr;
};

struct PropertyInitializer
{
	const char* mName = nullptr;
	const TypeInfo& mType;
	const PropertyHandlerBase& mHandler;
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

	// � Ŭ������ ���������� ó���� �� �����
	template <typename TClass, typename T>
	ReturnValueWrapper<T> Get(void* object) const {
		const TypeInfo& typeInfo = mHandler.GetTypeInfo();

		if (typeInfo.IsA<PropertyHandler<TClass, T>>())
		{
			auto concreteHandler = static_cast<const PropertyHandler<TClass, T>&>(mHandler);
			return ReturnValueWrapper(concreteHandler.Get(object));
		}
		else if (typeInfo.IsA<StaticPropertyHandler<TClass, T>>())
		{
			auto concreteHandler = static_cast<const StaticPropertyHandler<TClass, T>&>(mHandler);
			return ReturnValueWrapper(concreteHandler.Get(object));
		}
	}

	template <typename T>
	void Set(void* object, const T& value) const
	{
		if (mHandler.GetTypeInfo().IsChildOf<IPropertyHandler<T>>())
		{
			auto concreteHandler = static_cast<const IPropertyHandler<T>*>(&mHandler);
			concreteHandler->Set(object, value);
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
			auto concreteHandler = static_cast<const StaticPropertyHandler<TClass, T>&>(mHandler);
			concreteHandler.Set(object, value);
		}
		else
		{
			assert(false && "Property::Set<TClass, T> - Invalid casting");
		}
	}

	const char* GetName() const
	{
		return mName;
	}

	const TypeInfo& GetTypeInfo() const
	{
		return mType;
	}


private:
	const char* mName = nullptr;
	const TypeInfo& mType;
	const PropertyHandlerBase& mHandler;
};

template <typename T>
struct SizeOfArray
{
	constexpr static size_t Value = 1;
};

template <typename T, size_t N>
struct SizeOfArray<T[N]>
{
	constexpr static size_t Value = SizeOfArray<T>::Value * N;
};

// ������Ƽ ��ü ������ �ʿ��� �����͸� static���� ��������
template <typename TClass, typename T, typename TPtr, TPtr ptr>
class PropertyRegister
{
public:
	PropertyRegister(const char* name, TypeInfo& typeInfo)
	{
		if constexpr (std::is_member_pointer_v<TPtr>)
		{
			// non-static
			static PropertyHandler<TClass, T> handler(ptr);
			static PropertyInitializer initializer = { .mName = name, .mType = TypeInfo::GetStaticTypeInfo<T>(), .mHandler = handler };
			static Property property(typeInfo, initializer);
		}
		else
		{
			// static
			static StaticPropertyHandler<TClass, T> handler(ptr);
			static PropertyInitializer initializer = { .mName = name, .mType = TypeInfo::GetStaticTypeInfo<T>(), .mHandler = handler };
			static Property property(typeInfo, initializer);
		}
	}
};
