#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

struct Hashes {
    std::string md5;
    std::string sha1;
    std::string sha256;
};

struct SectionInfo {
    std::string name;
    std::string virtualAddress;
    std::string virtualSize;
    std::string rawSize;
    double entropy;
    std::string permissions;
    std::string suspiciousFlags;
};

struct FunctionInfo {
    std::string name;
    std::string address;
    bool suspicious;
};

struct ImportInfo {
    std::string dllName;
    std::vector<FunctionInfo> functions;
};

struct StringInfo {
    std::string value;
    std::string category;
    std::string address;
};

struct Indicator {
    std::string category;
    std::string description;
    std::string severity;
};

struct BinaryInfo {
    std::string filePath;
    std::string fileName;
    uint64_t fileSize;
    Hashes hashes;
    std::string architecture;
    std::string compileTime;
    std::string fileCreated;
    std::string entryPoint;
    std::string subsystem;
    std::string format;
    std::vector<SectionInfo> sections;
    std::vector<ImportInfo> imports;
    std::vector<StringInfo> strings;
    std::vector<Indicator> indicators;
};

BinaryInfo analyzeBinary(const char* path);
Hashes computeHashes(const char* path);
std::vector<StringInfo> extractStrings(const uint8_t* data, size_t len);
std::vector<Indicator> analyzeIndicators(
    const std::vector<SectionInfo>& sections,
    const std::vector<ImportInfo>& imports,
    const std::vector<StringInfo>& strings,
    uint64_t fileSize
);

#endif