#include <iostream>

#include "TypeInfo.h"
#include "Property.h"
#include "Method.h"
#include "Procedure.h"

void TypeInfo::PrintPropertyValues(void* object, int indent) const
{
	for (const Property* property : GetProperties())
	{
		property->PrintPropertyValue(object, indent);
	}
}
void TypeInfo::PrintPropertyValuesRecursive(void* object, int indent) const
{
	for (const Property* property : GetProperties())
	{
		const TypeInfo& propType = property->GetTypeInfo();

		if (!propType.GetProperties().empty())
		{
			void* memberPtr = property->GetRawPointer(object);
			property->PrintProperty(indent);
			propType.PrintPropertyValuesRecursive(memberPtr, indent + 1);
		}
		else
		{
			property->PrintPropertyValue(object, indent);
		}
	}
}

void TypeInfo::PrintProperties(int indent) const
{
	for (const Property* property : GetProperties())
	{
		property->PrintProperty(indent);
	}
}
void TypeInfo::PrintPropertiesRecursive(int indent) const
{
	for (const Property* property : GetProperties())
	{
		property->PrintProperty(indent);
		
		const TypeInfo& propType = property->GetTypeInfo();

		if (!propType.GetProperties().empty())
		{
			propType.PrintPropertiesRecursive(indent + 1); // ¿Á±Õ »£√‚
		}
	}
}

void TypeInfo::PrintMethods(int indent) const
{
	std::string indentStr(indent * 4, ' ');

	for (const Method* method : GetMethods())
	{
		std::cout
			<< indentStr
			<< "Method: " << method->GetName()
			<< " -> Return: " << method->GetReturnType().GetName();

		if (method->NumParameter() > 0)
		{
			std::cout << ", Params: (";
			for (size_t i = 0; i < method->NumParameter(); ++i)
			{
				std::cout << method->GetParameterType(i).GetName();
				if (i + 1 < method->NumParameter())
					std::cout << ", ";
			}
			std::cout << ")";
		}
		std::cout << std::endl;
	}
}

void TypeInfo::PrintProcedures(int indent) const
{
	std::string indentStr(indent * 4, ' ');

	for (const Procedure* proc : mProcedures)
	{
		std::cout
			<< indentStr
			<< "Procedure: " << proc->GetName();

		size_t paramCount = proc->NumParameter();
		if (paramCount > 0)
		{
			std::cout << " -> Params: (";
			for (size_t i = 0; i < paramCount; ++i)
			{
				std::cout << proc->GetParameterType(i).GetName();
				if (i + 1 < paramCount)
					std::cout << ", ";
			}
			std::cout << ")";
		}

		std::cout << std::endl;
	}
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
