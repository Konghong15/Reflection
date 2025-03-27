#pragma once

#include "TypeInfo.h"

namespace reflection
{
#define PROPERTY(Name) \
	inline static struct RegistPropertyExecutor_#Name \
	{ \
		RegistPropertyExecutor_##Name() \
		{ /* 템플릿 매개변수 : { 클래스에 정의된 ThisType과 변수의 타입, 변수의 포인터 타입, 변수의 주소 } */\
		  /* 일반 매개변수 : 이름, 해당 타입의 타입인포 */\
			static PropertyRegister<ThisType, decltype(##Name), decltype(&ThisType::##Name), &ThisType::##Name> property_register_##Name{ #Name, ThisType::StaticTypeInfo() }; \
		} \
	} regist_##Name;

	// 타입을 몰라도 보관하고 호출하기 위한 기본 인터페이스를 제공함
	class PropertyHandlerBase
	{
		GENERATE_CLASS_TYPE_INFO(PropertyHandlerBase)

	public:
		virtual ~PropertyHandlerBase() = default;
	};

	// 여기서 Type마다 Get/Set 구현을 강제함
	template <typename T>
	class IPropertyHandler : public PropertyHandlerBase
	{
		GENERATE_CLASS_TYPE_INFO(IPropertyHandler)
			using ElementType = std::remove_all_extents_t<T>; // 배열에서 base 타입만 남겨줌

	public:
		virtual ElementType& Get(void* object, size_t index = 0) const = 0;
		virtual void Set(void* object, const ElementType& value, size_t index = 0) const = 0;
	};

	// 핸들러의 역할
	// 1. 맴버 변수의 포인터를 저장함
	// 2. 런타임에 void*로 전달된 오브젝트를 원래 타입으로 캐스팅해서 접근함
	// ?. 스태틱 맴버를 관리할 필요가 없었다면 이게 굳이 필요했을까?
	template <typename TClass, typename T>
	class PropertyHandler : public IPropertyHandler<T>
	{
		GENERATE_CLASS_TYPE_INFO(PropertyHandler)
			using MemberPtr = T TClass::*; ; // 
		using ElementType = std::remove_all_extents_t<T>; // 배열에서 base 타입만 남겨줌

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
		void set(std::map<KeyType, ValueType, Pred, Alloc>& dest, std::map<KeyType, ValueType, Pred, Alloc>& src) const
		{
			// 복사 대입이 가능한 경우에만 카피함
			if constexpr (std::is_copy_assignable_v<KeyType> && std::is_copy_assignable_v<ValueType>)
			{
				dest = src
			}
		}

	private:
		MemberPtr mPtr = nullptr;
	};

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
				return (*m_ptr)[index];
			}
			else
			{
				return *m_ptr;
			}
		}

		virtual void Set([[maybe_unused]] void* object, const ElementType& value, size_t index = 0) const override
		{
			if constexpr (std::is_array_v<T>)
			{
				(*m_ptr)[index] = value;
			}
			else
			{
				*m_ptr = value;
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
		// const SerializeFunc m_serializer = nullptr;
		// const ParseFunc m_parser = nullptr;
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
				return *mValue
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

		// 이 래퍼 클래스 때문에 템플릿을 쓸 수 있구나
		template <typename T>
		ReturnValueWrapper<T> Get(void* object) const
		{
			if (mHandler.GetTypeInfo().IsChildOf<IPropertyHandler<T>>())
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
		void Set(void* object, const T& value) const
		{
			if (m_handler.GetTypeInfo().IsChildOf<IPropertyHandler<T>>())
			{
				auto concreteHandler = static_cast<const IPropertyHandler<T>*>(&mHandler);
				concreteHandler->Set(object, value);
			}
			else
			{
				assert(false && "Property::Set<T> - Invalied casting");
			}
		}

		const char* GetName() const;

	private:
		const char* mName = nullptr;
		const TypeInfo& mType;
		const PropertyHandlerBase& mHandler; // 여기서 젤 추상적인 참조를 잡아버리네.?
	};

	// 프로퍼티 객체 생성에 필요한 데이터를 static으로 생성해줌
	template <typename TClass, typename T, typename TPtr, TPtr ptr>
	class PropertyRegister
	{
	public:
		PropertyRegister(const char* name, TypeInfo& typeInfo)
		{
			if constexpr (std::is_member_pointr_v<TPtr>)
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
				static Property property(typeInfo, initializer)
			}
		}
	};

}
