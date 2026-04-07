#include "../../include/features/visuals.h"
#include "../../include/settings.h"
#include "../../include/memory.h"
#include "../../include/math.h"
#include "../../include/sdk/offsets.hpp"
#include "../../include/sdk/sdk.h"

#include "../../thirdparty/imgui/imgui.h"

namespace Features {
namespace Visuals {

// Оффсеты для полей сущности
constexpr std::ptrdiff_t m_iHealth        = 0x354;
constexpr std::ptrdiff_t m_iTeamNum       = 0x3E3;
constexpr std::ptrdiff_t m_pGameSceneNode = 0x338;
constexpr std::ptrdiff_t m_vOldOrigin     = 0x1588;
constexpr std::ptrdiff_t m_hPlayerPawn    = 0x80C;
constexpr std::ptrdiff_t m_bSpottedByMask = 0x16C4;
constexpr std::ptrdiff_t m_sSanitizedPlayerName = 0x770;

// Высота модели игрока в юнитах (приблизительно)
constexpr float PLAYER_HEIGHT = 72.0f;

void Run() {
    if (!g_Settings.visuals.enabled) return;

    auto* drawList = ImGui::GetBackgroundDrawList();
    if (!drawList) return;

    // Получаем базовый адрес client.dll
    uintptr_t clientBase = Memory::GetModuleBase("client.dll");
    if (!clientBase) return;

    // Получаем экранные размеры из engine2.dll
    uintptr_t engineBase = Memory::GetModuleBase("engine2.dll");
    if (!engineBase) return;

    int screenW = Memory::Read<int>(engineBase + cs2_dumper::offsets::engine2_dll::dwWindowWidth);
    int screenH = Memory::Read<int>(engineBase + cs2_dumper::offsets::engine2_dll::dwWindowHeight);
    if (screenW <= 0 || screenH <= 0) return;

    // Читаем ViewMatrix
    ViewMatrix viewMatrix = Memory::Read<ViewMatrix>(
        clientBase + cs2_dumper::offsets::client_dll::dwViewMatrix);

    // Читаем LocalPlayerPawn
    uintptr_t localPawn = Memory::Read<uintptr_t>(
        clientBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn) return;

    int localTeam = Memory::Read<int>(localPawn + m_iTeamNum);

    // Получаем EntityList
    uintptr_t entityList = Memory::Read<uintptr_t>(
        clientBase + cs2_dumper::offsets::client_dll::dwEntityList);
    if (!entityList) return;

    // Итерация по контроллерам (максимум 64 игрока)
    for (int i = 1; i <= 64; ++i) {
        // Получаем entry из EntityList
        uintptr_t listEntry = Memory::Read<uintptr_t>(entityList + (8 * ((i & 0x7FFF) >> 9) + 16));
        if (!listEntry) continue;

        // Получаем контроллер
        uintptr_t controller = Memory::Read<uintptr_t>(listEntry + 120 * (i & 0x1FF));
        if (!controller) continue;

        // Получаем хэндл pawn из контроллера
        uint32_t pawnHandle = Memory::Read<uint32_t>(controller + m_hPlayerPawn);
        if (pawnHandle == 0xFFFFFFFF) continue;

        // Получаем Pawn
        uintptr_t pawn = GameEntity::GetPawnFromHandle(entityList, pawnHandle);
        if (!pawn || pawn == localPawn) continue;

        // Проверка здоровья
        int health = Memory::Read<int>(pawn + m_iHealth);
        if (health <= 0 || health > 100) continue;

        // Проверка команды (не рисуем союзников)
        int team = Memory::Read<int>(pawn + m_iTeamNum);
        if (team == localTeam) continue;

        // Получаем GameSceneNode для позиции
        uintptr_t sceneNode = Memory::Read<uintptr_t>(pawn + m_pGameSceneNode);
        if (!sceneNode) continue;

        // Позиция ног (m_vOldOrigin находится в SceneNode)
        Vector3 origin = Memory::Read<Vector3>(sceneNode + m_vOldOrigin);

        // Позиция головы (прибавляем высоту)
        Vector3 headPos = { origin.x, origin.y, origin.z + PLAYER_HEIGHT };

        // Проекция на экран
        Vector2 screenFeet, screenHead;
        if (!Math::WorldToScreen(origin, screenFeet, viewMatrix, screenW, screenH)) continue;
        if (!Math::WorldToScreen(headPos, screenHead, viewMatrix, screenW, screenH)) continue;

        // Проверка видимости через m_bSpottedByMask
        uint32_t spottedMask = Memory::Read<uint32_t>(pawn + m_bSpottedByMask);
        bool isVisible = (spottedMask & (1 << (i - 1))) != 0; // Упрощенная проверка

        // Выбор цвета: зеленый если видим, красный если нет
        ImU32 boxColor;
        if (isVisible) {
            boxColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
                g_Settings.visuals.visibleColor[0],
                g_Settings.visuals.visibleColor[1],
                g_Settings.visuals.visibleColor[2],
                g_Settings.visuals.visibleColor[3]));
        } else {
            boxColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
                g_Settings.visuals.boxColor[0],
                g_Settings.visuals.boxColor[1],
                g_Settings.visuals.boxColor[2],
                g_Settings.visuals.boxColor[3]));
        }

        // Расчет размеров бокса
        float boxHeight = screenFeet.y - screenHead.y;
        float boxWidth  = boxHeight * 0.4f;

        float boxLeft   = screenHead.x - boxWidth * 0.5f;
        float boxRight  = screenHead.x + boxWidth * 0.5f;
        float boxTop    = screenHead.y;
        float boxBottom = screenFeet.y;

        // === 2D Box ===
        if (g_Settings.visuals.box) {
            drawList->AddRect(
                ImVec2(boxLeft, boxTop),
                ImVec2(boxRight, boxBottom),
                boxColor, 0.0f, 0, 1.5f);
        }

        // === Health Bar (слева от бокса) ===
        if (g_Settings.visuals.healthBar) {
            float barWidth   = 3.0f;
            float barLeft    = boxLeft - barWidth - 2.0f;
            float barHeight  = boxHeight;
            float healthFrac = static_cast<float>(health) / 100.0f;

            // Фон полоски здоровья (темный)
            drawList->AddRectFilled(
                ImVec2(barLeft, boxTop),
                ImVec2(barLeft + barWidth, boxBottom),
                IM_COL32(0, 0, 0, 180));

            // Цвет по здоровью: зеленый -> желтый -> красный
            ImU32 healthColor;
            if (health > 60) {
                healthColor = IM_COL32(0, 255, 0, 255);
            } else if (health > 30) {
                healthColor = IM_COL32(255, 255, 0, 255);
            } else {
                healthColor = IM_COL32(255, 0, 0, 255);
            }

            // Полоска снизу вверх
            float filledTop = boxBottom - barHeight * healthFrac;
            drawList->AddRectFilled(
                ImVec2(barLeft, filledTop),
                ImVec2(barLeft + barWidth, boxBottom),
                healthColor);
        }

        // === Snap Lines (от низа экрана к ногам) ===
        if (g_Settings.visuals.snapLines) {
            drawList->AddLine(
                ImVec2(static_cast<float>(screenW) / 2.0f, static_cast<float>(screenH)),
                ImVec2(screenFeet.x, screenFeet.y),
                boxColor, 1.0f);
        }
    }
}

} // namespace Visuals
} // namespace Features
