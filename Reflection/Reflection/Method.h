#pragma once

#include <cassert>
#include <type_traits>

#include "TypeInfo.h"

class CallableBase
{
	GENERATE_CLASS_TYPE_INFO(CallableBase)

public:
	virtual ~CallableBase() = default;
};

template <typename TRet, typename... TArgs>
class ICallable : public CallableBase
{
	GENERATE_CLASS_TYPE_INFO(ICallable)

public:
	virtual TRet Invoke(void* caller, TArgs&&... args) const = 0;
};

template <typename TClass, typename TRet, typename... TArgs>
class Callable : public ICallable<TRet, TArgs...>
{
	GENERATE_CLASS_TYPE_INFO(Callable)
		using FuncPtr = TRet(TClass::*)(TArgs...);

public:
	Callable(FuncPtr ptr) :
		mPtr(ptr) {
	}

	virtual TRet Invoke(void* caller, TArgs&&... args) const override
	{
		if constexpr (std::same_as<TRet, void>)
		{
			(static_cast<TClass*>(caller)->*mPtr)(std::forward<TArgs>(args)...);
		}
		else
		{
			return (static_cast<TClass*>(caller)->*mPtr)(std::forward<TArgs>(args)...);
		}
	}

private:
	FuncPtr mPtr = nullptr;
};

template <typename TClass, typename TRet, typename... TArgs>
class StaticCallable : public ICallable<TRet, TArgs...>
{
	GENERATE_CLASS_TYPE_INFO(StaticCallable)
		using FuncPtr = TRet(*)(TArgs...);

public:
	StaticCallable([[maybe_unused]] TClass* owner, FuncPtr ptr) :
		mPtr(ptr) {
	}

	virtual TRet Invoke([[maybe_unused]] void* caller, TArgs&&... args) const override
	{
		if constexpr (std::same_as<TRet, void>)
		{
			(*mPtr)(std::forward<TArgs>(args)...);
		}
		else
		{
			return (*mPtr)(std::forward<TArgs>(args)...);
		}
	}

private:
	FuncPtr mPtr = nullptr;
};

class Method
{
public:
	template <typename TRet, typename... TArgs>
	Method(TypeInfo& owner, [[maybe_unused]] TRet(*ptr)(TArgs...), const char* name, const CallableBase& callable) :
		mName(name),
		mCallable(callable)
	{
		collectFunctionSignature<TRet, TArgs...>();
		owner.AddMethod(this);
	}

	template <typename TClass, typename TRet, typename... TArgs>
	Method(TypeInfo& owner, [[maybe_unused]] TRet(TClass::* ptr)(TArgs...), const char* name, const CallableBase& callable) :
		mName(name),
		mCallable(callable)
	{
		collectFunctionSignature<TRet, TArgs...>();
		owner.AddMethod(this);
	}

	template <typename TClass, typename TRet, typename... TArgs>
	TRet Invoke(void* caller, TArgs&&... args) const
	{
		const TypeInfo& typeinfo = mCallable.GetTypeInfo();
		if (typeinfo.IsChildOf<Callable<TClass, TRet, TArgs...>>())
		{
			auto concreateCallable = static_cast<const Callable<TClass, TRet, TArgs...>&>(mCallable);
			if constexpr (std::same_as<TRet, void>)
			{
				concreateCallable.Invoke(caller, std::forward<TArgs>(args)...);
			}
			else
			{
				return concreateCallable.Invoke(caller, std::forward<TArgs>(args)...);
			}
		}
		else if (typeinfo.IsChildOf<StaticCallable<TClass, TRet, TArgs...>>())
		{
			auto concreateCallable = static_cast<const StaticCallable<TClass, TRet, TArgs...>&>(mCallable);
			if constexpr (std::same_as<TRet, void>)
			{
				concreateCallable.Invoke(caller, std::forward<TArgs>(args)...);
			}
			else
			{
				return concreateCallable.Invoke(caller, std::forward<TArgs>(args)...);
			}
		}
		else
		{
			assert(false && "Method::Invoke<TClass, TRet, TArgs...> - Invalied casting");
			if constexpr (!std::same_as<TRet, void>)
			{
				return {};
			}
		}
	}

	template <typename TRet, typename... TArgs>
	TRet Invoke(void* owner, TArgs&&... args) const
	{
		if (mCallable.GetTypeInfo().IsChildOf<ICallable<TRet, TArgs...>>())
		{
			auto concreateCallable = static_cast<const ICallable<TRet, TArgs...>*>(&mCallable);
			if constexpr (std::same_as<TRet, void>)
			{
				concreateCallable->Invoke(owner, std::forward<TArgs>(args)...);
			}
			else
			{
				return concreateCallable->Invoke(owner, std::forward<TArgs>(args)...);
			}
		}
		else
		{
			assert(false && "Method::Invoke<TRet, TArgs...> - Invalied casting");
			if constexpr (!std::same_as<TRet, void>)
			{
				return {};
			}
		}
	}

	const char* GetName() const
	{
		return mName;
	}

	const TypeInfo& GetReturnType() const
	{
		return *mReturnType;
	}

	const TypeInfo& GetParameterType(size_t i) const
	{
		return *mParameterTypes[i];
	}

	size_t NumParameter() const
	{
		return mParameterTypes.size();
	}

private:
	template <typename TRet, typename... Args>
	void collectFunctionSignature()
	{
		mReturnType = &TypeInfo::GetStaticTypeInfo<TRet>();
		mParameterTypes.reserve(sizeof...(Args));

		(mParameterTypes.emplace_back(&TypeInfo::GetStaticTypeInfo<Args>()), ...);
	}

private:
	const TypeInfo* mReturnType = nullptr;
	std::vector<const TypeInfo*> mParameterTypes;
	const char* mName = nullptr;
	const CallableBase& mCallable;
};

template <typename TClass, typename TPtr, TPtr ptr>
class MethodRegister
{
public:
	MethodRegister(const char* name, TypeInfo& typeInfo)
	{
		if constexpr (std::is_member_function_pointer_v<TPtr>)
		{
			static Callable callable(ptr);
			static Method method(typeInfo, ptr, name, callable);
		}
		else
		{
			TClass* forDeduction = nullptr;
			static StaticCallable callable(forDeduction, ptr);
			static Method method(typeInfo, ptr, name, callable);
		}
	}
};
