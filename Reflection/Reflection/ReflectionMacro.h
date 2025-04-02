#pragma once

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

#define PROPERTY(Name) \
	inline static struct RegistPropertyExecutor_##Name \
	{ \
		RegistPropertyExecutor_##Name() \
		{ \
			static PropertyRegister<ThisType, decltype(Name), decltype(&ThisType::Name), &ThisType::##Name> property_register_##Name{ #Name, ThisType::StaticTypeInfo() }; \
		} \
	} regist_##Name; \

#define METHOD( Name )	\
	inline static struct RegistMethodExecutor_##Name	\
	{	\
		RegistMethodExecutor_##Name()	\
		{	\
			static MethodRegister<ThisType, decltype(&ThisType::Name), &ThisType::Name> method_register_##Name{ #Name, ThisType::StaticTypeInfo() };	\
		}	\
		\
	} regist_##Name;

