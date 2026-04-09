#include "../../include/features/misc.h"
#include "../../include/settings.h"
#include "../../include/memory.h"
#include "../../include/sdk/offsets.hpp"

#include <windows.h>
#include <psapi.h>
#include <cstring>

// Оффсеты
constexpr std::ptrdiff_t m_fFlags         = 0x3EC;
constexpr std::ptrdiff_t m_iObserverMode  = 0x3D4;

// Флаг "на земле"
constexpr int FL_ONGROUND = (1 << 0);

namespace Features {
namespace Misc {

void RunBhop() {
    if (!g_Settings.misc.bhop) return;

    // Проверяем, зажат ли пробел
    if (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) return;

    uintptr_t clientBase = Memory::GetModuleBase("client.dll");
    if (!clientBase) return;

    uintptr_t localPawn = Memory::Read<uintptr_t>(
        clientBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    // Читаем флаги игрока
    int flags = Memory::Read<int>(localPawn + m_fFlags);

    // Если игрок на земле - прыжок
    if (flags & FL_ONGROUND) {
        // Симуляция нажатия и отпускания пробела для прыжка
        keybd_event(VK_SPACE, 0, 0, 0);
        keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
    }
}

// Статическое состояние для отслеживания нажатия кнопки V
static bool g_ThirdPersonKeyPressed = false;

void RunThirdPerson() {
    uintptr_t clientBase = Memory::GetModuleBase("client.dll");
    if (!clientBase) return;

    uintptr_t localPawn = Memory::Read<uintptr_t>(
        clientBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    // Отслеживание нажатия V (toggle, не held)
    bool keyDown = (GetAsyncKeyState('V') & 0x8000) != 0;
    if (keyDown && !g_ThirdPersonKeyPressed) {
        g_ThirdPersonKeyPressed = true;
        g_Settings.misc.thirdPerson = !g_Settings.misc.thirdPerson;
    } else if (!keyDown) {
        g_ThirdPersonKeyPressed = false;
    }

    // Переключение m_iObserverMode
    // 0 = first person, 1 = third person (chase)
    if (g_Settings.misc.thirdPerson) {
        Memory::Write<int>(localPawn + m_iObserverMode, 1);
    } else {
        Memory::Write<int>(localPawn + m_iObserverMode, 0);
    }
}

// Статическое состояние для aspect ratio патча
static bool    g_AspectPatched = false;
static float   g_OriginalAspect = 1.7777778f; // 16:9
static uintptr_t g_AspectAddress = 0;

void RunAspectRatio() {
    float desiredRatio = g_Settings.misc.aspectRatio;

    // Если значение 0 или по умолчанию 16:9, восстанавливаем оригинал
    if (desiredRatio <= 0.01f) {
        if (g_AspectPatched && g_AspectAddress) {
            // Восстанавливаем оригинальное значение
            DWORD oldProtect;
            VirtualProtect(reinterpret_cast<void*>(g_AspectAddress), sizeof(float), PAGE_EXECUTE_READWRITE, &oldProtect);
            Memory::Write<float>(g_AspectAddress, g_OriginalAspect);
            VirtualProtect(reinterpret_cast<void*>(g_AspectAddress), sizeof(float), oldProtect, &oldProtect);
            g_AspectPatched = false;
        }
        return;
    }

    uintptr_t engineBase = Memory::GetModuleBase("engine2.dll");
    if (!engineBase) return;

    // Находим адрес значения 1.77... (16:9) в engine2.dll при первом вызове
    if (!g_AspectAddress) {
        MODULEINFO modInfo = {};
        GetModuleInformation(GetCurrentProcess(),
            reinterpret_cast<HMODULE>(engineBase), &modInfo, sizeof(modInfo));

        // Ищем float 1.7777778 (0x3FE38E39 в IEEE 754)
        // Проходим по памяти модуля и ищем первое совпадение
        const float targetValue = 1.7777778f;
        const auto* scanBytes = reinterpret_cast<const uint8_t*>(engineBase);

        for (size_t i = 0; i < modInfo.SizeOfImage - sizeof(float); i += 4) {
            float value = *reinterpret_cast<const float*>(scanBytes + i);
            // Сравниваем с небольшой погрешностью
            if (value > 1.776f && value < 1.779f) {
                g_AspectAddress = engineBase + i;
                break;
            }
        }
    }

    if (!g_AspectAddress) return;

    // Патчим значение
    DWORD oldProtect;
    VirtualProtect(reinterpret_cast<void*>(g_AspectAddress), sizeof(float), PAGE_EXECUTE_READWRITE, &oldProtect);
    Memory::Write<float>(g_AspectAddress, desiredRatio);
    VirtualProtect(reinterpret_cast<void*>(g_AspectAddress), sizeof(float), oldProtect, &oldProtect);
    g_AspectPatched = true;
}

} // namespace Misc
} // namespace Features
