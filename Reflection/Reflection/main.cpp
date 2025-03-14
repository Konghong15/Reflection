#include <iostream>
#include <unordered_map>
#include <vector>
#include <functional>
#include <string>

struct TypeInfo {
	std::string name;
	std::vector<std::string> fields;
	std::vector<std::string> methods;
	std::function<void* ()> constructor;
};

// ���� Ŭ���� ���� �����
std::unordered_map<std::string, TypeInfo> g_TypeRegistry;

// ��ũ�θ� �̿��� Ŭ���� ���
#define REGISTER_CLASS(CLASS) \
    g_TypeRegistry[#CLASS] = TypeInfo{#CLASS, {}, {}, []() -> void* { return new CLASS(); }};

#define REGISTER_FIELD(CLASS, FIELD) \
    g_TypeRegistry[#CLASS].fields.push_back(#FIELD);

#define REGISTER_METHOD(CLASS, METHOD) \
    g_TypeRegistry[#CLASS].methods.push_back(#METHOD);

// ���� Ŭ����
class Person {
public:
	std::string name;
	int age;

	void SayHello() {
		std::cout << "Hello, my name is " << name << "!" << std::endl;
	}
};

// ��Ÿ������ ��� �Լ�
void RegisterTypes() {
	REGISTER_CLASS(Person);
	REGISTER_FIELD(Person, name);
	REGISTER_FIELD(Person, age);
	REGISTER_METHOD(Person, SayHello);
}

void* CreateInstnace(const std::string& className)
{
	auto it = g_TypeRegistry.find(className);

	if (it != g_TypeRegistry.end() && it->second.constructor)
	{
		return it->second.constructor();
	}

	return nullptr;
}

int main() {
	RegisterTypes();

	// "Person" ���ڿ��� ������� �������� ��ü ����
	void* obj = CreateInstnace("Person");
	if (obj)
	{
		Person* person = static_cast<Person*>(obj);
		person->name = "Alice";
		person->age = 25;
		person->SayHello();
		delete person;
	}
	else
	{
		std::cout << "Class not found!\n";
	}

	// ��Ÿ�ӿ��� Ŭ���� ���� ��ȸ
	auto it = g_TypeRegistry.find("Person");
	
	if (it != g_TypeRegistry.end()) {
		std::cout << "Class: " << it->second.name << "\n";
		std::cout << "Fields:\n";
		for (const auto& field : it->second.fields) {
			std::cout << "  " << field << "\n";
		}
		std::cout << "Methods:\n";
		for (const auto& method : it->second.methods) {
			std::cout << "  " << method << "()\n";
		}
	}

	return 0;
}
