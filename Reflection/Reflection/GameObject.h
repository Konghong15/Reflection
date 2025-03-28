#pragma once

#include "TypeInfo.h"
#include "Property.h"
#include "ReflectionMacros.h"

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

	// Getter
	const Vector3& GetPosition() const { return mPosition; }

	// Setter
	void SetPosition(const Vector3& position) { mPosition = position; }
	void SetPosition(float x, float y, float z) { mPosition = Vector3(x, y, z); }

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
