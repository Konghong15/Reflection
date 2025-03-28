#include "DemoClass.h"

#include <iostream>

int main() {
	const TypeInfo& type = DemoClass::StaticTypeInfo();

	std::cout << "Class: " << type.GetName() << std::endl;

	for (const Property* prop : type.GetProperties()) {
		std::cout << "Property: " << prop->GetName() << std::endl;
	}

	for (const Method* method : type.GetMethods()) {
		std::cout << "Method: " << method->GetName() << std::endl;
	}

	return 0;
}