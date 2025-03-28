#pragma once
#include "../Reflection//ReflectionMacros.h"
#include "Demo.generate.h"

class DemoClass {
	GENERATE_CLASS_TYPE_INFO()

		PROPERTY()
		int health = 100;

	METHOD()
		void Jump() {}
};