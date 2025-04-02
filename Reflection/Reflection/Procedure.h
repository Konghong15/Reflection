#pragma once

#include <cassert>
#include <type_traits>

#include "TypeInfo.h"
#include "Method.h" 

#define PROCEDURE(Name) \
	inline static struct RegisterProcedureExecutor_##Name \
	{ \
		RegisterProcedureExecutor_##Name() \
		{ \
			static ProcedureRegister<ThisType, decltype(&ThisType::Name), &ThisType::Name> procedure_register_##Name{ #Name, ThisType::StaticTypeInfo() }; \
		} \
	} regist_proc_##Name;

class Procedure
{
public:
	template <typename TRet, typename... TArgs>
	Procedure(TypeInfo& owner, [[maybe_unused]] TRet(*ptr)(TArgs...), const char* name, const CallableBase& callable)
		: mName(name), mCallable(callable)
	{
		static_assert(std::is_same_v<TRet, void>, "Procedure must return void");
		collectFunctionSignature<TArgs...>();
		owner.addProcedure(this);
	}

	template <typename TClass, typename... TArgs>
	Procedure(TypeInfo& owner, [[maybe_unused]] void(TClass::* ptr)(TArgs...), const char* name, const CallableBase& callable)
		: mName(name), mCallable(callable)
	{
		collectFunctionSignature<TArgs...>();
		owner.addProcedure(this);
	}

	template <typename TClass, typename... TArgs>
	Procedure(TypeInfo& owner, [[maybe_unused]] void(TClass::* ptr)(TArgs...) const, const char* name, const CallableBase& callable)
		: mName(name), mCallable(callable)
	{
		collectFunctionSignature<TArgs...>();
		owner.addProcedure(this);
	}

	template <typename... TArgs>
	void Invoke(void* caller, TArgs&&... args) const
	{
		std::vector<const TypeInfo*> callTypes = { &TypeInfo::GetStaticTypeInfo<TArgs>()... };

		if (callTypes.size() != mParameterTypes.size())
		{
			assert(false && "Procedure::Invoke - Argument count mismatch");
			return;
		}
		for (size_t i = 0; i < callTypes.size(); ++i)
		{
			if (!callTypes[i]->IsChildOf(*mParameterTypes[i]))
			{
				std::cerr << "[Procedure::Invoke] Type mismatch at arg #" << i
					<< ": expected " << mParameterTypes[i]->GetName()
					<< ", got " << callTypes[i]->GetName() << std::endl;

				assert(false && "Procedure::Invoke - Argument type mismatch");
				return;
			}
		}

		auto concreteCallable = static_cast<const ICallable<void, TArgs...>*>(&mCallable);
		concreteCallable->Invoke(caller, std::forward<TArgs>(args)...);
	}

	const char* GetName() const
	{
		return mName;
	}

	const std::vector<const TypeInfo*>& GetParameterTypes() const
	{
		return mParameterTypes;
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
	template <typename... Args>
	void collectFunctionSignature()
	{
		mParameterTypes.reserve(sizeof...(Args));
		(mParameterTypes.emplace_back(&TypeInfo::GetStaticTypeInfo<Args>()), ...);
	}

private:
	std::vector<const TypeInfo*> mParameterTypes;
	const char* mName;
	const CallableBase& mCallable;
};

template <typename TClass, typename TPtr, TPtr Ptr>
class ProcedureRegister
{
public:
	ProcedureRegister(const char* name, TypeInfo& typeInfo)
	{
		if constexpr (std::is_member_function_pointer_v<TPtr>)
		{
			if constexpr (is_const_member_function<TPtr>::value)
			{
				static ConstCallable callable(Ptr);
				static Procedure procedure(typeInfo, Ptr, name, callable);
			}
			else
			{
				static Callable callable(Ptr);
				static Procedure procedure(typeInfo, Ptr, name, callable);
			}
		}
		else
		{
			TClass* forDeduction = nullptr;
			static StaticCallable callable(forDeduction, Ptr);
			static Procedure procedure(typeInfo, Ptr, name, callable);
		}
	}
};
