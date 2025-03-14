#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <functional>

// 리플렉션 데이터 저장 구조체
struct TypeInfo {
	std::string name;
	std::vector<std::string> fields;
	std::function<void* ()> constructor;
};

// 전역 클래스 정보 저장소
std::unordered_map<std::string, TypeInfo>& GetTypeRegistry() {
	static std::unordered_map<std::string, TypeInfo> registry;
	return registry;
}

// 템플릿을 이용한 클래스 등록
template<typename T>
struct Reflect {
	static void Register(const std::string& className, std::function<void* ()> constructor) {
		GetTypeRegistry()[className] = { className, {}, constructor };
	}
};

// 필드 등록 템플릿
template<typename T, typename U>
struct Field {
	std::string name;
	U T::* ptr;
};

// 클래스 정보 저장 템플릿
template<typename T>
struct ClassMeta {
	static std::vector<std::pair<std::string, size_t>> fields;

	template<typename... Fields>
	static void Register(const std::string& className, std::function<void* ()> constructor, Fields... f) {
		Reflect<T>::Register(className, constructor);
		(fields.push_back({ f.name, sizeof(f.ptr) }), ...);
	}
};

// `ClassMeta`의 필드 초기화
template<typename T>
std::vector<std::pair<std::string, size_t>> ClassMeta<T>::fields = {};

// 객체를 문자열로 동적 생성
void* CreateInstance(const std::string& className) {
	auto& registry = GetTypeRegistry();
	if (registry.find(className) != registry.end()) {
		return registry[className].constructor();
	}
	return nullptr;
}

// 🔹 예제 클래스
class Person {
public:
	std::string name;
	int age;

	void SayHello() {
		std::cout << "Hello, my name is " << name << "!" << std::endl;
	}

	// 객체를 문자열로 변환 (JSON 없이)
	std::string Serialize() {
		std::stringstream ss;
		for (const auto& field : ClassMeta<Person>::fields) {
			if (field.first == "name") ss << "name: " << name << ", ";
			if (field.first == "age") ss << "age: " << age;
		}
		return ss.str();
	}
};

// 🔹 클래스 등록 (템플릿 사용)
void RegisterClasses() {
	ClassMeta<Person>::Register("Person", []() -> void* { return new Person(); },
		Field<Person, std::string>{"name", & Person::name},
		Field<Person, int>{"age", & Person::age}
	);
}

// 🔹 런타임에서 클래스 정보 조회 & 객체 생성
int main() {
	RegisterClasses();

	// 문자열 기반으로 객체 생성
	void* obj = CreateInstance("Person");
	if (obj) {
		Person* person = static_cast<Person*>(obj);
		person->name = "Alice";
		person->age = 25;
		person->SayHello();

		// 객체 직렬화 (JSON 없이)
		std::cout << "Serialized: " << person->Serialize() << std::endl;

		delete person;
	}
	else {
		std::cout << "Class not found!\n";
	}

	return 0;
}
