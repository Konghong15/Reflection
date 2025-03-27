#pragma once

#include "TypeInfo.h"

namespace reflection
{
#define PROPERTY(Name) \
	inline static struct RegistPropertyExecutor_#Name \
	{ \
		RegistPropertyExecutor_##Name() \
		{ /* ���ø� �Ű����� : { Ŭ������ ���ǵ� ThisType�� ������ Ÿ��, ������ ������ Ÿ��, ������ �ּ� } */\
		  /* �Ϲ� �Ű����� : �̸�, �ش� Ÿ���� Ÿ������ */\
			static PropertyRegister<ThisType, decltype(##Name), decltype(&ThisType::##Name), &ThisType::##Name> property_register_##Name{ #Name, ThisType::StaticTypeInfo() }; \
		} \
	} regist_##Name;

	// Ÿ���� ���� �����ϰ� ȣ���ϱ� ���� �⺻ �������̽��� ������
	class PropertyHandlerBase
	{
		GENERATE_CLASS_TYPE_INFO(PropertyHandlerBase)

	public:
		virtual ~PropertyHandlerBase() = default;
	};

	// ���⼭ Type���� Get/Set ������ ������
	template <typename T>
	class IPropertyHandler : public PropertyHandlerBase
	{
		GENERATE_CLASS_TYPE_INFO(IPropertyHandler)
			using ElementType = std::remove_all_extents_t<T>; // �迭���� base Ÿ�Ը� ������

	public:
		virtual ElementType& Get(void* object, size_t index = 0) const = 0;
		virtual void Set(void* object, const ElementType& value, size_t index = 0) const = 0;
	};

	// �ڵ鷯�� ����
	// 1. �ɹ� ������ �����͸� ������
	// 2. ��Ÿ�ӿ� void*�� ���޵� ������Ʈ�� ���� Ÿ������ ĳ�����ؼ� ������
	// ?. ����ƽ �ɹ��� ������ �ʿ䰡 �����ٸ� �̰� ���� �ʿ�������?
	template <typename TClass, typename T>
	class PropertyHandler : public IPropertyHandler<T>
	{
		GENERATE_CLASS_TYPE_INFO(PropertyHandler)
			using MemberPtr = T TClass::*; ; // 
		using ElementType = std::remove_all_extents_t<T>; // �迭���� base Ÿ�Ը� ������

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
			// ���� ������ ������ ��쿡�� ī����
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

			// ����ȯ ������, T Ÿ������ ������ ��ȯ����
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

		// �� ���� Ŭ���� ������ ���ø��� �� �� �ֱ���
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
		const PropertyHandlerBase& mHandler; // ���⼭ �� �߻����� ������ ��ƹ�����.?
	};

	// ������Ƽ ��ü ������ �ʿ��� �����͸� static���� ��������
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
