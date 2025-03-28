#pragma once

#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <filesystem>

struct PropertyInfo {
	std::string type;
	std::string name;
	std::string metaName;
};

struct MethodInfo {
	std::string name;
};

struct ClassInfo {
	std::string name;
	std::vector<PropertyInfo> properties;
	std::vector<MethodInfo> methods;
};

class ReflectionGenerator {
public:
	void Parse(const std::filesystem::path& path);
	void WriteGeneratedHeader(const std::filesystem::path& outPath);

private:
	std::filesystem::path mHeaderFilename;
	std::vector<ClassInfo> mClasses;
};