#include <cassert>
#include <iostream>

#include "TypeInfo.h"
#include "Property.h"
#include "Method.h"

template <typename To, typename From>
To* Cast(From* src)
{
	if (src == nullptr)
	{
		return nullptr;
	}
	if (!src->GetTypeInfo().IsChildOf<To>())
	{
		return nullptr;
	}

	return reinterpret_cast<To*>(src);
}

bool FloatEqual(float a, float b, float epsilon = 1e-6f) {
	return std::fabs(a - b) < epsilon;
}

class Component
{
	GENERATE_CLASS_TYPE_INFO(Component)
public:
	virtual ~Component() {}

private:
};

class AComponent : public Component
{
	GENERATE_CLASS_TYPE_INFO(AComponent)
public:

private:


};

class BComponent : public Component
{
	GENERATE_CLASS_TYPE_INFO(BComponent)
public:

private:
};

struct Vector3
{
	GENERATE_CLASS_TYPE_INFO(Vector3)
		PROPERTY(x)
		PROPERTY(y)
		PROPERTY(z)

public:
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	Vector3() = default;
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
};

class TransformComponent : public Component
{
	GENERATE_CLASS_TYPE_INFO(TransformComponent)

public:
	TransformComponent() = default;
	TransformComponent(const Vector3& position) : mPosition(position) {}

	METHOD(GetPosition)
		const Vector3& GetPosition() { return mPosition; }

	METHOD(SetPositionVec3)
		void SetPositionVec3(const Vector3& position) { mPosition = position; }

	METHOD(SetPositionXYZ)
		void SetPositionXYZ(float x, float y, float z) { mPosition = Vector3(x, y, z); }

private:
	PROPERTY(mPosition)
		Vector3 mPosition;
};

class GameObject
{
public:
	void AddComponent(Component* comp)
	{
		mComponents.push_back(comp);
	}


	template <typename T>
	T* GetComponentOrNull()
	{
		for (Component* comp : mComponents)
		{
			if (auto* concrete = Cast<T>(comp))
			{
				return concrete;
			}
		}

		return nullptr;
	}

	template <typename T>
	bool TryGetComponent(T** outComp)
	{
		for (Component* comp : mComponents)
		{
			if (auto* concrete = Cast<T>(comp))
			{
				*outComp = concrete;
				return true;
			}
		}

		return false;
	}

private:
	std::vector<Component*> mComponents;
};


void TestMethod(void);
void TestTypeInfo(void);
void TestProperty(void);

int main()
{
	TestTypeInfo();
	TestProperty();
	TestMethod();

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
		const Vector3 INIT_POS = { 0, 20, 30 };
		const Vector3 TEST_POS = { 100, 200, 300 };

		GameObject gameObject;
		TransformComponent transform(INIT_POS);
		gameObject.AddComponent(&transform);
		Component* component = gameObject.GetComponentOrNull<TransformComponent>();
		assert(component != nullptr);

		const TypeInfo& typeInfo = component->GetTypeInfo();
		const Property* posProperty = typeInfo.GetProperty("mPosition");

		if (posProperty != nullptr)
		{
			const Vector3& pos = posProperty->Get<TransformComponent, Vector3>(&transform);
			// const Vector3& pos = posProperty->Get<Vector3>(&transform);

			assert(FloatEqual(pos.x, INIT_POS.x));
			assert(FloatEqual(pos.y, INIT_POS.y));
			assert(FloatEqual(pos.z, INIT_POS.z));

			posProperty->Set(component, TEST_POS);
			assert(FloatEqual(pos.x, TEST_POS.x));
			assert(FloatEqual(pos.y, TEST_POS.y));
			assert(FloatEqual(pos.z, TEST_POS.z));
		}
	}
}


void TestMethod(void)
{
	{
		const Vector3 INIT_POS = { 0, 20, 30 };
		const Vector3 TEST_POS_1 = { 100, 200, 300 };
		const Vector3 TEST_POS_2 = { 300, 400, 500 };

		GameObject gameObject;
		TransformComponent transform(INIT_POS);
		gameObject.AddComponent(&transform);
		Component* component = gameObject.GetComponentOrNull<TransformComponent>();
		assert(component != nullptr);

		const TypeInfo& typeInfo = component->GetTypeInfo();
		const Method* getPosition = typeInfo.GetMethod("GetPosition");

		assert(getPosition != nullptr);
		if (getPosition != nullptr)
		{
			const Vector3& pos = getPosition->Invoke<const Vector3&>(component);
			assert(FloatEqual(pos.x, INIT_POS.x));
			assert(FloatEqual(pos.y, INIT_POS.y));
			assert(FloatEqual(pos.z, INIT_POS.z));
		}

		const Method* setPositionVec3 = typeInfo.GetMethod("SetPositionVec3");

		assert(setPositionVec3 != nullptr);
		if (setPositionVec3 != nullptr)
		{
			setPositionVec3->Invoke<void>(component, TEST_POS_1);
			const Vector3& pos = transform.GetPosition();
			assert(FloatEqual(pos.x, TEST_POS_1.x));
			assert(FloatEqual(pos.y, TEST_POS_1.y));
			assert(FloatEqual(pos.z, TEST_POS_1.z));
		}

		const Method* setPositionXYZ = typeInfo.GetMethod("SetPositionXYZ");

		assert(setPositionXYZ != nullptr);
		if (setPositionXYZ != nullptr)
		{
			setPositionXYZ->Invoke<void>(component, 10.f, 20.f, 30.f);
			const Vector3& pos = transform.GetPosition();
			assert(FloatEqual(pos.x, 10.f));
			assert(FloatEqual(pos.y, 20.f));
			assert(FloatEqual(pos.z, 30.f));
		}
	}
}