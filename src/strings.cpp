#include "parser.h"
#include <vector>
#include <string>
#include <sstream>

std::string categorizeString(const std::string& s) {
    if (s.find("http://") == 0 || s.find("https://") == 0 || s.find("ftp://") == 0) {
        return "URL";
    }
    if ((s.length() > 3 && s[1] == ':' && s[2] == '\\') ||
        s.find("/") == 0 || s.find("./") == 0 ||
        s.find("\\Windows\\") != std::string::npos ||
        s.find("\\Program") != std::string::npos ||
        s.find(".exe") != std::string::npos || s.find(".dll") != std::string::npos) {
        return "Path";
    }
    if (s.find("HKEY_") == 0 || s.find("\\Software\\Classes\\") != std::string::npos) {
        return "Registry";
    }
    const char* cmds[] = {"cmd", "powershell", "reg", "net", "sc", "del", "copy", "move"};
    for (const char* cmd : cmds) {
        if (s.find(cmd) == 0 || s.find(" " + std::string(cmd)) != std::string::npos) {
            return "Command";
        }
    }
    return "Plain";
}

std::vector<StringInfo> extractStrings(const uint8_t* data, size_t len) {
    std::vector<StringInfo> strings;
    std::vector<uint8_t> current;
    
    for (size_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        if (b >= 0x20 && b <= 0x7e) {
            current.push_back(b);
            if (current.size() > 256) current.clear();
        } else {
            if (current.size() >= 4) {
                std::string value(current.begin(), current.end());
                StringInfo info;
                info.value = value;
                info.category = categorizeString(value);
                std::ostringstream oss;
                oss << "0x" << std::hex << (i - current.size());
                info.address = oss.str();
                strings.push_back(info);
            }
            current.clear();
        }
    }
    
    if (current.size() >= 4) {
        std::string value(current.begin(), current.end());
        StringInfo info;
        info.value = value;
        info.category = categorizeString(value);
        std::ostringstream oss;
        oss << "0x" << std::hex << (len - current.size());
        info.address = oss.str();
        strings.push_back(info);
    }
    
    if (strings.size() > 1000) strings.resize(1000);
    return strings;
}