#include "Reflect.h"

namespace reflect {
	struct TypeDescriptor_Int : TypeDescriptor {
		TypeDescriptor_Int() : TypeDescriptor{ "int", sizeof(int) }
		{
		}
		virtual void Dump(const void* obj, int /* unused */) const override
		{
			std::cout << "int{" << *(const int*)obj << "}";
		}
	};

	// 전체 템플릿 특수화로 프리미티브 정의
	template <>
	TypeDescriptor* TypeResolver<int>::Get()
	{
		static TypeDescriptor_Int typeDesc;
		return &typeDesc;
	}

	struct TypeDescriptor_StdString : TypeDescriptor {
		TypeDescriptor_StdString() : TypeDescriptor{ "std::string", sizeof(std::string) }
		{
		}
		virtual void Dump(const void* obj, int /* unused */) const override
		{
			std::cout << "std::string{\"" << *(const std::string*)obj << "\"}";
		}
	};
	template <>
	TypeDescriptor* TypeResolver<std::string>::Get()
	{
		static TypeDescriptor_StdString typeDesc;
		return &typeDesc;
	}
} 
