#pragma once

#include <cassert>
#include <type_traits>

#include "TypeInfo.h"

#define METHOD( Name )	\
	inline static struct RegistMethodExecutor_##Name	\
	{	\
		RegistMethodExecutor_##Name()	\
		{	\
			static MethodRegister<ThisType, decltype(&ThisType::Name), &ThisType::Name> method_register_##Name{ #Name, ThisType::StaticTypeInfo() };	\
		}	\
		\
	} regist_##Name;

// non template class���� ������ ���� ���̽� Ŭ����
class CallableBase
{
	GENERATE_TYPE_INFO(CallableBase)

public:
	virtual ~CallableBase() = default;
};

// ������ �����ϰ� ���ø� �Ű������� ����ȯ�� ���� ���ø� �������̽� Ŭ����
template <typename TRet, typename... TArgs>
class ICallable : public CallableBase
{
	GENERATE_TYPE_INFO(ICallable)

public:
	virtual TRet Invoke(void* caller, TArgs&&... args) const = 0;
};

template <typename TClass, typename TRet, typename... TArgs>
class Callable : public ICallable<TRet, TArgs...>
{
	GENERATE_TYPE_INFO(Callable)
		using FuncPtr = TRet(TClass::*)(TArgs...);

public:
	Callable(FuncPtr ptr)
		: mPtr(ptr)
	{
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
class ConstCallable : public ICallable<TRet, TArgs...>
{
	using FuncPtr = TRet(TClass::*)(TArgs...) const;
public:
	ConstCallable(FuncPtr ptr)
		: mPtr(ptr)
	{
	}

	virtual TRet Invoke(void* caller, TArgs&&... args) const override 
	{
		if constexpr (std::same_as<TRet, void>)
		{
			(static_cast<const TClass*>(caller)->*mPtr)(std::forward<TArgs>(args)...);
		}
		else
		{
			return (static_cast<const TClass*>(caller)->*mPtr)(std::forward<TArgs>(args)...);
		}
	}

private:
	FuncPtr mPtr = nullptr;
};

template <typename TClass, typename TRet, typename... TArgs>
class StaticCallable : public ICallable<TRet, TArgs...>
{
	GENERATE_TYPE_INFO(StaticCallable)
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
	// �Ϲ� �Լ� ������
	template <typename TRet, typename... TArgs>
	Method(TypeInfo& owner, [[maybe_unused]] TRet(*ptr)(TArgs...), const char* name, const CallableBase& callable) :
		mName(name),
		mCallable(callable)
	{
		collectFunctionSignature<TRet, TArgs...>();
		owner.addMethod(this);
	}

	// �ɹ� �Լ� non-const
	template <typename TClass, typename TRet, typename... TArgs>
	Method(TypeInfo& owner, [[maybe_unused]] TRet(TClass::* ptr)(TArgs...), const char* name, const CallableBase& callable) :
		mName(name),
		mCallable(callable)
	{
		collectFunctionSignature<TRet, TArgs...>();
		owner.addMethod(this);
	}

	// �ɹ� �Լ� const
	template <typename TClass, typename TRet, typename... TArgs>
	Method(TypeInfo& owner, [[maybe_unused]] TRet(TClass::* ptr)(TArgs...) const, const char* name, const CallableBase& callable) :
		mName(name),
		mCallable(callable)
	{
		collectFunctionSignature<TRet, TArgs...>();
		owner.addMethod(this);
	}

	template <typename TRet, typename... TArgs>
	TRet Invoke(void* caller, TArgs&&... args) const
	{
		// 1. ��ȯ Ÿ�� �˻�
		const TypeInfo& returnType = TypeInfo::GetStaticTypeInfo<TRet>();

		if (!returnType.IsChildOf(*mReturnType))
		{
			std::cerr << "[Method::Invoke] Return type mismatch: expected "
				<< mReturnType->GetName() << ", got " << returnType.GetName() << std::endl;

			assert(false && "Method::Invoke - Return type mismatch");
			
			if constexpr (!std::same_as<TRet, void>)
			{
				return {};
			}
			else
			{
				return;
			}
		}

		// 2. �Ű����� �˻�
		std::vector<const TypeInfo*> callTypes = { &TypeInfo::GetStaticTypeInfo<TArgs>()... };

		if (callTypes.size() != mParameterTypes.size())
		{
			std::cerr << "[Method::Invoke] Argument count mismatch: expected "
				<< mParameterTypes.size() << ", got " << callTypes.size() << std::endl;

			assert(false && "Method::Invoke - Argument count mismatch");
			if constexpr (!std::same_as<TRet, void>)
				return {};
			else
				return;
		}
		for (size_t i = 0; i < callTypes.size(); ++i)
		{
			if (!callTypes[i]->IsChildOf(*mParameterTypes[i]))
			{
				std::cerr << "[Method::Invoke] Type mismatch at arg #" << i
					<< ": expected " << mParameterTypes[i]->GetName()
					<< ", got " << callTypes[i]->GetName() << std::endl;

				assert(false && "Method::Invoke - Argument type mismatch");
				if constexpr (!std::same_as<TRet, void>)
					return {};
				else
					return;
			}
		}

		// 3. �Լ� ȣ��
		auto* concrete = static_cast<const ICallable<TRet, TArgs...>*>(&mCallable);
		if constexpr (std::same_as<TRet, void>)
		{
			concrete->Invoke(caller, std::forward<TArgs>(args)...);
		}
		else
		{
			return concrete->Invoke(caller, std::forward<TArgs>(args)...);
		}
	}
	
	inline const char* GetName() const;
	inline const TypeInfo& GetReturnType() const;
	inline const std::vector<const TypeInfo*>& GetParameterTypes() const;
	inline const TypeInfo& GetParameterType(size_t i) const;
	inline size_t NumParameter() const;

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

inline const char* Method::GetName() const
{
	return mName;
}

inline const TypeInfo& Method::GetReturnType() const
{
	return *mReturnType;
}

inline const std::vector<const TypeInfo*>& Method::GetParameterTypes() const
{
	return mParameterTypes;
}

inline const TypeInfo& Method::GetParameterType(size_t i) const
{
	return *mParameterTypes[i];
}

inline size_t Method::NumParameter() const
{
	return mParameterTypes.size();
}

template <typename T>
struct is_const_member_function : std::false_type {};

template <typename Ret, typename Class, typename... Args>
struct is_const_member_function<Ret(Class::*)(Args...) const> : std::true_type {};

template <typename TClass, typename TPtr, TPtr ptr>
class MethodRegister
{
public:
	MethodRegister(const char* name, TypeInfo& typeInfo)
	{
		if constexpr (std::is_member_function_pointer_v<TPtr>)
		{
			if constexpr (is_const_member_function<TPtr>::value)
			{
				static ConstCallable callable(ptr);
				static Method method(typeInfo, ptr, name, callable);
			}
			else
			{
				static Callable callable(ptr);
				static Method method(typeInfo, ptr, name, callable);
			}
		}
		else
		{
			TClass* forDeduction = nullptr;
			static StaticCallable callable(forDeduction, ptr);
			static Method method(typeInfo, ptr, name, callable);
		}
	}
};
