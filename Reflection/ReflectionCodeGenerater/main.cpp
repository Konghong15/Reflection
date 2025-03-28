
#include "ReflectionGenerator.h"

int main(void)
{
	ReflectionGenerator gen;
	gen.Parse("../Reflection/GameObject.h");
	gen.WriteGeneratedHeader("../Reflection/GameObject.generate.h");
}