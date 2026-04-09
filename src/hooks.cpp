#include "../include/hooks.h"
#include "../include/renderer.h"
#include "../include/settings.h"
#include "../include/features/visuals.h"
#include "../include/features/combat.h"
#include "../include/features/misc.h"
#include "../include/memory.h"
#include "../include/sdk/offsets.hpp"

#include "../thirdparty/minhook/include/MinHook.h"

#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace Hooks {

// Получение адреса функции Present из vtable IDXGISwapChain
static void* GetSwapChainVTable() {
    // Создаем временное окно и swap chain для получения vtable
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0, 0,
                      GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
                      L"DummyDX11", nullptr };
    RegisterClassEx(&wc);

    HWND hWnd = CreateWindow(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                              0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &scd, &swapChain, &device, &featureLevel, &context);

    void* vtable = nullptr;
    if (SUCCEEDED(hr)) {
        // vtable swapChain
        vtable = *reinterpret_cast<void**>(swapChain);
        swapChain->Release();
        device->Release();
        context->Release();
    }

    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return vtable;
}

bool Setup() {
    if (MH_Initialize() != MH_OK) {
        return false;
    }

    // Получаем vtable IDXGISwapChain
    void* vtable = GetSwapChainVTable();
    if (!vtable) {
        return false;
    }

    // Present находится по индексу 8, ResizeBuffers по индексу 13
    auto* vfunc = static_cast<void**>(vtable);

    // Хук на Present (индекс 8)
    if (MH_CreateHook(vfunc[8], &hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK) {
        return false;
    }

    // Хук на ResizeBuffers (индекс 13)
    if (MH_CreateHook(vfunc[13], &hkResizeBuffers, reinterpret_cast<void**>(&oResizeBuffers)) != MH_OK) {
        return false;
    }

    // Активация всех хуков
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        return false;
    }

    return true;
}

void Cleanup() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    Renderer::Cleanup();
}

HRESULT __stdcall hkPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
    if (!swapChain) {
        return oPresent(swapChain, syncInterval, flags);
    }

    // Инициализация ImGui при первом вызове
    if (!Renderer::g_Initialized) {
        if (!Renderer::Init(swapChain)) {
            return oPresent(swapChain, syncInterval, flags);
        }
    }

    // Защита от отрисовки при сворачивании или загрузке карты
    if (!Renderer::g_Device || !Renderer::g_Context || !Renderer::g_RenderTarget) {
        return oPresent(swapChain, syncInterval, flags);
    }

    uintptr_t clientBase = Memory::GetModuleBase("client.dll");
    bool isInGame = false;
    
    if (clientBase) {
        uintptr_t localPawn = Memory::Read<uintptr_t>(
            clientBase + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
        uintptr_t entityList = Memory::Read<uintptr_t>(
            clientBase + cs2_dumper::offsets::client_dll::dwEntityList);
        
        // Строгая проверка, чтобы не заблокировать поток при загрузке карты/потере контекста
        if (localPawn != 0 && entityList != 0) {
            isInGame = true;
        }
    }

    // Начало кадра ImGui
    Renderer::BeginFrame();

    // Отрисовка меню доступна всегда
    if (g_Settings.menu.open) {
        Renderer::DrawMenu();
    }

    // Вызов фич (в т.ч. ESP/Aimbot) только если игра и данные сущностей полностью валидны и мы в матче
    if (isInGame) {
        if (g_Settings.visuals.enabled) {
            Features::Visuals::Run();
        }
        if (g_Settings.aimbot.enabled) {
            Features::Combat::RunAimbot();
        }
        if (g_Settings.triggerbot.enabled) {
            Features::Combat::RunTriggerbot();
        }
        if (g_Settings.misc.bhop) {
            Features::Misc::RunBhop();
        }

        Features::Misc::RunThirdPerson();
        Features::Misc::RunAspectRatio();
    }

    // Завершение кадра
    Renderer::EndFrame();

    return oPresent(swapChain, syncInterval, flags);
}

HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount,
                                   UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags) {
    // При изменении размера буфера нужно сбросить render target
    if (Renderer::g_RenderTarget) {
        Renderer::g_RenderTarget->Release();
        Renderer::g_RenderTarget = nullptr;
    }

    return oResizeBuffers(swapChain, bufferCount, width, height, newFormat, swapChainFlags);
}

} // namespace Hooks
