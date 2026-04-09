#pragma once

#include <cmath>
#include <cstdlib>

#include <windows.h>
#include <cstdint>
#include "../memory.h"

// Базовые структуры для координат
struct Vector3 {
    float x, y, z;
};

struct Vector2 {
    float x, y;
};

// Матрица 4x4 для WorldToScreen (ViewMatrix)
struct ViewMatrix {
    float m[4][4];
};

// Перечисление команд для фильтрации ESP
enum TeamID : int {
    TEAM_NONE = 0,
    TEAM_SPECTATOR = 1,
    TEAM_TERRORIST = 2,
    TEAM_COUNTER_TERRORIST = 3
};

// Класс-обертка для удобного получения Pawn (тела игрока) из EntityList
class GameEntity {
public:
    // Функция для получения адреса Pawn по хэндлу (m_hPlayerPawn)
    static uintptr_t GetPawnFromHandle(uintptr_t entityList, uint32_t handle) {
        if (!entityList || handle == 0xFFFFFFFF) return 0;

        // В CS2 EntityList разбит на чанки по 512 сущностей
        uintptr_t listEntry = Memory::Read<uintptr_t>(entityList + 0x8 * ((handle & 0x7FFF) >> 9) + 16);
        if (!listEntry) return 0;

        return Memory::Read<uintptr_t>(listEntry + 120 * (handle & 0x1FF));
    }
};
