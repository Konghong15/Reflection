#pragma once

#include <cassert>
#include <string>

#include "TypeInfo.h"

// -- 매크로
#define PROPERTY(Name) \
	inline static struct RegistPropertyExecutor_#Name \
	{ \
		RegistPropertyExecutor_##Name() \
		{ \
			static PropertyRegister<ThisType, decltype(##Name), decltype(&ThisType::##Name), &ThisType::##Name> property_register_##Name{ #Name, ThisType::StaticTypeInfo() }; \
		} \
	} regist_##Name;

	// 범용적인 참조를 위한 베이스 핸들러
class PropertyHandlerBase
{
	GENERATE_CLASS_TYPE_INFO(PropertyHandlerBase)

public:
	virtual ~PropertyHandlerBase() = default;
};

// Get/Set 구현을 강제시키는 인터페이스 클래스 핸들러
template <typename T>
class IPropertyHandler : public PropertyHandlerBase
{
	GENERATE_CLASS_TYPE_INFO(IPropertyHandler)
		using ElementType = std::remove_all_extents_t<T>; // T 타입으로 전달된 것의 타입을 캡쳐해버림

public:
	virtual ElementType& Get(void* object, size_t index = 0) const = 0;
	virtual void Set(void* object, const ElementType& value, size_t index = 0) const = 0;
};

// 맴버 변수 포인터와 안전한 캐스팅을 보장하는 실 핸들러 클래스
template <typename TClass, typename T>
class PropertyHandler : public IPropertyHandler<T>
{
	GENERATE_CLASS_TYPE_INFO(PropertyHandler)
		using MemberPtr = T TClass::*; // 객체를 포인터로 들고 있기 위한 포인터 자료형 정의
	using ElementType = std::remove_all_extents_t<T>; // T 타입으로 전달된 것의 타입을 캡쳐해버림

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

// 스태틱 타입에서 사용하는 핸들러
template <typename TClass, typename T>
class StaticPropertyHandler : public IPropertyHandler<T>
{
	GENERATE_CLASS_TYPE_INFO(StaticPropertyHandler)
		using ElementType = std::remove_all_extents_t<T>; // 배열에서 base 타입만 남겨줌

public:
	explicit StaticPropertyHandler(T* ptr)
		: mPtr(ptr)
	{
	}

	// maybe_unused : 이거 안써도 경고 붙이지 말라는 의도
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

		// 형변환 연산자, T 타입으로 묵시적 변환가능
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

	// 전달된 자식이 프로퍼티 핸들러의 자료형에 의해 검사된 후에 처리됨
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

	// 어떤 클래스의 데이터인지 처리할 때 사용함
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

// 프로퍼티 객체 생성에 필요한 데이터를 static으로 생성해줌
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
