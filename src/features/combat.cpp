#include <cmath>

#include "../../include/features/combat.h"
#include "../../include/settings.h"
#include "../../include/memory.h"
#include "../../include/math_utils.h"
#include "../../include/sdk/offsets.hpp"
#include "../../include/sdk/client_dll.hpp"
#include "../../include/sdk/sdk.h"

#include <windows.h>

// Высота глаз (EyePos = Origin + ViewOffset, обычно z ~= 64.06)
constexpr float EYE_HEIGHT = 64.06f;
// Позиция головы (приблизительно, чуть выше глаз)
constexpr float HEAD_HEIGHT = 72.0f;

namespace Features {
namespace Combat {

void RunAimbot() {
    if (!g_Settings.aimbot.enabled) return;

    // Aimbot активируется при зажатой правой кнопке мыши (ADS)
    if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) return;

    uintptr_t clientBase = Memory::GetModuleBase("client.dll");
    if (!clientBase) return;

    // Получаем LocalPlayerPawn
    uintptr_t localPawn = Memory::Read<uintptr_t>(
        clientBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    int localTeam = Memory::Read<int>(localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
    if (localTeam <= 0) return;

    // EyePos = Origin + ViewOffset
    uintptr_t sceneNode = Memory::Read<uintptr_t>(localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
    if (!sceneNode) return;

    Vector3 localOrigin = Memory::Read<Vector3>(sceneNode + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin);
    // Для более точного расчета можно использовать m_vecViewOffset, но для простоты:
    Vector3 eyePos = { localOrigin.x, localOrigin.y, localOrigin.z + EYE_HEIGHT };

    // Текущие углы обзора
    Vector3 viewAngles = Memory::Read<Vector3>(
        clientBase + cs2_dumper::offsets::client_dll::dwViewAngles);

    // EntityList
    uintptr_t entityList = Memory::Read<uintptr_t>(
        clientBase + cs2_dumper::offsets::client_dll::dwEntityList);
    if (!entityList) return;

    // Поиск лучшей цели
    float bestFov = g_Settings.aimbot.fov;
    Vector3 bestAngle = { 0.f, 0.f, 0.f };
    bool targetFound = false;

    for (int i = 1; i <= 64; ++i) {
        uintptr_t listEntry = Memory::Read<uintptr_t>(entityList + (8 * ((i & 0x7FFF) >> 9) + 16));
        if (!listEntry) continue;

        uintptr_t controller = Memory::Read<uintptr_t>(listEntry + 120 * (i & 0x1FF));
        if (!controller) continue;

        uint32_t pawnHandle = Memory::Read<uint32_t>(controller + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
        if (!pawnHandle || pawnHandle == 0xFFFFFFFF) continue;

        uintptr_t pawn = GameEntity::GetPawnFromHandle(entityList, pawnHandle);
        if (!pawn || pawn == localPawn) continue;

        int health = Memory::Read<int>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
        if (health <= 0 || health > 100) continue;

        int team = Memory::Read<int>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        if (team <= 0 || team == localTeam) continue;

        // Позиция цели
        uintptr_t targetScene = Memory::Read<uintptr_t>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
        if (!targetScene) continue;

        Vector3 targetOrigin = Memory::Read<Vector3>(targetScene + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin);

        // Позиция hitbox'а в зависимости от настроек
        float hitboxHeight;
        switch (g_Settings.aimbot.hitbox) {
            case 0: hitboxHeight = HEAD_HEIGHT; break;    // Head
            case 1: hitboxHeight = 60.0f; break;          // Neck
            case 2: hitboxHeight = 45.0f; break;          // Chest
            default: hitboxHeight = HEAD_HEIGHT; break;
        }

        Vector3 targetPos = { targetOrigin.x, targetOrigin.y, targetOrigin.z + hitboxHeight };

        // Расчет углов до цели
        Vector3 aimAngle = Math::CalculateAngle(eyePos, targetPos);

        // Проверка FOV
        float fov = Math::GetFov(viewAngles, aimAngle);
        if (fov < bestFov) {
            bestFov = fov;
            bestAngle = aimAngle;
            targetFound = true;
        }
    }

    // Применяем углы с плавностью (Silent Aim записывает напрямую в viewAngles)
    if (targetFound) {
        Vector3 delta = {
            bestAngle.x - viewAngles.x,
            bestAngle.y - viewAngles.y,
            0.0f
        };
        Math::NormalizeAngles(delta);

        Vector3 smoothed = {
            viewAngles.x + delta.x / g_Settings.aimbot.smooth,
            viewAngles.y + delta.y / g_Settings.aimbot.smooth,
            0.0f
        };
        Math::NormalizeAngles(smoothed);

        // Запись viewAngles обратно в память
        Memory::Write<Vector3>(
            clientBase + cs2_dumper::offsets::client_dll::dwViewAngles, smoothed);
    }
}

static DWORD g_LastTriggerTime = 0;

void RunTriggerbot() {
    if (!g_Settings.triggerbot.enabled) return;

    // Triggerbot активируется при зажатой клавише ALT
    if (!(GetAsyncKeyState(VK_MENU) & 0x8000)) return;

    uintptr_t clientBase = Memory::GetModuleBase("client.dll");
    if (!clientBase) return;

    uintptr_t localPawn = Memory::Read<uintptr_t>(
        clientBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    int localTeam = Memory::Read<int>(localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
    if (localTeam <= 0) return;

    // Читаем m_iIDEntIndex (индекс сущности под прицелом)
    int crosshairEntity = Memory::Read<int>(localPawn + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_iIDEntIndex);
    if (crosshairEntity <= 0 || crosshairEntity > 64) return;

    // Получаем EntityList
    uintptr_t entityList = Memory::Read<uintptr_t>(
        clientBase + cs2_dumper::offsets::client_dll::dwEntityList);
    if (!entityList) return;

    // Проверяем сущность под прицелом
    uintptr_t listEntry = Memory::Read<uintptr_t>(
        entityList + (8 * ((crosshairEntity & 0x7FFF) >> 9) + 16));
    if (!listEntry) return;

    uintptr_t targetController = Memory::Read<uintptr_t>(listEntry + 120 * (crosshairEntity & 0x1FF));
    if (!targetController) return;

    uint32_t pawnHandle = Memory::Read<uint32_t>(targetController + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
    if (!pawnHandle || pawnHandle == 0xFFFFFFFF) return;

    uintptr_t targetPawn = GameEntity::GetPawnFromHandle(entityList, pawnHandle);
    if (!targetPawn) return;

    // Проверка здоровья
    int health = Memory::Read<int>(targetPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
    if (health <= 0 || health > 200) return;

    // Проверка команды (не стреляем по союзникам, если не включен FF)
    int targetTeam = Memory::Read<int>(targetPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
    if (!g_Settings.triggerbot.friendlyFire && targetTeam == localTeam) return;

    // Неблокирующая задержка (вместо sleep_for, замораживавшего игру)
    DWORD currentTime = GetTickCount();
    if (currentTime - g_LastTriggerTime < static_cast<DWORD>(g_Settings.triggerbot.delay_ms + 20)) {
        return; // Еще не прошло достаточно времени
    }

    // Симуляция клика мышью (без sleep)
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    g_LastTriggerTime = currentTime;
}

} // namespace Combat
} // namespace Features
