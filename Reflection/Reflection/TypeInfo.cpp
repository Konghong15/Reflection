#include "TypeInfo.h"
#include "Property.h"

void TypeInfo::addMethod(const Method* method)
{
}
void TypeInfo::addProperty(const Property* property)
{
	mProperties.emplace_back(property);
	mPropertyMap.emplace(property->GetName(), property);
}
void TypeInfo::collectSuperMethods()
{
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
