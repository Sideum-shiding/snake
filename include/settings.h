#pragma once
#include <string>

// Глобальная структура настроек всех модулей
struct Settings {
    // === Aimbot ===
    struct {
        bool enabled       = false;
        float fov          = 5.0f;    // Максимальный угол захвата (градусы)
        float smooth       = 3.0f;    // Плавность наведения (1 = мгновенно)
        bool visibleOnly   = true;    // Стрелять только по видимым
        int  hitbox        = 0;       // 0 = head, 1 = neck, 2 = chest
    } aimbot;

    // === Triggerbot ===
    struct {
        bool enabled       = false;
        int  delay_ms      = 0;       // Задержка перед выстрелом (мс)
        bool friendlyFire  = false;   // Стрелять по союзникам
    } triggerbot;

    // === Visuals / ESP ===
    struct {
        bool enabled       = false;
        bool box           = true;    // 2D-бокс вокруг врагов
        bool healthBar     = true;    // Полоска здоровья
        bool name          = true;    // Имя над боксом
        bool snapLines     = false;   // Линии от низа экрана до бокса
        float boxColor[4]  = { 1.f, 0.f, 0.f, 1.f };       // Красный по умолчанию
        float visibleColor[4] = { 0.f, 1.f, 0.f, 1.f };    // Зеленый при видимости
    } visuals;

    // === Misc ===
    struct {
        bool bhop          = false;
        bool thirdPerson   = false;
        float aspectRatio  = 0.0f;    // 0 = не менять; иначе кастомное значение
    } misc;

    // === Menu ===
    struct {
        bool open          = true;    // Меню открыто
        int  activeTab     = 0;       // Текущая вкладка (0-3)
    } menu;

    // === Config ===
    std::string configPath = "config.json";
};

// Глобальный объект настроек (определен в config.cpp)
extern Settings g_Settings;
