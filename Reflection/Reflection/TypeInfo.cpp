#include "TypeInfo.h"
#include "Property.h"
#include "Method.h"

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
