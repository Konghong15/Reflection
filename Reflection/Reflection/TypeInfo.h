#pragma once

#include <concepts>
#include <map>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>
#include <cassert>

// -- 전방선언 --
class Method;
class Property;
class TypeInfo;

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
private: \


template <typename T>
concept HasSuper = requires
{
	typename T::Super;
} && !std::same_as<typename T::Super, void>;

template <typename T>
concept HasStaticTypeInfo = requires
{
	T::StaticTypeInfo();
};

template <typename T, typename U = void>
struct SuperClassTypeDeduction
{
	using Type = void;
};

template <typename T>
struct SuperClassTypeDeduction<T, std::void_t<typename T::ThisType>>
{
	using Type = T::ThisType;
};

// 부모의 타입 인포 객체를 얻어오는 객체
template <typename T>
struct TypeInfoInitializer
{
	TypeInfoInitializer(const std::string& name)
		: mName(name)
	{
		if constexpr (HasSuper<T>)
		{
			mSuper = &(typename T::Super::StaticTypeInfo());
		}

		if constexpr (std::is_array_v<T>)
		{
			using ElementType = std::remove_all_extents_t<T>;
			mElementType = &TypeInfo::template GetStaticTypeInfo<ElementType>();
		}
	}

	const std::string mName = nullptr;
	const TypeInfo* mSuper = nullptr;
	const TypeInfo* mElementType = nullptr;
};

template <typename T>
constexpr const char* PrettyTypeName()
{
	return __FUNCSIG__;
}

template <typename T>
std::string ExtractTypeName()
{
	std::string_view sig = PrettyTypeName<T>();

	// MSVC는 "PrettyTypeName<...>" 안에 타입이 들어있음
	auto start = sig.find("PrettyTypeName<") + strlen("PrettyTypeName<");
	auto end = sig.find(">(void)");

	if (start == std::string_view::npos || end == std::string_view::npos || end <= start)
		return std::string(sig); // fallback

	return std::string(sig.substr(start, end - start));
}

class TypeInfo
{
	friend class Method;
	friend class Property;

public:
	// 왜 생성자에서 initializer를 받을까?
	template <typename T>
	explicit TypeInfo(const TypeInfoInitializer<T>& initializer)
		: mTypeHash(typeid(T).hash_code())
		, mName(initializer.mName)
		, mFullName(typeid(T).name())
		, mSuper(initializer.mSuper)
		, mIsArray(std::is_array_v<T>)
		, mIsPointer(std::is_pointer_v<T>)
		, mArrayExtent(std::extent_v<T>)
		, mElementType(initializer.mElementType)
	{
		if constexpr (HasSuper<T>)
		{
			collectSuperMethods();
			collectSuperProperties();
		}
		if constexpr(std::is_array_v<T>)
		{

		}
	}

	template <typename T> requires HasStaticTypeInfo<T>
	static const TypeInfo& GetStaticTypeInfo()
	{
		return T::StaticTypeInfo();
	}

	template <typename T> requires std::is_pointer_v<T>&& HasStaticTypeInfo<std::remove_pointer_t<T>>
	static const TypeInfo& GetStaticTypeInfo()
	{
		return std::remove_pointer_t<T>::StaticTypeInfo();
	}

	template <typename T> requires (!HasStaticTypeInfo<T>) && (!HasStaticTypeInfo<std::remove_pointer_t<T>>)
		static const TypeInfo& GetStaticTypeInfo()
	{
		static TypeInfo typeInfo{ TypeInfoInitializer<T>(ExtractTypeName<T>()) };
		return typeInfo;
	}

	void PrintObject(void* object, int indent = 0, bool recursive = false) const;

	void PrintProperties(int indent = 0) const;
	void PrintPropertiesRecursive(int indent = 0) const;
	void PrintMethods(int indent = 0) const;

	bool IsA(const TypeInfo& other) const
	{
		if (this == &other)
		{
			return true;
		}

		return mTypeHash == other.mTypeHash;
	}
	template <typename T>
	bool IsA() const
	{
		return IsA(GetStaticTypeInfo<T>());
	}

	bool IsChildOf(const TypeInfo& other) const
	{
		if (IsA(other))
		{
			return true;
		}

		for (const TypeInfo* superOrNull = mSuper; superOrNull != nullptr; superOrNull = superOrNull->GetSuperOrNull())
		{
			assert(superOrNull != nullptr);
			if (superOrNull->IsA(other))
			{
				return true;
			}
		}

		return false;
	}

	template <typename T>
	bool IsChildOf() const
	{
		return IsChildOf(GetStaticTypeInfo<T>());
	}

	inline const std::vector<const Method*>& GetMethods() const
	{
		return mMethods;
	}
	inline const Method* GetMethod(const char* name) const
	{
		auto iter = mMethodMap.find(name);
		return (iter == mMethodMap.end()) ? nullptr : iter->second;
	}

	inline const std::vector<const Property*>& GetProperties() const
	{
		return mProperties;
	}
	inline const Property* GetProperty(const char* name) const
	{
		auto iter = mPropertyMap.find(name);
		return (iter == mPropertyMap.end()) ? nullptr : iter->second;
	}

	inline const TypeInfo* GetSuperOrNull() const
	{
		return mSuper;
	}

	const std::string GetName() const
	{
		return mName;
	}

	inline bool IsArray() const
	{
		return mIsArray;
	}

	bool HasElementType() const {
		return mElementType != nullptr;
	}

	const TypeInfo* GetElementType() const {
		return mElementType;
	}

	size_t GetArrayExtent() const
	{
		return mArrayExtent;
	}

	inline bool IsPointer() const
	{
		return mIsPointer;
	}

private:
	void addMethod(const Method* method);
	void addProperty(const Property* property);
	void collectSuperMethods();
	void collectSuperProperties();

private:
	size_t mTypeHash;
	const std::string mName;
	std::string mFullName;
	const TypeInfo* mSuper = nullptr;

	bool mIsArray;
	bool mIsPointer;

	std::vector<const Method*> mMethods;
	std::map<std::string_view, const Method*> mMethodMap;

	std::vector<const Property*> mProperties;
	std::map<std::string_view, const Property*> mPropertyMap;

	const TypeInfo* mElementType = nullptr;
	size_t mArrayExtent = 0;
};
