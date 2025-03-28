#include <type_traits>
#include <cassert>
#include <iostream>

#include "GameObject.h"
#include "ReflectionMacros.h"

void TestTypeInfo(void);
void TestProperty(void);

int main()
{
	TestTypeInfo();
	TestProperty();

	return 0;
}

void TestTypeInfo(void)
{
	{
		GameObject gameObject;
		AComponent aComp;
		BComponent bComp;

		gameObject.AddComponent(&aComp);
		gameObject.AddComponent(&bComp);

		AComponent* aCompPtr = nullptr;
		BComponent* bCompPtr = nullptr;
		assert(gameObject.TryGetComponent(&aCompPtr));
		assert(gameObject.TryGetComponent(&bCompPtr));
		assert(&aComp == aCompPtr);
		assert(&bComp == bCompPtr);
	}

	{
		GameObject gameObject;
		AComponent aComp;

		gameObject.AddComponent(&aComp);

		AComponent* aCompPtr = nullptr;
		BComponent* bCompPtr = nullptr;
		assert(gameObject.TryGetComponent(&aCompPtr));
		assert(gameObject.TryGetComponent(&bCompPtr) == false);
		assert(&aComp == aCompPtr);
		assert(bCompPtr == nullptr);
	}
}

void TestProperty(void)
{
	{
		GameObject gameObject;
		TransformComponent transform({ 0, 20, 30 });

		gameObject.AddComponent(&transform);

		Component* component = gameObject.GetComponentOrNull<TransformComponent>();
		assert(component != nullptr);

		const TypeInfo& typeInfo = component->GetTypeInfo();
		const Property* posProperty = typeInfo.GetProperty("mPosition");

		if (posProperty != nullptr)
		{
			posProperty->Get<TransformComponent, int>(&transform);
			const Vector3& pos = posProperty->Get<TransformComponent, Vector3>(&transform);
			//const Vector3& pos = posProperty->Get<Vector3>(&transform);
			std::cout
				<< "pos : { " << pos.x
				<< " , " << pos.y
				<< " , " << pos.z
				<< " } " << std::endl;

			posProperty->Set(component, Vector3{ 100, 200, 300 });

			std::cout
				<< "pos : { " << pos.x
				<< " , " << pos.y
				<< " , " << pos.z
				<< " } " << std::endl;
		}

		static struct Temp
		{
			int a;
			int b;
		} temp;

		temp.a = 20;
	}
}
