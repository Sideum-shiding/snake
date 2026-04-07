#include "../include/math.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Math {

bool WorldToScreen(const Vector3& world, Vector2& screen, const ViewMatrix& matrix, int screenW, int screenH) {
    // Вычисление clip-space координат через ViewMatrix
    float w = matrix.m[3][0] * world.x + matrix.m[3][1] * world.y + matrix.m[3][2] * world.z + matrix.m[3][3];

    // Если w < 0.01, точка за камерой
    if (w < 0.01f) {
        return false;
    }

    float invW = 1.0f / w;

    float x = matrix.m[0][0] * world.x + matrix.m[0][1] * world.y + matrix.m[0][2] * world.z + matrix.m[0][3];
    float y = matrix.m[1][0] * world.x + matrix.m[1][1] * world.y + matrix.m[1][2] * world.z + matrix.m[1][3];

    x *= invW;
    y *= invW;

    // Преобразование из NDC [-1, 1] в экранные координаты
    screen.x = (screenW * 0.5f) + (x * screenW * 0.5f);
    screen.y = (screenH * 0.5f) - (y * screenH * 0.5f);

    return true;
}

Vector3 CalculateAngle(const Vector3& source, const Vector3& destination) {
    Vector3 delta = {
        destination.x - source.x,
        destination.y - source.y,
        destination.z - source.z
    };

    float hypotenuse = std::sqrt(delta.x * delta.x + delta.y * delta.y);

    Vector3 angles;
    // Pitch (вверх/вниз)
    angles.x = -std::atan2(delta.z, hypotenuse) * (180.0f / static_cast<float>(M_PI));
    // Yaw (влево/вправо)
    angles.y = std::atan2(delta.y, delta.x) * (180.0f / static_cast<float>(M_PI));
    // Roll всегда 0
    angles.z = 0.0f;

    NormalizeAngles(angles);
    return angles;
}

void NormalizeAngles(Vector3& angles) {
    // Pitch: [-89, 89]
    if (angles.x > 89.0f) angles.x = 89.0f;
    if (angles.x < -89.0f) angles.x = -89.0f;

    // Yaw: [-180, 180]
    while (angles.y > 180.0f) angles.y -= 360.0f;
    while (angles.y < -180.0f) angles.y += 360.0f;

    // Roll: 0
    angles.z = 0.0f;
}

float Distance(const Vector3& a, const Vector3& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float GetFov(const Vector3& viewAngles, const Vector3& aimAngles) {
    Vector3 delta = {
        aimAngles.x - viewAngles.x,
        aimAngles.y - viewAngles.y,
        0.0f
    };
    NormalizeAngles(delta);
    return std::sqrt(delta.x * delta.x + delta.y * delta.y);
}

} // namespace Math
