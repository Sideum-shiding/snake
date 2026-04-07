#pragma once
#include <string>

namespace Config {
    // Сохранить текущие настройки в JSON-файл
    bool Save(const std::string& path);
    // Загрузить настройки из JSON-файла
    bool Load(const std::string& path);
}
