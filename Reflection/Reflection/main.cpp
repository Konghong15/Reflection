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
void TestGC(void);
void TestRPC(void);

class TestClass
{
	static int& GetNum1()
	{
		// 이 함수는 호출해야 초기화 되지만
		static int num{ 10 };
		return num;
	}
	static int& GetNum2()
	{
		// 이 함수는 inline static 맴버로 인해 실행하자마자 초기화 됨
		static int num{ 10 };
		return num;
	}

	inline static int& a = GetNum2();
};


int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	GCManager::Create();

	auto intType = TypeInfo::GetStaticTypeInfo<std::vector<int>::value_type>();

	TestTypeInfo();
	TestProperty();
	TestMethod();
	TestGC();
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

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec)
{
	os << "[";
	for (size_t i = 0; i < vec.size(); ++i)
	{
		os << vec[i];
		if (i != vec.size() - 1)
			os << ", ";
	}
	os << "]";
	return os;
}

void TestTypeInfo(void)
{
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
	Vector2() : x(0), y(0) {}
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

struct ArrayTest
{
	GENERATE_TYPE_INFO(ArrayTest)
		PROPERTY(mVectorsinVec)
		PROPERTY(mVectors)

public:
	std::vector<Vector2> mVectorsinVec;
	FixedVector<Vector2, 10> mVectors;
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

	vec2.GetTypeInfo().PrintPropertyValues(&vec2);

	ArrayTest arrayTest;
	arrayTest.mVectorsinVec.push_back({ 1, 5 });
	arrayTest.mVectorsinVec.push_back({ 10, 3 });
	arrayTest.mVectorsinVec.push_back({ 12, 51 });
	arrayTest.mVectors.Add({ 5, 2 });
	arrayTest.mVectors.Add({ 2, 3 });
	arrayTest.mVectors.Add({ 3, 6 });
	arrayTest.GetTypeInfo().PrintPropertyValues(&arrayTest);
	arrayTest.GetTypeInfo().PrintTypeInfoValues(&arrayTest);
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
		void SetName(std::string& name)
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

	Person::StaticTypeInfo().PrintTypeInfo();
}

class TempObject : public GCObject
{
	GENERATE_TYPE_INFO(TempObject)
		PROPERTY(mPrev)
		PROPERTY(mNext)
		PROPERTY(mNeighbor)

public:
	GCObject* mPrev = nullptr;
	GCObject* mNext = nullptr;
	GCObject* mNeighbor[2] = { nullptr, nullptr };
};

class GameInstance : public GCObject
{
	GENERATE_TYPE_INFO(GameInstance)

public:
	void CreateReferenceChain()
	{
		mGCObjectsInVec.resize(OBJECT_COUNT, nullptr);  // 깊이 10 * 개수
		const size_t DEPTH = 10;

		for (int i = 0; i < OBJECT_COUNT / DEPTH; ++i)
		{
			TempObject* chain[DEPTH];

			for (int j = 0; j < DEPTH; ++j)
			{
				chain[j] = NewGCObject<TempObject>(GCManager::Get());
				mGCObjects.Add(chain[j]);
				mGCObjectsInVec[i * DEPTH + j] = chain[j];
			}

			// prev/next + neighbor 연결
			for (int j = 0; j < DEPTH; ++j)
			{
				TempObject* current = static_cast<TempObject*>(chain[j]);

				if (j > 0)
				{
					TempObject* left = static_cast<TempObject*>(chain[j - 1]);
					current->mPrev = left;
				}

				if (j < DEPTH - 1)
				{
					TempObject* right = static_cast<TempObject*>(chain[j + 1]);
					current->mNext = right;
				}
			}
		}
	}

	void RandomizeNeighbors()
	{
		for (GCObject* obj : mGCObjects)
		{
			if (obj->GetTypeInfo().IsChildOf<TempObject>())
			{
				GCObject* left = mGCObjects[rand() % OBJECT_COUNT];
				GCObject* right = mGCObjects[rand() % OBJECT_COUNT];

				TempObject* tempObject = static_cast<TempObject*>(obj);
				tempObject->mNeighbor[0] = left;
				tempObject->mNeighbor[1] = right;
			}
		}
	}

	void ReleaseObjectReference()
	{
		const size_t totalCount = OBJECT_COUNT;

		for (int i = totalCount - 1; i >= 0; --i)
		{
			mGCObjects[i] = nullptr;
			mGCObjects.RemoveLast();
		}

		for (size_t i = 0; i < mGCObjectsInVec.size(); ++i)
		{
			mGCObjectsInVec[i] = nullptr;
		}

		mGCObjectsInVec.clear();
	}

private:
	enum { OBJECT_COUNT = 10000 };
	PROPERTY(mGCObjects)
		FixedVector<GCObject*, OBJECT_COUNT> mGCObjects;
	PROPERTY(mGCObjectsInVec)
		std::vector<GCObject*> mGCObjectsInVec;
};

void TestGC(void)
{
	const size_t TEST_INSTANCE_COUNT = 10;
	GameInstance* gameInstances[TEST_INSTANCE_COUNT];
	const GCDebugInfo& lastInfo = GCManager::Get().GetLastDebugInfo();

	// 1. 싱글 스레드-마크
	for (size_t i = 0; i < TEST_INSTANCE_COUNT; ++i)
	{
		gameInstances[i] = NewGCObject<GameInstance>(GCManager::Get());
		gameInstances[i]->SetRoot(true);
		gameInstances[i]->CreateReferenceChain();
		gameInstances[i]->RandomizeNeighbors();
	}

	GCManager::Get().Collect();
	assert(lastInfo.RootObjectCount == 10);
	assert(lastInfo.DeletedObjects == 0);

	// 2. 싱글 스레드-스윕
	for (size_t i = 0; i < TEST_INSTANCE_COUNT; ++i)
	{
		gameInstances[i]->ReleaseObjectReference();
	}

	GCManager::Get().Collect();
	assert(lastInfo.RootObjectCount == 10);
	assert(lastInfo.DeletedObjects == 100000);

	// 3. 멀티 스레드-마크
	for (size_t i = 0; i < TEST_INSTANCE_COUNT; ++i)
	{
		gameInstances[i]->CreateReferenceChain();
		gameInstances[i]->RandomizeNeighbors();
	}

	GCManager::Get().CollectMultiThread();
	assert(lastInfo.RootObjectCount == 10);
	assert(lastInfo.DeletedObjects == 0);

	// 4. 멀티 스레드-스윕
	for (size_t i = 0; i < TEST_INSTANCE_COUNT; ++i)
	{
		gameInstances[i]->ReleaseObjectReference();
	}

	GCManager::Get().CollectMultiThread();
	assert(lastInfo.RootObjectCount == 10);
	assert(lastInfo.DeletedObjects == 100000);

	// 5. 루트 제거 테스트
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