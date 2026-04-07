#pragma once
#include <math.h>
#include <stdlib.h>
#include <cstdint>
#include <string>
#include <windows.h>

namespace Memory {
    // Получить базовый адрес модуля по имени
    uintptr_t GetModuleBase(const std::string& moduleName);

    // Чтение произвольного типа из адреса (внутри процесса)
    template <typename T>
    T Read(uintptr_t address) {
        return *reinterpret_cast<T*>(address);
    }

    // Запись произвольного типа по адресу
    template <typename T>
    void Write(uintptr_t address, const T& value) {
        *reinterpret_cast<T*>(address) = value;
    }

    // Поиск паттерна байтов в модуле (IDA-стиль, "48 8B ? ? 90")
    uintptr_t PatternScan(uintptr_t moduleBase, size_t moduleSize, const std::string& pattern);
}
