#include <iostream>

#include "TypeInfo.h"
#include "Property.h"
#include "Method.h"
#include "Procedure.h"

void TypeInfo::PrintTypeInfo(int indent) const
{
	std::string indentStr(indent * 4, ' ');
	std::cout << indentStr << "{\n";

	// TypeName 출력
	std::cout << indentStr << "    \"TypeName\": \"" << GetName() << "\",\n";

	// 재사용된 출력 함수들
	PrintProperties(indent + 1);

	if (!GetMethods().empty())
	{
		std::cout << ",\n";
		PrintMethods(indent + 1);
	}

	if (!GetProcedures().empty())
	{
		std::cout << ",\n";
		PrintProcedures(indent + 1);
	}

	std::cout << "\n" << indentStr << "}\n";
}

void TypeInfo::PrintTypeInfoValues(void* object, int indent) const
{
	std::string indentStr(indent * 4, ' ');
	std::cout << indentStr << "{\n";

	// TypeName 출력
	std::cout << indentStr << "    \"TypeName\": \"" << GetName() << "\",\n";

	// 재사용된 출력 함수들
	PrintPropertyValues(object, indent + 1);

	if (!GetMethods().empty())
	{
		std::cout << ",\n";
		PrintMethods(indent + 1);
	}

	if (!GetProcedures().empty())
	{
		std::cout << ",\n";
		PrintProcedures(indent + 1);
	}

	std::cout << "\n" << indentStr << "}\n";
}

void TypeInfo::PrintPropertyValues(void* object, int indent) const
{
	std::string indentStr(indent * 4, ' ');
	std::cout << indentStr << "\"Properties\": [\n";

	bool first = true;
	for (const Property* property : GetProperties())
	{
		if (!first)
			std::cout << ",\n";
		first = false;

		property->PrintPropertyValue(object, indent + 1);
	}

	std::cout << "\n" << indentStr << "]\n";
}

void TypeInfo::PrintProperties(int indent) const
{
	std::string indentStr(indent * 4, ' ');
	std::cout << indentStr << "\"Properties\": [\n";

	bool first = true;
	for (const Property* property : GetProperties())
	{
		if (!first)
			std::cout << ",\n";
		first = false;

		property->PrintProperty(indent + 1);
	}

	std::cout << "\n" << indentStr << "]\n";
}

void TypeInfo::PrintMethods(int indent) const
{
	std::string indentStr(indent * 4, ' ');
	std::cout << indentStr << "\"Methods\": [\n";

	bool firstMethod = true;
	for (const Method* method : GetMethods())
	{
		if (!firstMethod)
			std::cout << ",\n";
		firstMethod = false;

		std::cout << indentStr << "    {\n";
		std::cout << indentStr << "        \"Name\": \"" << method->GetName() << "\",\n";
		std::cout << indentStr << "        \"Return\": \"" << method->GetReturnType().GetName() << "\"";

		// Parameters
		if (method->NumParameter() > 0)
		{
			std::cout << ",\n" << indentStr << "        \"Params\": [";
			for (size_t i = 0; i < method->NumParameter(); ++i)
			{
				std::cout << "\"" << method->GetParameterType(i).GetName() << "\"";
				if (i + 1 < method->NumParameter())
					std::cout << ", ";
			}
			std::cout << "]";
		}
		else
		{
			std::cout << ",\n" << indentStr << "        \"Params\": []";
		}

		std::cout << "\n" << indentStr << "    }";
	}

	std::cout << "\n" << indentStr << "]\n";
}

void TypeInfo::PrintProcedures(int indent) const
{
	std::string indentStr(indent * 4, ' ');

	std::cout << indentStr << "\"Procedures\": [\n";

	bool firstProc = true;
	for (const Procedure* proc : mProcedures)
	{
		if (!firstProc)
			std::cout << ",\n";
		firstProc = false;

		std::cout << indentStr << "    {\n";
		std::cout << indentStr << "        \"Name\": \"" << proc->GetName() << "\"";

		size_t paramCount = proc->NumParameter();
		std::cout << ",\n" << indentStr << "        \"Params\": [";

		for (size_t i = 0; i < paramCount; ++i)
		{
			std::cout << "\"" << proc->GetParameterType(i).GetName() << "\"";
			if (i + 1 < paramCount)
				std::cout << ", ";
		}

		std::cout << "]\n";
		std::cout << indentStr << "    }";
	}

	std::cout << "\n" << indentStr << "]\n";
}

void TypeInfo::addMethod(const Method* method)
{
	mMethods.emplace_back(method);
	mMethodMap.emplace(method->GetName(), method);
}
void TypeInfo::addProperty(const Property* property)
{
	mProperties.emplace_back(property);
	mPropertyMap.emplace(property->GetName(), property);
}
void TypeInfo::addProcedure(const Procedure* procedure)
{
	mProcedures.push_back(procedure);
	mProcedureMap[procedure->GetName()] = procedure;
}

void TypeInfo::collectSuperMethods()
{
	assert(mSuper != nullptr);
	const std::vector<const Method*>& methods = mSuper->GetMethods();
	for (const Method* method : methods)
	{
		addMethod(method);
	}
}
void TypeInfo::collectSuperProperties()
{
	assert(mSuper != nullptr);
	const std::vector<const Property*>& properties = mSuper->GetProperties();
	for (const Property* property : properties)
	{
		addProperty(property);
	}
}
void TypeInfo::collectSuperProcedures()
{
	if (!mSuper) return;

	for (const Procedure* proc : mSuper->mProcedures)
	{
		mProcedures.push_back(proc);
		mProcedureMap[proc->GetName()] = proc;
	}
}
