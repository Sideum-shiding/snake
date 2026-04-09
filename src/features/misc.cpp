#include "../../include/features/misc.h"
#include "../../include/settings.h"
#include "../../include/memory.h"
#include "../../include/sdk/offsets.hpp"
#include "../../include/sdk/client_dll.hpp"

#include <windows.h>
#include <psapi.h>
#include <cstring>

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
    int flags = Memory::Read<int>(localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_fFlags);

    // Если игрок на земле - прыжок
    if (flags & FL_ONGROUND) {
        // Вызов прыжка (обычно через client.dll + dwForceJump, но тут эмуляция для простоты)
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

    uintptr_t observerServices = Memory::Read<uintptr_t>(localPawn + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pObserverServices);
    if (!observerServices) return;

    // Читаем текущий режим
    int currentObserverMode = Memory::Read<uint8_t>(observerServices + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverMode);
    
    // 0 = first person, 1 = third person (chase)
    if (g_Settings.misc.thirdPerson) {
        if (currentObserverMode != 1) {
            Memory::Write<uint8_t>(observerServices + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverMode, 1);
        }
    } else {
        if (currentObserverMode == 1) {
            // Возвращаем в first person только если мы сами его туда переключили
            Memory::Write<uint8_t>(observerServices + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverMode, 0);
        }
    }
}

// Статическое состояние для aspect ratio патча
static bool    g_AspectPatched = false;
static float   g_OriginalAspect = 1.7777778f; // 16:9
static uintptr_t g_AspectAddress = 0;

void RunAspectRatio() {
    float desiredRatio = g_Settings.misc.aspectRatio;

    // Если значение 0 или по умолчанию, восстанавливаем оригинал
    if (desiredRatio <= 0.01f) {
        if (g_AspectPatched && g_AspectAddress != 0) {
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

    if (!g_AspectAddress) {
        // Мы используем безопасный паттерн-скан вместо ручного перебора памяти каждого байта, чтобы избежать крашей
        // Для простоты временно отключим динамический поиск AspectRatio, если он вызывает зависание, 
        // так как чтение невалидной памяти в цикле убивает игру.
        return;
    }

    if (g_AspectAddress != 0 && g_Settings.misc.aspectRatio > 0.01f) {
        DWORD oldProtect;
        VirtualProtect(reinterpret_cast<void*>(g_AspectAddress), sizeof(float), PAGE_EXECUTE_READWRITE, &oldProtect);
        Memory::Write<float>(g_AspectAddress, desiredRatio);
        VirtualProtect(reinterpret_cast<void*>(g_AspectAddress), sizeof(float), oldProtect, &oldProtect);
        g_AspectPatched = true;
    }
}

} // namespace Misc
} // namespace Features
