#pragma once
#include <d3d11.h>

// Типы оригинальных функций для хуков
using FnPresent    = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
using FnResizeBuffers = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

namespace Hooks {
    // Инициализация MinHook и установка хуков на Present/ResizeBuffers
    bool Setup();
    // Снятие хуков и очистка
    void Cleanup();

    // Хуки DX11
    HRESULT __stdcall hkPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags);
    HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount,
                                       UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags);

    // Оригиналы
    inline FnPresent       oPresent       = nullptr;
    inline FnResizeBuffers oResizeBuffers = nullptr;
}
