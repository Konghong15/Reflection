#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cassert>
#include <iostream>
#include <vector>
#include <string>

#include "FixedVector.h"
#include "GCManager.h"
#include "GCUtility.h"
#include "GCObject.h"
#include "TypeInfo.h"
#include "Property.h"
#include "Method.h"
#include "Procedure.h"

void TestTypeInfo(void);
void TestProperty(void);
void TestMethod(void);
void TestCG(void);
void TestRPC(void);

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	GCManager::Create();

	TestTypeInfo();
	TestProperty();
	TestMethod();
	TestCG();
	TestRPC();

	GCManager::Destroy();
	return 0;
}

bool FloatEqual(float a, float b, float epsilon = 1e-6f) {

	return std::fabs(a - b) < epsilon;
}

class Animal
{
	GENERATE_TYPE_INFO(Animal)

public:

};

class Cat : public Animal
{
	GENERATE_TYPE_INFO(Cat)
};

void TestTypeInfo(void)
{
	// 상속 관계 체크
	Cat cat;
	Animal* animalPtr = &cat;

	assert(animalPtr->GetTypeInfo().IsChildOf<Cat>());
	assert(animalPtr->GetTypeInfo().IsChildOf(Cat::StaticTypeInfo()));

	assert(animalPtr->GetTypeInfo().IsA <Cat>());
	assert(animalPtr->GetTypeInfo().IsA(Cat::StaticTypeInfo()));
}

struct Vector2
{
	GENERATE_TYPE_INFO(Vector2)
		PROPERTY(x)
		PROPERTY(y)
public:
	Vector2() = default;
	Vector2(float inX, float inY) : x(inX), y(inY) {}

public:
	float x;
	float y;

	friend std::ostream& operator<<(std::ostream& os, const Vector2& vec)
	{
		os << "Vector2(" << vec.x << ", " << vec.y << ")";
		return os;
	}
};

void TestProperty(void)
{
	const float INIT_X = 10.f;
	const float INIT_Y = 20.f;
	const float SET_X = 30.f;
	const float SET_Y = 40.f;

	Vector2 vec2;
	vec2.x = INIT_X;
	vec2.y = INIT_Y;

	assert(vec2.GetTypeInfo().GetProperties().size() == 2);

	const Property* propX = vec2.GetTypeInfo().GetProperty("x");
	assert(propX != nullptr);
	assert(FloatEqual(propX->Get<float>(&vec2), INIT_X));

	const Property* propY = Vector2::StaticTypeInfo().GetProperty("y");
	assert(FloatEqual(propY->Get<float>(&vec2), INIT_Y));

	propX->Set(&vec2, SET_X);
	assert(FloatEqual(propX->Get<float>(&vec2), SET_X));
	propY->Set(&vec2, SET_Y);
	assert(FloatEqual(propY->Get<float>(&vec2), SET_Y));

	vec2.GetTypeInfo().PrintObject(&vec2);
}

class Person
{
	GENERATE_TYPE_INFO(Person)
public:
	Person(const std::string& name)
		: mName(name)
	{
	}

	METHOD(GetName)
		const std::string& GetName() const
	{
		return mName;
	}

	METHOD(SetName)
		void SetName(const std::string& name)
	{
		mName = name;
	}

private:
	std::string mName;
};

void TestMethod(void)
{
	const std::string INIT_NAME = "Hong";
	const std::string SET_NAME = "Kong";

	Person person(INIT_NAME);

	assert(person.GetTypeInfo().GetMethods().size() == 2);

	const Method* getter = person.GetTypeInfo().GetMethod("GetName");
	assert(getter != nullptr);
	const std::string& name = getter->Invoke<const std::string&>(&person);
	assert(name == INIT_NAME);

	const Method* setter = Person::StaticTypeInfo().GetMethod("SetName");
	assert(setter != nullptr);
	setter->Invoke<void>(&person, SET_NAME);
	assert(person.GetName() == SET_NAME);

	Person::StaticTypeInfo().PrintMethods();
}

class TempObject : public GCObject
{
	GENERATE_TYPE_INFO(TempObject)
public:
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


void TestCG(void)
{
	const size_t TEST_INSTANCE_COUNT = 10;
	GameInstance* gameInstances[TEST_INSTANCE_COUNT];
	const GCDebugInfo& lastInfo = GCManager::Get().GetLastDebugInfo();

	// 1. 아무 처리되지 않는 것 확인
	for (size_t i = 0; i < TEST_INSTANCE_COUNT; ++i)
	{
		gameInstances[i] = NewGCObject<GameInstance>(GCManager::Get());
		gameInstances[i]->SetRoot(true);
		gameInstances[i]->CreateTenThousandObjects();
	}

	GCManager::Get().Collect();
	assert(lastInfo.RootObjectCount == 10);
	assert(lastInfo.DeletedObjects == 0);

	// 2. 10만개 오브젝트 GC 처리
	for (size_t i = 0; i < TEST_INSTANCE_COUNT; ++i)
	{
		gameInstances[i]->ReleaseTenThousandObjects();
	}

	GCManager::Get().Collect();
	assert(lastInfo.RootObjectCount == 10);
	assert(lastInfo.DeletedObjects == 100000);

	// 3. 멀티 스레드 테스트
	for (size_t i = 0; i < TEST_INSTANCE_COUNT; ++i)
	{
		gameInstances[i]->CreateTenThousandObjects();
		gameInstances[i]->ReleaseTenThousandObjects();
	}

	GCManager::Get().CollectMultiThread();
	assert(lastInfo.RootObjectCount == 10);
	assert(lastInfo.DeletedObjects == 100000);

	// 4. 루트 제거 테스트
	for (size_t i = 0; i < TEST_INSTANCE_COUNT; ++i)
	{
		gameInstances[i]->SetRoot(false);
	}

	GCManager::Get().Collect();
	assert(lastInfo.RootObjectCount == 0);
	assert(lastInfo.DeletedObjects == 10);
}

class MoveablePerson : public Person
{
	GENERATE_TYPE_INFO(MoveablePerson)
public:
	MoveablePerson(const std::string& name)
		: Person(name)
		, mPos(0, 0)
	{
	}

	PROCEDURE(MoveXYStatic)
		static void MoveXYStatic(float x, float y)
	{
		mPosStatic.x = x;
		mPosStatic.y = y;
	}

	PROCEDURE(MoveVec)
		void MoveVec(const Vector2& pos)
	{
		mPos = pos;
	}

	PROCEDURE(MoveXY)
		void MoveXY(float x, float y)
	{
		mPos.x = x;
		mPos.y = y;
	}

	const Vector2& GetPos() const
	{
		return mPos;
	}

	PROPERTY(mPosStatic)
		static Vector2 mPosStatic;
private:
	PROPERTY(mPos)
		Vector2 mPos;
};

Vector2 MoveablePerson::mPosStatic;

void TestRPC(void)
{
	const Vector2 MOVE_VEC = { 50, 70 };
	const float MOVE_X = 100.f;
	const float MOVE_Y = 50.f;

	Vector2::StaticTypeInfo().IsA(MOVE_VEC.GetTypeInfo());

	MoveablePerson moveablePerson("Hong");
	assert(moveablePerson.GetTypeInfo().GetProcedures().size() == 3);

	const Procedure* moveXY = moveablePerson.GetTypeInfo().GetProcedure("MoveXY");
	assert(moveXY != nullptr);
	moveXY->Invoke(&moveablePerson, MOVE_X, MOVE_Y);
	assert(FloatEqual(moveablePerson.GetPos().x, MOVE_X) && FloatEqual(moveablePerson.GetPos().y, MOVE_Y));

	const Procedure* moveVec = MoveablePerson::StaticTypeInfo().GetProcedure("MoveVec");
	assert(moveVec != nullptr);
	moveVec->Invoke(&moveablePerson, MOVE_VEC);
	assert(FloatEqual(moveablePerson.GetPos().x, MOVE_VEC.x) && FloatEqual(moveablePerson.GetPos().y, MOVE_VEC.y));

	const Procedure* moveXYStatic = MoveablePerson::StaticTypeInfo().GetProcedure("MoveXYStatic");
	assert(moveXYStatic != nullptr);
	moveXYStatic->Invoke(nullptr, MOVE_X, MOVE_Y);
	assert(FloatEqual(moveablePerson.mPosStatic.x, MOVE_X) && FloatEqual(moveablePerson.mPosStatic.y, MOVE_Y));

	moveablePerson.GetTypeInfo().PrintProcedures(1);
}