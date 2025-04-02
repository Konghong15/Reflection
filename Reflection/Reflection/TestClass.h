#pragma once

#include <cassert>
#include <iostream>
#include <vector>

#include "GCObject.h"

class Component
{
	GENERATE_TYPE_INFO(Component)
public:
	virtual ~Component() {}

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

	friend std::ostream& operator<<(std::ostream& os, const Vector3& vec)
	{
		os << "Vector3(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
		return os;
	}
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

	PROCEDURE(SetPositionXYZ)
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
