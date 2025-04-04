#pragma once

#include <cassert>
#include <concepts>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <vector>

#define GENERATE_TYPE_INFO(TypeName) \
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

class Method;
class Procedure;
class Property;
class TypeInfo;

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

template <typename T, typename = void>
struct IsIterable : std::false_type {};

template <typename T>
struct IsIterable<T, std::void_t<
	decltype(std::declval<T>().begin()),
	decltype(std::declval<T>().end())
	>> : std::true_type {};

template <typename T, typename = void>
struct HasValueType : std::false_type {};

template <typename T>
struct HasValueType<T, std::void_t<typename T::value_type>> : std::true_type {};

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
		if constexpr (IsIterable<T>::value && HasValueType<T>::value)
		{
			mIsIterable = true;
			using ElementType = typename T::value_type;
			mIteratorElementType = &TypeInfo::template GetStaticTypeInfo<ElementType>();
		}
	}

	const std::string mName = nullptr;
	const TypeInfo* mSuper = nullptr;
	const TypeInfo* mElementType = nullptr;
	bool mIsIterable = false;
	const TypeInfo* mIteratorElementType = nullptr;
};


// T 타입으로부터 이름 추출
template <typename T>
std::string ExtractTypeName()
{
	std::string_view sig = __FUNCSIG__;

	auto start = sig.find("ExtractTypeName<") + strlen("ExtractTypeName<");
	auto end = sig.find(">(void)");

	if (start == std::string_view::npos || end == std::string_view::npos || end <= start)
		return std::string(sig);

	return std::string(sig.substr(start, end - start));
}

class TypeInfo
{
	friend class Method;
	friend class Property;
	friend class Procedure;

public:
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
		, mIsIterable(initializer.mIsIterable)
		, mIteratorElementType(initializer.mIteratorElementType)
	{
		if constexpr (HasSuper<T>)
		{
			collectSuperMethods();
			collectSuperProperties();
			collectSuperProcedures();
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
		static TypeInfo typeInfo{ TypeInfoInitializer<T>(ExtractTypeName<T>()) };
		return typeInfo;
	}
	template <typename T> requires (!HasStaticTypeInfo<T>) && (!HasStaticTypeInfo<std::remove_pointer_t<T>>)
		static const TypeInfo& GetStaticTypeInfo()
	{
		static TypeInfo typeInfo{ TypeInfoInitializer<T>(ExtractTypeName<T>()) };
		return typeInfo;
	}

	template <typename T>
	bool IsA() const
	{
		return IsA(GetStaticTypeInfo<T>());
	}
	template <typename T>
	bool IsChildOf() const
	{
		return IsChildOf(GetStaticTypeInfo<T>());
	}

	void PrintMethods(int indent = 0) const;
	void PrintProcedures(int indent = 0) const;
	void PrintProperties(int indent = 0) const;
	void PrintPropertiesRecursive(int indent = 0) const;
	void PrintPropertyValues(void* object, int indent = 0) const;
	void PrintPropertyValuesRecursive(void* object, int indent = 0) const;

	inline bool IsA(const TypeInfo& other) const;
	inline bool IsChildOf(const TypeInfo& other) const;

	inline const std::vector<const Method*>& GetMethods() const;
	inline const Method* GetMethod(const char* name) const;

	inline const std::vector<const Procedure*>& GetProcedures() const;
	inline const Procedure* GetProcedure(const char* name) const;

	inline const std::vector<const Property*>& GetProperties() const;
	inline const Property* GetProperty(const char* name) const;

	inline const TypeInfo* GetSuperOrNull() const;
	inline const std::string GetName() const;

	inline bool IsArray() const;
	inline bool HasElementType() const;
	inline const TypeInfo* GetElementType() const;
	inline size_t GetArrayExtent() const;

	inline bool IsPointer() const;
	inline size_t GetSize() const;

	inline bool IsIterable() const;
	inline const TypeInfo* GetIteratorElementType() const;

private:
	void addMethod(const Method* method);
	void addProperty(const Property* property);
	void addProcedure(const Procedure* property);

	void collectSuperMethods();
	void collectSuperProperties();
	void collectSuperProcedures();

private:
	size_t mTypeHash;
	const std::string mName;
	std::string mFullName;
	const TypeInfo* mSuper = nullptr;

	bool mIsArray;
	bool mIsPointer;

	std::vector<const Procedure*> mProcedures;
	std::map<std::string_view, const Procedure*> mProcedureMap;

	std::vector<const Method*> mMethods;
	std::map<std::string_view, const Method*> mMethodMap;

	std::vector<const Property*> mProperties;
	std::map<std::string_view, const Property*> mPropertyMap;

	const TypeInfo* mElementType = nullptr;
	size_t mArrayExtent = 0;
	size_t mSize;

	bool mIsIterable = false;
	const TypeInfo* mIteratorElementType = nullptr;
};

inline bool TypeInfo::IsA(const TypeInfo& other) const
{
	return (this == &other) || (mTypeHash == other.mTypeHash);
}

inline bool TypeInfo::IsChildOf(const TypeInfo& other) const
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

inline const std::vector<const Method*>& TypeInfo::GetMethods() const
{
	return mMethods;
}

inline const Method* TypeInfo::GetMethod(const char* name) const
{
	auto iter = mMethodMap.find(name);
	return (iter == mMethodMap.end()) ? nullptr : iter->second;
}

inline const std::vector<const Procedure*>& TypeInfo::GetProcedures() const
{
	return mProcedures;
}

inline const Procedure* TypeInfo::GetProcedure(const char* name) const
{
	auto iter = mProcedureMap.find(name);
	return (iter == mProcedureMap.end()) ? nullptr : iter->second;
}

inline const std::vector<const Property*>& TypeInfo::GetProperties() const
{
	return mProperties;
}

inline const Property* TypeInfo::GetProperty(const char* name) const
{
	auto iter = mPropertyMap.find(name);
	return (iter == mPropertyMap.end()) ? nullptr : iter->second;
}

inline const TypeInfo* TypeInfo::GetSuperOrNull() const
{
	return mSuper;
}

inline const std::string TypeInfo::GetName() const
{
	return mName;
}

inline bool TypeInfo::IsArray() const
{
	return mIsArray;
}

inline bool TypeInfo::HasElementType() const
{
	return mElementType != nullptr;
}

inline const TypeInfo* TypeInfo::GetElementType() const
{
	return mElementType;
}

inline size_t TypeInfo::GetArrayExtent() const
{
	return mArrayExtent;
}

inline bool TypeInfo::IsPointer() const
{
	return mIsPointer;
}

inline size_t TypeInfo::GetSize() const
{
	return mSize;
}

inline bool TypeInfo::IsIterable() const
{
	return mIsIterable;
}

inline const TypeInfo* TypeInfo::GetIteratorElementType() const
{
	return mIteratorElementType;
}