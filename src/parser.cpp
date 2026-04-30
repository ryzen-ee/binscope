#include "parser.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <sys/stat.h>

struct ElfHeader {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct DosHeader {
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint8_t e_res[8];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint8_t e_res2[20];
    uint32_t e_lfanew;
};

struct PEHeader {
    uint16_t machine;
    uint16_t number_of_sections;
    uint32_t time_date_stamp;
    uint32_t pointer_to_symbol_table;
    uint32_t number_of_symbols;
    uint16_t size_of_optional_header;
    uint16_t characteristics;
};

struct SectionHeader {
    uint8_t name[8];
    uint32_t virtual_size;
    uint32_t virtual_address;
    uint32_t size_of_raw_data;
    uint32_t pointer_to_raw_data;
    uint32_t characteristics;
};

struct OptionalHeader32 {
    uint16_t magic;
    uint8_t major_linker_version;
    uint8_t minor_linker_version;
    uint32_t size_of_code;
    uint32_t size_of_initialized_data;
    uint32_t size_of_uninitialized_data;
    uint32_t address_of_entry_point;
    uint32_t base_of_code;
    uint32_t base_of_data;
    uint32_t image_base;
    uint32_t section_alignment;
    uint32_t file_alignment;
    uint16_t major_os_version;
    uint16_t minor_os_version;
    uint32_t size_of_image;
    uint32_t size_of_headers;
    uint32_t check_sum;
    uint16_t subsystem;
    uint16_t dll_characteristics;
    uint32_t size_of_stack_reserve;
    uint32_t size_of_stack_commit;
    uint32_t size_of_heap_reserve;
    uint32_t size_of_heap_commit;
    uint32_t loader_flags;
    uint32_t number_of_rva_and_sizes;
    uint64_t data_directory[16];
};

struct OptionalHeader64 {
    uint16_t magic;
    uint8_t major_linker_version;
    uint8_t minor_linker_version;
    uint32_t size_of_code;
    uint32_t size_of_initialized_data;
    uint32_t size_of_uninitialized_data;
    uint32_t address_of_entry_point;
    uint32_t base_of_code;
    uint64_t image_base;
    uint32_t section_alignment;
    uint32_t file_alignment;
    uint16_t major_os_version;
    uint16_t minor_os_version;
    uint32_t size_of_image;
    uint32_t size_of_headers;
    uint32_t check_sum;
    uint16_t subsystem;
    uint16_t dll_characteristics;
    uint64_t size_of_stack_reserve;
    uint64_t size_of_stack_commit;
    uint64_t size_of_heap_reserve;
    uint64_t size_of_heap_commit;
    uint32_t loader_flags;
    uint32_t number_of_rva_and_sizes;
    uint64_t data_directory[16];
};

struct ImportDescriptor {
    uint32_t original_first_thunk;
    uint32_t time_date_stamp;
    uint32_t forwarder_chain;
    uint32_t name_rva;
    uint32_t first_thunk;
};

std::string readStringFromRVA(const uint8_t* data, size_t len, uint32_t rva, size_t maxLen = 256);

bool isELF(const uint8_t* data, size_t len) {
    return len >= 16 && data[0] == 0x7f && data[1] == 'E' && data[2] == 'L' && data[3] == 'F';
}

bool isPE(const uint8_t* data, size_t len) {
    return len >= 64 && data[0] == 'M' && data[1] == 'Z';
}

std::string formatHex(uint64_t val) {
    std::ostringstream oss;
    oss << "0x" << std::hex << val;
    return oss.str();
}

bool rvaToOffset(uint32_t rva, uint32_t& offset, const std::vector<SectionInfo>& sections) {
    for (const auto& sec : sections) {
        uint32_t va = 0;
        if (sec.virtualAddress.substr(0, 2) == "0x") {
            va = std::stoul(sec.virtualAddress.substr(2), nullptr, 16);
        }
        uint32_t vs = std::stoul(sec.virtualSize.substr(2), nullptr, 16);
        if (rva >= va && rva < va + vs) {
            offset = rva - va + std::stoul(sec.rawSize.substr(2), nullptr, 16) - vs;
            return true;
        }
    }
    return false;
}

std::string readStringFromRVA(const uint8_t* data, size_t len, uint32_t rva, size_t maxLen) {
    if (rva == 0 || rva >= len) return "";
    const char* str = reinterpret_cast<const char*>(data + rva);
    size_t slen = 0;
    while (slen < maxLen && rva + slen < len && str[slen] != '\0') {
        slen++;
    }
    return std::string(str, slen);
}

std::string getSectionName(const uint8_t* nameData, size_t nameLen, const char* stringTable, size_t tableSize) {
    if (nameLen == 0 || nameData == nullptr) return "";
    
    if (nameData[0] == '/') {
        if (nameData[1] >= '0' && nameData[1] <= '9') {
            std::string numStr;
            for (size_t i = 1; i < nameLen && i < 8; i++) {
                if (nameData[i] >= '0' && nameData[i] <= '9') {
                    numStr += char(nameData[i]);
                } else {
                    break;
                }
            }
            if (!numStr.empty()) {
                size_t offset = std::stoul(numStr);
                if (stringTable && offset < tableSize) {
                    return std::string(stringTable + offset);
                }
            }
        }
    }
    
    std::string result;
    for (size_t i = 0; i < 8 && i < nameLen; i++) {
        if (nameData[i] == 0) break;
        result += char(nameData[i]);
    }
    return result;
}

double calculateEntropy(const uint8_t* data, size_t len) {
    if (len == 0) return 0.0;
    int counts[256] = {0};
    for (size_t i = 0; i < len; i++) counts[data[i]]++;
    double entropy = 0.0;
    for (int i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            double p = (double)counts[i] / len;
            entropy -= p * log2(p);
        }
    }
    return entropy;
}

std::string getELFPermissions(uint64_t flags) {
    std::string perms;
    if (flags & 0x4) perms += 'R';
    if (flags & 0x2) perms += 'W';
    if (flags & 0x1) perms += 'X';
    return perms.empty() ? "None" : perms;
}

std::string getPEPermissions(uint32_t chars) {
    std::string perms;
    if (chars & 0x40000000) perms += 'R';
    if (chars & 0x80000000) perms += 'W';
    if (chars & 0x20000000) perms += 'X';
    return perms.empty() ? "None" : perms;
}

void parseELF(const uint8_t* data, size_t len, BinaryInfo& info);
void parsePE(const uint8_t* data, size_t len, BinaryInfo& info);

BinaryInfo analyzeBinary(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file");
    }
    
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    file.close();
    
    BinaryInfo info;
    info.filePath = path;
    
    size_t lastSlash = info.filePath.find_last_of("/\\");
    info.fileName = (lastSlash != std::string::npos) ? 
        info.filePath.substr(lastSlash + 1) : info.filePath;
    
    info.fileSize = size;
    
    struct stat st;
    if (stat(path, &st) == 0) {
        time_t created = st.st_ctime;
        if (created > 0) {
            char buf[64];
            struct tm* tm = localtime(&created);
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
            info.fileCreated = buf;
        }
    }
    
    info.hashes = computeHashes(path);
    
    if (isELF(buffer.data(), buffer.size())) {
        info.format = "ELF";
        parseELF(buffer.data(), buffer.size(), info);
    } else if (isPE(buffer.data(), buffer.size())) {
        info.format = "PE";
        parsePE(buffer.data(), buffer.size(), info);
    } else {
        throw std::runtime_error("Unsupported binary format");
    }
    
    if (info.architecture == "Unknown" || info.architecture.find("Unknown") != std::string::npos) {
        std::string lowerName = info.fileName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerName.find("x64") != std::string::npos || lowerName.find("64-bit") != std::string::npos || lowerName.find("amd64") != std::string::npos) {
            info.architecture = "x64 (detected via filename)";
        } else if (lowerName.find("x86") != std::string::npos || lowerName.find("32-bit") != std::string::npos || lowerName.find("i386") != std::string::npos) {
            info.architecture = "x86 (detected via filename)";
        } else if (lowerName.find("arm64") != std::string::npos || lowerName.find("aarch64") != std::string::npos) {
            info.architecture = "ARM64 (detected via filename)";
        } else if (lowerName.find("arm") != std::string::npos) {
            info.architecture = "ARM (detected via filename)";
        }
    }
    
    info.strings = extractStrings(buffer.data(), buffer.size());
    info.indicators = analyzeIndicators(info.sections, info.imports, info.strings, info.fileSize);
    
    return info;
}

void parseELF(const uint8_t* data, size_t len, BinaryInfo& info) {
    const ElfHeader* eh = reinterpret_cast<const ElfHeader*>(data);
    
    uint16_t machine = eh->e_machine;
    switch (machine) {
        case 0x03: info.architecture = "x86 (ELF32)"; break;
        case 0x3e: info.architecture = "x64 (ELF64)"; break;
        case 0x28: info.architecture = "ARM"; break;
        case 0xb7: info.architecture = "ARM64 (AArch64)"; break;
        case 0x08: info.architecture = "MIPS"; break;
        case 0x14: info.architecture = "PowerPC"; break;
        case 0x15: info.architecture = "PowerPC64"; break;
        case 0x12: info.architecture = "SPARC"; break;
        case 0x2a: info.architecture = "SPARC64"; break;
        case 0x47: info.architecture = "RISC-V"; break;
        default:
            if (eh->e_ident[4] == 2) {
                info.architecture = "x64 (ELF64)";
            } else {
                info.architecture = "x86 (ELF32)";
            }
            break;
    }
    info.entryPoint = formatHex(eh->e_entry);
    info.subsystem = "ELF";
    
    uint16_t shnum = eh->e_shnum;
    uint32_t shoff = eh->e_shoff;
    uint16_t shentsize = eh->e_shentsize;
    
    uint16_t shstrndx = eh->e_shstrndx;
    const char* shstrtab = nullptr;
    if (shstrndx < shnum) {
        const SectionHeader* strSec = reinterpret_cast<const SectionHeader*>(data + shoff + shstrndx * shentsize);
        shstrtab = reinterpret_cast<const char*>(data + strSec->pointer_to_raw_data);
    }
    
    for (int i = 1; i < shnum && i < 100; i++) {
        const SectionHeader* sh = reinterpret_cast<const SectionHeader*>(data + shoff + i * shentsize);
        
        SectionInfo sec;
        char name[9] = {0};
        memcpy(name, sh->name, 8);
        sec.name = name;
        
        sec.virtualAddress = formatHex(sh->virtual_address);
        sec.virtualSize = formatHex(sh->virtual_size);
        sec.rawSize = formatHex(sh->size_of_raw_data);
        
        size_t rawStart = sh->pointer_to_raw_data;
        size_t rawLen = sh->size_of_raw_data;
        if (rawStart + rawLen <= len) {
            sec.entropy = calculateEntropy(data + rawStart, rawLen);
        } else {
            sec.entropy = 0.0;
        }
        
        sec.permissions = getELFPermissions(sh->virtual_size);
        
        if (sec.entropy > 7.0 && sh->size_of_raw_data > 1024) {
            sec.suspiciousFlags = "High entropy (possibly packed)";
        }
        
        info.sections.push_back(sec);
    }
}

uint32_t rvaToFileOffset(uint32_t rva, const uint8_t* data, size_t len, 
                          uint32_t numSections, size_t sectionOffset) {
    for (uint32_t i = 0; i < numSections; i++) {
        const SectionHeader* sh = reinterpret_cast<const SectionHeader*>(data + sectionOffset + i * 40);
        uint32_t secVA = sh->virtual_address;
        uint32_t secVS = sh->virtual_size;
        uint32_t secRaw = sh->pointer_to_raw_data;
        uint32_t secRawSize = sh->size_of_raw_data;
        
        if (rva >= secVA && rva < secVA + secVS) {
            return secRaw + (rva - secVA);
        }
    }
    return 0;
}

void parsePE(const uint8_t* data, size_t len, BinaryInfo& info) {
    const DosHeader* dh = reinterpret_cast<const DosHeader*>(data);
    uint32_t peOffset = dh->e_lfanew;
    
    if (peOffset + 24 > len) {
        throw std::runtime_error("Invalid PE file");
    }
    
    const PEHeader* peh = reinterpret_cast<const PEHeader*>(data + peOffset + 4);
    
    uint16_t machine = peh->machine;
    switch (machine) {
        case 0x014c: info.architecture = "x86 (PE32)"; break;
        case 0x8664: info.architecture = "x64 (PE64)"; break;
        case 0x01c0: info.architecture = "ARM"; break;
        case 0xaa64: info.architecture = "ARM64"; break;
        case 0x01c4: info.architecture = "ARMNT (ARM Thumb-2)"; break;
        case 0x0200: info.architecture = "IA64 (Itanium)"; break;
        case 0x01e0: info.architecture = "MIPS"; break;
        case 0x0162: info.architecture = "MIPS64"; break;
        case 0x01f0: info.architecture = "PowerPC"; break;
        case 0x0120: info.architecture = "R4000 (MIPS)"; break;
        default: 
            info.architecture = "Unknown (0x" + formatHex(machine) + ")"; 
            break;
    }
    
    uint32_t importRva = 0;
    uint32_t importSize = 0;
    bool is64bit = false;
    bool hasValidOptHeader = false;
    
    size_t optHeaderOffset = peOffset + 4 + sizeof(PEHeader);
    if (optHeaderOffset + 2 < len) {
        uint16_t magic = *reinterpret_cast<const uint16_t*>(data + optHeaderOffset);
        if (magic == 0x10b) {
            const OptionalHeader32* oh = reinterpret_cast<const OptionalHeader32*>(data + optHeaderOffset);
            info.entryPoint = formatHex(oh->address_of_entry_point);
            info.subsystem = (oh->subsystem == 2) ? "GUI" : "Console";
            if (oh->number_of_rva_and_sizes > 1) {
                importRva = static_cast<uint32_t>(oh->data_directory[1] & 0xFFFFFFFF);
                importSize = static_cast<uint32_t>((oh->data_directory[1] >> 32) & 0xFFFFFFFF);
            }
            is64bit = false;
            hasValidOptHeader = true;
        } else if (magic == 0x20b) {
            const OptionalHeader64* oh = reinterpret_cast<const OptionalHeader64*>(data + optHeaderOffset);
            info.entryPoint = formatHex(oh->address_of_entry_point);
            info.subsystem = (oh->subsystem == 2) ? "GUI" : "Console";
            if (oh->number_of_rva_and_sizes > 1) {
                importRva = static_cast<uint32_t>(oh->data_directory[1] & 0xFFFFFFFF);
                importSize = static_cast<uint32_t>((oh->data_directory[1] >> 32) & 0xFFFFFFFF);
            }
            is64bit = true;
            hasValidOptHeader = true;
        }
    }
    
    if (!hasValidOptHeader) {
        info.subsystem = "Unknown";
    }
    
    if (peh->time_date_stamp > 0) {
        time_t ts = peh->time_date_stamp;
        char buf[64];
        struct tm* tm = gmtime(&ts);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
        info.compileTime = buf;
    }
    
    uint32_t numSections = peh->number_of_sections;
    size_t sectionOffset = optHeaderOffset + peh->size_of_optional_header;
    
    const char* stringTable = nullptr;
    size_t stringTableSize = 0;
    
    for (uint32_t i = 0; i < numSections && i < 50; i++) {
        const SectionHeader* sh = reinterpret_cast<const SectionHeader*>(data + sectionOffset + i * 40);
        
        SectionInfo sec;
        sec.name = getSectionName(sh->name, 8, stringTable, stringTableSize);
        
        if (sec.name == ".rsrc" && !stringTable && sh->pointer_to_raw_data > 0 && sh->size_of_raw_data > 0 && sh->size_of_raw_data < len) {
            stringTable = reinterpret_cast<const char*>(data + sh->pointer_to_raw_data);
            stringTableSize = sh->size_of_raw_data;
            
            for (uint32_t j = 0; j < i; j++) {
                const SectionHeader* prevSh = reinterpret_cast<const SectionHeader*>(data + sectionOffset + j * 40);
                if (prevSh->name[0] == '/' && prevSh->name[1] >= '0' && prevSh->name[1] <= '9') {
                    info.sections[j].name = getSectionName(prevSh->name, 8, stringTable, stringTableSize);
                }
            }
        }
        
        if (sec.name.length() > 0 && sec.name[0] == '/' && !stringTable) {
            uint32_t strSecNum = 0;
            std::string numStr;
            for (size_t k = 1; k < 8 && k < 8; k++) {
                if (sec.name[k] >= '0' && sec.name[k] <= '9') {
                    numStr += sec.name[k];
                } else {
                    break;
                }
            }
            if (!numStr.empty()) {
                strSecNum = std::stoul(numStr);
            }
            
            if (strSecNum > 0 && strSecNum < numSections) {
                const SectionHeader* strSec = reinterpret_cast<const SectionHeader*>(data + sectionOffset + strSecNum * 40);
                if (strSec->pointer_to_raw_data > 0 && strSec->size_of_raw_data > 0 && strSec->pointer_to_raw_data + strSec->size_of_raw_data <= len) {
                    stringTable = reinterpret_cast<const char*>(data + strSec->pointer_to_raw_data);
                    stringTableSize = strSec->size_of_raw_data;
                }
            }
        }
        
        sec.virtualAddress = formatHex(sh->virtual_address);
        sec.virtualSize = formatHex(sh->virtual_size);
        sec.rawSize = formatHex(sh->size_of_raw_data);
        
        size_t rawStart = sh->pointer_to_raw_data;
        size_t rawLen = sh->size_of_raw_data;
        if (rawStart + rawLen <= len && rawLen > 0) {
            sec.entropy = calculateEntropy(data + rawStart, rawLen);
        } else {
            sec.entropy = 0.0;
        }
        
        sec.permissions = getPEPermissions(sh->characteristics);
        
        if (sec.entropy > 7.0 && sh->size_of_raw_data > 1024) {
            sec.suspiciousFlags = "High entropy (possibly packed)";
        }
        if ((sh->characteristics & 0x20000000) && (sh->characteristics & 0x40000000)) {
            if (!sec.suspiciousFlags.empty()) sec.suspiciousFlags += "; ";
            sec.suspiciousFlags += "Execute + Write (suspicious)";
        }
        
        info.sections.push_back(sec);
    }
    
    if (importRva > 0 && importSize > 0) {
        uint32_t importOffset = rvaToFileOffset(importRva, data, len, numSections, sectionOffset);
        if (importOffset > 0 && importOffset + sizeof(ImportDescriptor) <= len) {
            const ImportDescriptor* importDesc = reinterpret_cast<const ImportDescriptor*>(data + importOffset);
            
            while (importDesc->name_rva > 0) {
                ImportInfo import;
                import.dllName = readStringFromRVA(data, len, importDesc->name_rva);
                
                uint32_t funcListOffset = 0;
                if (importDesc->original_first_thunk > 0) {
                    funcListOffset = rvaToFileOffset(importDesc->original_first_thunk, data, len, numSections, sectionOffset);
                } else if (importDesc->first_thunk > 0) {
                    funcListOffset = rvaToFileOffset(importDesc->first_thunk, data, len, numSections, sectionOffset);
                }
                
                if (funcListOffset > 0 && funcListOffset < len) {
                    const uint32_t* thunk = reinterpret_cast<const uint32_t*>(data + funcListOffset);
                    while (*thunk != 0) {
                        FunctionInfo func;
                        if (is64bit) {
                            const uint64_t* thunk64 = reinterpret_cast<const uint64_t*>(thunk);
                            if ((*thunk64 & 0x8000000000000000ULL) == 0) {
                                uint32_t nameRva = static_cast<uint32_t>(*thunk64);
                                func.name = readStringFromRVA(data, len, nameRva + 2);
                            } else {
                                func.address = formatHex(*thunk64 & 0x7FFFFFFFFFFFFFFFULL);
                            }
                        } else {
                            if ((*thunk & 0x80000000ULL) == 0) {
                                uint32_t nameRva = *thunk;
                                func.name = readStringFromRVA(data, len, nameRva + 2);
                            } else {
                                func.address = formatHex(*thunk & 0x7FFFFFFF);
                            }
                        }
                        
                        if (!func.name.empty()) {
                            if (func.name.find("VirtualAlloc") != std::string::npos ||
                                func.name.find("VirtualProtect") != std::string::npos ||
                                func.name.find("CreateRemoteThread") != std::string::npos ||
                                func.name.find("WriteProcessMemory") != std::string::npos ||
                                func.name.find("GetProcAddress") != std::string::npos ||
                                func.name.find("LoadLibrary") != std::string::npos ||
                                func.name.find("WinExec") != std::string::npos ||
                                func.name.find("ShellExecute") != std::string::npos ||
                                func.name.find("CreateProcess") != std::string::npos) {
                                func.suspicious = true;
                            }
                            import.functions.push_back(func);
                        }
                        
                        if (is64bit) {
                            thunk = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(thunk) + 8);
                        } else {
                            thunk++;
                        }
                    }
                }
                
                if (!import.dllName.empty()) {
                    info.imports.push_back(import);
                }
                
                importDesc++;
            }
        }
    }
}