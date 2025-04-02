#include <cassert>
#include <iostream>
#include <vector>

#ifdef _DEBUG
#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC
#endif

#include "FixedVector.h"
#include "GCManager.h"
#include "GCObject.h"
#include "GCUtility.h"
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
	GENERATE_TYPE_INFO(Component)
public:
	virtual ~Component() {}

private:
};

class AComponent : public Component
{
	GENERATE_TYPE_INFO(AComponent)
public:

private:


};

class BComponent : public Component
{
	GENERATE_TYPE_INFO(BComponent)
public:

private:
};

struct Vector3
{
	GENERATE_TYPE_INFO(Vector3)
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
	GENERATE_TYPE_INFO(TransformComponent)

public:
	TransformComponent() = default;
	TransformComponent(const Vector3& position) : mPosition(position), mNum(20.123f) {}

	METHOD(GetPosition)
		const Vector3& GetPosition() const { return mPosition; }

	METHOD(SetPositionVec3)
		void SetPositionVec3(const Vector3& position) { mPosition = position; }

	METHOD(SetPositionXYZ)
		void SetPositionXYZ(float x, float y, float z) { mPosition = Vector3(x, y, z); }

private:
	PROPERTY(mPosition)
		Vector3 mPosition;

	PROPERTY(mScale)
		Vector3 mScale;

	PROPERTY(mRotation)
		Vector3 mRotation;

	PROPERTY(mNums)
		std::vector<int> mNums;

	PROPERTY(mNum)
		float mNum;
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

class TempObject : public GCObject
{
	GENERATE_TYPE_INFO(TempObject)
public:

private:
};

class GameInstance : public GCObject
{
	GENERATE_TYPE_INFO(GameInstance)

public:
	void CreateTenThousandObjects()
	{
		for (int i = 0; i < OBJECT_COUNT; ++i)
		{
			mGCObjects.Add(NewGCObject<TempObject>(GCManager::Get()));
		}
	}

	void ReleaseTenThousandObjects()
	{
		const size_t SIZE = mGCObjects.GetSize();

		for (int i = static_cast<int>(SIZE) - 1; i >= 0; --i)
		{
			mGCObjects[i] = nullptr;
			mGCObjects.RemoveLast();
		}
	}

private:
	enum { OBJECT_COUNT = 10000 };
	PROPERTY(mGCObjects)
		FixedVector<GCObject*, OBJECT_COUNT> mGCObjects;
};

void TestMethod(void);
void TestTypeInfo(void);
void TestProperty(void);
void PrintFunction(void);
void PrintProperty(void);
void CollectGarbage(void);

int main()
{
	GCManager::Create();

	TestTypeInfo();
	TestProperty();
	TestMethod();
	PrintProperty();
	PrintFunction();
	CollectGarbage();

	GCManager::Destroy();

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
	{
		FixedVector<int, 10> nums;

		for (size_t i = 0; i < 10; ++i)
		{
			nums.Add(i);
		}

		const auto& typeInfo = nums.GetTypeInfo();
		for (const auto& property : typeInfo.GetProperties())
		{
			property->Print(&nums, 0);
		}

		const auto numArray = typeInfo.GetProperty("mElements");

		int& num3 = numArray->Get<int>(&nums, 3);

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
	{
		const int num = 20;
		FixedVector<int, 10> fixedVector;

		const auto& typeInfo = fixedVector.GetTypeInfo();
		const Method* push_back = typeInfo.GetMethod("push_back");

		if (push_back != nullptr)
		{
			push_back->Invoke<void>(&fixedVector, num);
		}
	}
}

void PrintProperty(void)
{
	{
		const Vector3 INIT_POS = { 0, 20, 30 };
		TransformComponent transform(INIT_POS);
		const TypeInfo& typeInfo = transform.GetTypeInfo();

		typeInfo.PrintProperties();
		typeInfo.PrintPropertiesRecursive();
		typeInfo.PrintObject(&transform);
	}
}

void PrintFunction(void)
{
	{
		const Vector3 INIT_POS = { 0, 20, 30 };
		TransformComponent transform(INIT_POS);

		const TypeInfo& typeInfo = transform.GetTypeInfo();
		typeInfo.PrintMethods();
	}
}

void CollectGarbage(void)
{

	{
		GameInstance* gameInstance = NewGCObject<GameInstance>(GCManager::Get());
		gameInstance->SetRoot(true);

#ifdef _DEBUG
		// 메모리 릭 감지 시작
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
		const TypeInfo& typeInfo = gameInstance->GetTypeInfo();

		gameInstance->CreateTenThousandObjects();
		GCManager::Get().Collect();
		gameInstance->ReleaseTenThousandObjects();
		GCManager::Get().Collect();
	}
}