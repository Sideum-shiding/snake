#pragma once
#include <math.h>
#include <stdlib.h>
#include <cstdint>
#include <string>
#include <windows.h>

namespace Memory {
    // Получить базовый адрес модуля по имени
    uintptr_t GetModuleBase(const std::string& moduleName);

    // Чтение произвольного типа из адреса (внутри процесса) с обработкой исключений (SEH)
    template <typename T>
    T Read(uintptr_t address) {
        if (!address) return T{};
        T result{};
        __try {
            result = *reinterpret_cast<T*>(address);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return T{}; // Возвращаем пустое значение при Access Violation
        }
        return result;
    }

    // Запись произвольного типа по адресу с обработкой исключений
    template <typename T>
    void Write(uintptr_t address, const T& value) {
        if (!address) return;
        __try {
            *reinterpret_cast<T*>(address) = value;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // Игнорируем ошибку записи
        }
    }

    // Поиск паттерна байтов в модуле (IDA-стиль, "48 8B ? ? 90")
    uintptr_t PatternScan(uintptr_t moduleBase, size_t moduleSize, const std::string& pattern);
}
