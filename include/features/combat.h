#pragma once

namespace Features {
namespace Combat {
    // Silent Aim: корректирует viewAngles на ближайшую цель
    void RunAimbot();
    // Triggerbot: автоматический выстрел при наведении прицела на врага
    void RunTriggerbot();
}
}
