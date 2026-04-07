#pragma once
#include <d3d11.h>

namespace Renderer {
    // Инициализация ImGui с DX11 контекстом
    bool Init(IDXGISwapChain* swapChain);
    // Начало нового кадра ImGui
    void BeginFrame();
    // Завершение кадра и отрисовка
    void EndFrame();
    // Очистка ресурсов
    void Cleanup();
    // Отрисовка главного меню
    void DrawMenu();

    // Состояние рендерера
    inline bool              g_Initialized = false;
    inline ID3D11Device*         g_Device      = nullptr;
    inline ID3D11DeviceContext*  g_Context     = nullptr;
    inline ID3D11RenderTargetView* g_RenderTarget = nullptr;
}
