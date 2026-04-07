#include "../include/memory.h"
#include <psapi.h>
#include <vector>
#include <sstream>

namespace Memory {

uintptr_t GetModuleBase(const std::string& moduleName) {
    HMODULE hModule = GetModuleHandleA(moduleName.c_str());
    return reinterpret_cast<uintptr_t>(hModule);
}

// Вспомогательная функция: парсинг IDA-стиля паттерна в байты и маску
static void ParsePattern(const std::string& pattern, std::vector<int>& bytes) {
    std::istringstream stream(pattern);
    std::string token;

    while (stream >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back(-1); // Wildcard
        } else {
            bytes.push_back(static_cast<int>(std::stoul(token, nullptr, 16)));
        }
    }
}

uintptr_t PatternScan(uintptr_t moduleBase, size_t moduleSize, const std::string& pattern) {
    std::vector<int> patternBytes;
    ParsePattern(pattern, patternBytes);

    const size_t patternSize = patternBytes.size();
    const auto scanBytes = reinterpret_cast<uint8_t*>(moduleBase);

    for (size_t i = 0; i < moduleSize - patternSize; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternSize; ++j) {
            if (patternBytes[j] != -1 && scanBytes[i + j] != static_cast<uint8_t>(patternBytes[j])) {
                found = false;
                break;
            }
        }
        if (found) {
            return moduleBase + i;
        }
    }

    return 0;
}

} // namespace Memory
