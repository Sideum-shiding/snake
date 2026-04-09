#include <new>
#include <thread>
#include <chrono>
#include <windows.h>

#include "../include/hooks.h"
#include "../include/settings.h"
#include "../include/config.h"

// Глобальный хэндл модуля DLL
HMODULE g_Module = nullptr;

// Основной поток инициализации
void MainThread(HMODULE hModule) {
    // Ждем загрузки нужных модулей (client.dll, engine2.dll)
    while (!GetModuleHandleA("client.dll") || !GetModuleHandleA("engine2.dll")) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Небольшая задержка для стабильности
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Загрузка конфигурации (если файл существует)
    Config::Load(g_Settings.configPath);

    // Установка хуков на DX11 Present/ResizeBuffers
    if (!Hooks::Setup()) {
        // Если хуки не установились, выгружаем DLL
        FreeLibraryAndExitThread(hModule, 0);
        return;
    }

    // Основной цикл: ожидание нажатия END для выгрузки
    while (!GetAsyncKeyState(VK_END)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Сохранение конфигурации перед выгрузкой
    Config::Save(g_Settings.configPath);

    // Очистка хуков
    Hooks::Cleanup();

    // Выгрузка DLL
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_Module = hModule;
        DisableThreadLibraryCalls(hModule);

        // Запуск основного потока
        HANDLE hThread = CreateThread(nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread),
            hModule, 0, nullptr);

        if (hThread) {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}
