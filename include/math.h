#pragma once
#include "sdk/sdk.h"

namespace Math {
    // Проецирует мировые координаты в экранные через ViewMatrix
    // Возвращает true если точка перед камерой
    bool WorldToScreen(const Vector3& world, Vector2& screen, const ViewMatrix& matrix, int screenW, int screenH);

    // Рассчитывает углы (pitch, yaw) от source до destination
    Vector3 CalculateAngle(const Vector3& source, const Vector3& destination);

    // Нормализация углов в диапазон [-180, 180]
    void NormalizeAngles(Vector3& angles);

    // Расстояние между двумя точками
    float Distance(const Vector3& a, const Vector3& b);

    // FOV между текущим направлением и нужным углом
    float GetFov(const Vector3& viewAngles, const Vector3& aimAngles);
}
