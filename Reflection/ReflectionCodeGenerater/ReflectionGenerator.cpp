#include "ReflectionGenerator.h"

void ReflectionGenerator::Parse(const std::filesystem::path& path) {
	mHeaderFilename = path.filename();
	std::ifstream file(path);
	std::string line;

	std::regex classDeclRegex(R"(class\s+(\w+))");
	std::regex generateMacroRegex(R"(GENERATE_CLASS_TYPE_INFO\s*\(\))");
	std::regex propertyRegex(R"(PROPERTY\s*\(\))");
	std::regex memberRegex(R"((\w[\w\s:<>,\*&]*)\s+(\w+)\s*;)");
	std::regex methodMacroRegex(R"(METHOD\s*\(\))");
	std::regex methodSigRegex(R"(\w[\w\s:<>,\*&]*\s+(\w+)\s*\(.*\)\s*;)");

	std::string currentClassName;
	ClassInfo* currentClass = nullptr;
	bool pendingProperty = false;
	bool pendingMethod = false;

	while (std::getline(file, line)) {
		std::smatch match;

		// 클래스 선언 찾기
		if (std::regex_search(line, match, classDeclRegex)) {
			currentClassName = match[1].str();
			currentClass = nullptr;
		}

		// 클래스 내부에서 매크로 확인
		if (!currentClassName.empty() && std::regex_search(line, generateMacroRegex)) {
			mClasses.emplace_back(ClassInfo{ currentClassName });
			currentClass = &mClasses.back();
			currentClassName.clear();
		}

		if (currentClass == nullptr) continue;

		// PROPERTY
		if (std::regex_search(line, propertyRegex)) {
			pendingProperty = true;
		}
		else if (pendingProperty && std::regex_search(line, match, memberRegex)) {
			PropertyInfo info;
			info.type = match[1].str();
			info.name = match[2].str();
			info.metaName = info.name; // 기본적으로 멤버 이름을 메타 이름으로 사용
			currentClass->properties.push_back(info);
			pendingProperty = false;
		}

		// METHOD
		else if (std::regex_search(line, methodMacroRegex)) {
			pendingMethod = true;
		}
		else if (pendingMethod && std::regex_search(line, match, methodSigRegex)) {
			MethodInfo m;
			m.name = match[1].str();
			currentClass->methods.push_back(m);
			pendingMethod = false;
		}
	}
}

void ReflectionGenerator::WriteGeneratedHeader(const std::filesystem::path& outPath) {
	std::ofstream out(outPath);
	out << "#pragma once\n";
	out << "#include \"" << mHeaderFilename.string() << "\"\n\n";

	for (const auto& cls : mClasses) {
		out << "inline static struct RegisterTypeInfo_" << cls.name << " {\n";
		out << "    RegisterTypeInfo_" << cls.name << "() {\n";
		out << "        TypeInfo::GetStaticTypeInfo<" << cls.name << ">();\n";
		out << "    }\n";
		out << "} register_typeinfo_" << cls.name << ";\n\n";

		for (const auto& prop : cls.properties) {
			out << "inline static struct RegistPropertyExecutor_" << prop.name << " {\n";
			out << "    RegistPropertyExecutor_" << prop.name << "() {\n";
			out << "        static PropertyRegister<" << cls.name << ", " << prop.type << ", decltype(&" << cls.name << "::" << prop.name << "), &" << cls.name << "::" << prop.name << ">\n";
			out << "            property_register_" << prop.name << "{ \"" << prop.metaName << "\", TypeInfo::GetStaticTypeInfo<" << cls.name << ">() };\n";
			out << "    }\n";
			out << "} regist_" << prop.name << ";\n\n";
		}
		for (const auto& method : cls.methods) {
			out << "inline static struct RegistMethodExecutor_" << method.name << " {\n";
			out << "    RegistMethodExecutor_" << method.name << "() {\n";
			out << "        static MethodRegister<" << cls.name << ", decltype(&" << cls.name << "::" << method.name << "), &" << cls.name << "::" << method.name << ">\n";
			out << "            method_register_" << method.name << "{ \"" << method.name << "\", TypeInfo::GetStaticTypeInfo<" << cls.name << ">() };\n";
			out << "    }\n";
			out << "} regist_" << method.name << ";\n\n";
		}
	}
}
