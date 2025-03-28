#pragma once
#include "GameObject.h"

inline static struct RegistPropertyExecutor_mPosition {
    RegistPropertyExecutor_mPosition() {
        static PropertyRegister<TransformComponent, return, decltype(&TransformComponent::mPosition), &TransformComponent::mPosition>
            property_register_mPosition{ "z", TypeInfo::GetStaticTypeInfo<TransformComponent>() };
    }
} regist_mPosition;

inline static struct RegistPropertyExecutor_mPosition {
    RegistPropertyExecutor_mPosition() {
        static PropertyRegister<TransformComponent, Vector3, decltype(&TransformComponent::mPosition), &TransformComponent::mPosition>
            property_register_mPosition{ "mPosition", TypeInfo::GetStaticTypeInfo<TransformComponent>() };
    }
} regist_mPosition;

