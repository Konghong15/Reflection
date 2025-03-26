#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <cstddef>

namespace reflect
{
    // �⺻ Ÿ�� ��ũ����
    struct TypeDescriptor {
        const char* Name;
        size_t Size;

        TypeDescriptor(const char* name, size_t size)
            : Name(name)
            , Size(size)
        {
        }
        virtual ~TypeDescriptor()
        {
        }
        virtual std::string GetFullName() const
        {
            return Name;
        }
        virtual void Dump(const void* instance, int indentLevel = 0) const = 0;
    };


    // ������Ƽ�� ������ ��ũ���� �������� �� ���� ����
    template <typename T>
    TypeDescriptor* GetPrimitiveDescriptor();

    // ���÷��� ������ ã�� �� SFINAE�� �б� ó�����ִ� ���� Ŭ����
    struct TypeResolveHelper {
        template <typename T> static char func(decltype(&T::Reflection));
        template <typename T> static int func(...);
        template <typename T>
        struct IsReflected {
            enum {
                value = (sizeof(func<T>(nullptr)) == sizeof(char))
            };
        };

        template <typename T, typename std::enable_if<IsReflected<T>::value, int>::type = 0>
        static TypeDescriptor* Get()
        {
            return &T::Reflection;
        }

        // This version is called otherwise:
        template <typename T, typename std::enable_if<!IsReflected<T>::value, int>::type = 0>
        static TypeDescriptor* Get()
        {
            return GetPrimitiveDescriptor<T>();
        }
    };

    // ���÷����� �õ��� �� �����ϴ� �������̽�
    template <typename T>
    struct TypeResolver {
        static TypeDescriptor* Get()
        {
            return TypeResolveHelper::Get<T>();
        }
    };

    // ����ü Ÿ�� ��ũ����
    struct TypeDescriptor_Struct : TypeDescriptor {
        struct Member {
            const char* Name;
            size_t Offset;
            TypeDescriptor* Type;
        };

        std::vector<Member> Members;

        TypeDescriptor_Struct(void (*initializer)(TypeDescriptor_Struct*))
            : TypeDescriptor(nullptr, 0)
        {
            initializer(this);
        }
        TypeDescriptor_Struct(const char* name, size_t size, const std::initializer_list<Member>& members)
            : TypeDescriptor(nullptr, 0)
            , Members(members)
        {
        }

        virtual void Dump(const void* instance, int indentLevel) const override
        {
            std::cout << Name << " {" << std::endl;

            for (const Member& member : Members) {
                std::cout
                    << std::string(4 * (indentLevel + 1), ' ')
                    << member.Name
                    << " = ";

                void* memberPtr = (char*)instance + member.Offset;
                member.Type->Dump(memberPtr, indentLevel + 1);

                std::cout << std::endl;
            }

            std::cout << std::string(4 * indentLevel, ' ') << "}";
        };
    };

    // ��ũ��
#define REFLECT() \
    friend struct reflect::TypeResolveHelper; \
    static reflect::TypeDescriptor_Struct Reflection; \
    static void initReflection(reflect::TypeDescriptor_Struct*);

#define REFLECT_STRUCT_BEGIN(type) \
    reflect::TypeDescriptor_Struct type::Reflection{type::initReflection}; \
    void type::initReflection(reflect::TypeDescriptor_Struct* typeDesc) { \
        using T = type; \
        typeDesc->Name = #type; \
        typeDesc->Size = sizeof(T); \
        typeDesc->Members = {

#define REFLECT_STRUCT_MEMBER(name) \
            { #name, offsetof(T, name), reflect::TypeResolver<decltype(T::name)>::Get() },

#define REFLECT_STRUCT_END() \
        }; \
    }

    // ���� ��ũ����
    struct TypeDescriptor_StdVector : TypeDescriptor {
        TypeDescriptor* elementType;
        size_t(*GetSize)(const void*); // �� �Լ� �����ͷ� ����ؼ� ����?
        const void* (*GetElement)(const void*, size_t);  // �� �Լ� �����ͷ� ����ؼ� ����?

        template <typename ElementType>
        TypeDescriptor_StdVector(ElementType*)
            : TypeDescriptor("std::vector<>", sizeof(std::vector<ElementType>))
                , elementType{ TypeResolver<ElementType>::Get() }
        {
            GetSize = [](const void* vecPtr) ->size_t
                {
                    const auto& vec = *(const std::vector<ElementType>*) vecPtr;
                    return vec.size();
                };

            GetElement = [](const void* vecPtr, size_t index) ->const void*
                {
                    const auto& vec = *(const std::vector<ElementType>*) vecPtr;
                    return &vec[index];
                };
        }
        virtual std::string GetFullName() const override
        {
            return std::string("std::vector<") + elementType->GetFullName() + ">";
        }
        virtual void Dump(const void* obj, int indentLevel) const override
        {
            size_t numElements = GetSize(obj);
            std::cout << GetFullName();

            if (numElements == 0) {
                std::cout << "{}";
                return;
            }

            std::cout << "{" << std::endl;

            for (size_t index = 0; index < numElements; ++index) {
                std::cout
                    << std::string(4 * (indentLevel + 1), ' ')
                    << "["
                    << index
                    << "] ";

                const void* elememtPtr = GetElement(obj, index);
                elementType->Dump(elememtPtr, indentLevel + 1);

                std::cout << std::endl;
            }

            std::cout << std::string(4 * indentLevel, ' ') << "}";
        }
    };

    // ���� Ư��ȭ - ��� �� Ŭ������?
    template <typename T>
    class TypeResolver<std::vector<T>> {
    public:
        static TypeDescriptor* Get()
        {
            static TypeDescriptor_StdVector typeDesc{ (T*) nullptr };
            return &typeDesc;
        }
    };
};