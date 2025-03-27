#pragma once

#include <concepts>
#include <map>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>

namespace reflection
{
	// -- 매크로 -- 
#define GENERATE_CLASS_TYPE_INFO(TypeName) \
private: \
	friend SuperClassTypeDeduction; \
	friend TypeInfoInitializer; \
\
public: \
	using Super = typename SuperClassTypeDeduction<TypeName>::Type; \
	using ThisType = TypeName; \
\
	static TypeInfo& StaticTypeInfo() \
	{ \
			static TypeInfo typeInfo { TypeInfoInitializer<ThisType>(#TypeName)}; \
			return typeInfo; \
	} \
\
	virtual const TypeInfo& GetTypeInfo() const \
	{ \
		return mTypeInfo; \
	} \
\
private: \
	inline static TypeInfo& mTypeInfo = StaticTypeInfo(); \
\
private: 

	// -- 전방선언 --
	class Method;
	class Property;
	class TypeInfo;

	// -- 컴파일 타임 조건 검사 -- 
	template <typename T>
	concept HasSuper = requires
	{
		typename T::Super;
	} && !std::same_as<typename T::Super, void>;

	template <typename T>
	concept HasStaticTypeInfo = requires
	{
		T::StaticTypeInfo(); // 이 맴버가 있으면 조건이 true로 평가됨
	};

	// 부모의 타입 인포 객체를 얻어오는 객체
	template <typename T>
	struct TypeInfoInitializer
	{
		TypeInfoInitializer(const char* name)
			: mName(name)
		{
			if constexpr (HasSuper<T>)
			{
				mSuper = &(typename T::Super::StaticTypeInfo());
			}
		}

		const char* mName = nullptr;
		const TypeInfo* mSuper = nullptr;
	};

	class TypeInfo
	{
		friend Method;
		friend Property;

	public:
		// 왜 생성자에서 initializer를 받을까?
		template <typename T>
		explicit TypeInfo(const TypeInfoInitializer<T>& initializer)
			: mTypeHash(typeid(T).hash_code())
			, mName(initializer.mName)
			, mFullName(typeid(T).name())
			, mSuper(initializer.mSuper)
			, mIsArray(std::is_array_v<T>)
		{
			if constexpr (HasSuper<T>)
			{

			}
		}

		template <typename T> requires HasStaticTypeInfo<T>
		static const TypeInfo& GetStaticTypeInfo();
		template <typename T> requires std::is_pointer_v<T>&& HasStaticTypeInfo<std::remove_pointer_t<T>>
		static const TypeInfo& GetStaticTypeInfo();
		template <typename T> requires (!HasStaticTypeInfo<T>) && (!HasStaticTypeInfo<std::remove_pointer_t<T>>)
		static const TypeInfo& GetStaticTypeInfo();

		template <typename T>
		bool IsA() const;
		bool IsA(const TypeInfo& other) const;

		template <typename T>
		bool IsChildOf() const;
		bool IsChildOf(const TypeInfo& other) const;

		inline const std::vector<const Method*>& GetMethods() const;
		inline const Method* GetMethod(const char* name) const;

		inline const std::vector<const Property*>& GetProperties() const;
		inline const Property* GetProperty(const char* name) const;

		inline const TypeInfo* GetSuperOrNull() const;

		const char* GetName() const;

		inline bool IsArray() const;

	private:
		void collectSuperMethods();
		void collectSuperProperties();
		void addProperty(const Property* property);
		void addMethod(const Method* method);

	private:
		size_t mTypeHash;
		const char* mName = nullptr;
		std::string mFullName;
		const TypeInfo* mSuper = nullptr;

		bool mIsArray = false;

		std::vector<const Method*> mMethods;
		std::map<std::string_view, const Method*> mMethodMap;

		std::vector<const Property*> mProperties;
		std::map<std::string_view, const Property*> mPropertyMap;
	};

	template <typename T> requires HasStaticTypeInfo<T>
	const TypeInfo& TypeInfo::GetStaticTypeInfo() {
		return T::StaticTypeInfo();
	}

	template <typename T> requires std::is_pointer_v<T>&& HasStaticTypeInfo<std::remove_pointer_t<T>>
	const TypeInfo& TypeInfo::GetStaticTypeInfo()
	{
		return std::remove_pointer_t<T>::StaticTypeInfo()
	}

	template <typename T> requires (!HasStaticTypeInfo<T>) && (!HasStaticTypeInfo<std::remove_pointer_t<T>>)
		const TypeInfo& TypeInfo::GetStaticTypeInfo()
	{
		static TypeInfo typeInfo{ TypeInfoInitializer<T>("unreflected type variable") };
		return typeInfo;
	}

	template <typename T>
	bool TypeInfo::IsA() const
	{
		return IsA(GetStaticTypeInfo<T>());
	}
}