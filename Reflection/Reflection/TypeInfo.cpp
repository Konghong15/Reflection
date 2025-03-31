#include <iostream>

#include "TypeInfo.h"
#include "Property.h"
#include "Method.h"

void TypeInfo::PrintProperties(int indent) const
{
	std::string indentStr(indent * 4, ' ');

	for (const Property* property : GetProperties())
	{
		std::cout
			<< indentStr
			<< "Type: " << property->GetTypeInfo().GetName()
			<< ", Name: " << property->GetName()
			<< std::endl;

		const TypeInfo& propType = property->GetTypeInfo();
	}
}

void TypeInfo::PrintPropertiesRecursive(int indent) const
{
	std::string indentStr(indent * 4, ' ');

	for (const Property* property : GetProperties())
	{
		std::cout
			<< indentStr
			<< "Type: " << property->GetTypeInfo().GetName()
			<< ", Name: " << property->GetName()
			<< std::endl;

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
