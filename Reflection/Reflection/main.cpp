#include <type_traits>
#include <cassert>
#include "TypeInfo.h"
#include "Property.h"

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


int main()
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

	return 0;
}
