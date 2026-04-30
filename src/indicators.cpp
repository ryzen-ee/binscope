#include "parser.h"
#include <vector>
#include <string>
#include <algorithm>

std::vector<Indicator> analyzeIndicators(
    const std::vector<SectionInfo>& sections,
    const std::vector<ImportInfo>& imports,
    const std::vector<StringInfo>& strings,
    uint64_t fileSize
) {
    std::vector<Indicator> indicators;
    
    const char* packers[] = {"upx", "aspack", "petite", "upack", "themida"};
    for (const auto& sec : sections) {
        for (const char* p : packers) {
            if (sec.name.find(p) != std::string::npos) {
                Indicator ind;
                ind.category = "Packing";
                ind.description = "Packer section detected: " + sec.name;
                ind.severity = "Critical";
                indicators.push_back(ind);
                break;
            }
        }
        if (sec.entropy > 7.5) {
            Indicator ind;
            ind.category = "Entropy";
            ind.description = "High entropy in " + sec.name + " section";
            ind.severity = "Warning";
            indicators.push_back(ind);
        }
        if (sec.name == ".text" && sec.permissions.find('W') != std::string::npos) {
            Indicator ind;
            ind.category = "Section";
            ind.description = ".text section is writable (unusual)";
            ind.severity = "Warning";
            indicators.push_back(ind);
        }
    }
    
    const char* suspiciousAPIs[] = {"VirtualAlloc", "VirtualProtect", "CreateRemoteThread", "WriteProcessMemory", "GetProcAddress", "LoadLibrary", "WinExec", "ShellExecute"};
    for (const auto& imp : imports) {
        for (const auto& func : imp.functions) {
            for (const char* api : suspiciousAPIs) {
                if (func.name.find(api) != std::string::npos) {
                    Indicator ind;
                    ind.category = "Imports";
                    ind.description = "Suspicious API: " + func.name + " from " + imp.dllName;
                    ind.severity = "Warning";
                    indicators.push_back(ind);
                    break;
                }
            }
        }
    }
    
    for (const auto& str : strings) {
        if (str.category == "Command") {
            Indicator ind;
            ind.category = "Strings";
            ind.description = "Suspicious command: " + str.value.substr(0, 50);
            ind.severity = "Warning";
            indicators.push_back(ind);
        }
        if (str.value.find("autorun") != std::string::npos || str.value.find("schtasks") != std::string::npos) {
            Indicator ind;
            ind.category = "Persistence";
            ind.description = "Persistence mechanism: " + str.value.substr(0, 50);
            ind.severity = "Warning";
            indicators.push_back(ind);
        }
    }
    
    if (fileSize > 10 * 1024 * 1024) {
        Indicator ind;
        ind.category = "File Size";
        ind.description = "Large file: " + std::to_string(fileSize / (1024*1024)) + " MB";
        ind.severity = "Info";
        indicators.push_back(ind);
    }
    
    std::sort(indicators.begin(), indicators.end(), [](const Indicator& a, const Indicator& b) { return a.description < b.description; });
    indicators.erase(std::unique(indicators.begin(), indicators.end(), [](const Indicator& a, const Indicator& b) { return a.description == b.description; }), indicators.end());
    
    return indicators;
}