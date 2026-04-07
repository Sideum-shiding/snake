#include "../include/renderer.h"
#include "../include/settings.h"
#include "../include/config.h"

#include "../thirdparty/imgui/imgui.h"
#include "../thirdparty/imgui/backends/imgui_impl_win32.h"
#include "../thirdparty/imgui/backends/imgui_impl_dx11.h"

#include <d3d11.h>

// Forward declare для обработки WndProc ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Оригинальная WndProc окна
static WNDPROC g_OriginalWndProc = nullptr;
static HWND    g_Hwnd            = nullptr;

// Наша WndProc-обертка
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Переключение меню по INSERT
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        g_Settings.menu.open = !g_Settings.menu.open;
    }

    // Если меню открыто, перехватываем ввод для ImGui
    if (g_Settings.menu.open) {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        return TRUE;
    }

    return CallWindowProc(g_OriginalWndProc, hWnd, msg, wParam, lParam);
}

namespace Renderer {

bool Init(IDXGISwapChain* swapChain) {
    // Получаем Device и Context из SwapChain
    HRESULT hr = swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_Device));
    if (FAILED(hr)) return false;

    g_Device->GetImmediateContext(&g_Context);

    // Получаем HWND из описания SwapChain
    DXGI_SWAP_CHAIN_DESC desc;
    swapChain->GetDesc(&desc);
    g_Hwnd = desc.OutputWindow;

    // Создаем render target view
    ID3D11Texture2D* backBuffer = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (backBuffer) {
        g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RenderTarget);
        backBuffer->Release();
    }

    // Инициализация ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // Не сохраняем imgui.ini

    // Темная тема
    ImGui::StyleColorsDark();

    // Стилизация
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding   = 6.0f;
    style.FrameRounding    = 4.0f;
    style.GrabRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowPadding    = ImVec2(10, 10);
    style.FramePadding     = ImVec2(6, 4);

    // Инициализация бэкендов
    ImGui_ImplWin32_Init(g_Hwnd);
    ImGui_ImplDX11_Init(g_Device, g_Context);

    // Подмена WndProc
    g_OriginalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(g_Hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));

    g_Initialized = true;
    return true;
}

void BeginFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void EndFrame() {
    ImGui::Render();

    // Привязка render target
    g_Context->OMSetRenderTargets(1, &g_RenderTarget, nullptr);

    // Отрисовка ImGui поверх сцены
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Cleanup() {
    // Восстанавливаем оригинальный WndProc
    if (g_OriginalWndProc && g_Hwnd) {
        SetWindowLongPtr(g_Hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_OriginalWndProc));
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_RenderTarget) { g_RenderTarget->Release(); g_RenderTarget = nullptr; }
    if (g_Context)      { g_Context->Release();      g_Context = nullptr; }
    if (g_Device)       { g_Device->Release();        g_Device = nullptr; }

    g_Initialized = false;
}

void DrawMenu() {
    ImGui::SetNextWindowSize(ImVec2(520, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug Overlay", &g_Settings.menu.open, ImGuiWindowFlags_NoCollapse);

    // Вкладки
    if (ImGui::BeginTabBar("##tabs")) {
        // === Вкладка Aimbot ===
        if (ImGui::BeginTabItem("Aimbot")) {
            g_Settings.menu.activeTab = 0;

            ImGui::Checkbox("Enable Aimbot", &g_Settings.aimbot.enabled);
            ImGui::SliderFloat("FOV", &g_Settings.aimbot.fov, 1.0f, 30.0f, "%.1f");
            ImGui::SliderFloat("Smooth", &g_Settings.aimbot.smooth, 1.0f, 20.0f, "%.1f");
            ImGui::Checkbox("Visible Only", &g_Settings.aimbot.visibleOnly);
            ImGui::Combo("Hitbox", &g_Settings.aimbot.hitbox, "Head\0Neck\0Chest\0");

            ImGui::Separator();
            ImGui::Checkbox("Enable Triggerbot", &g_Settings.triggerbot.enabled);
            ImGui::SliderInt("Trigger Delay (ms)", &g_Settings.triggerbot.delay_ms, 0, 200);

            ImGui::EndTabItem();
        }

        // === Вкладка Visuals ===
        if (ImGui::BeginTabItem("Visuals")) {
            g_Settings.menu.activeTab = 1;

            ImGui::Checkbox("Enable ESP", &g_Settings.visuals.enabled);
            ImGui::Checkbox("2D Box", &g_Settings.visuals.box);
            ImGui::Checkbox("Health Bar", &g_Settings.visuals.healthBar);
            ImGui::Checkbox("Name", &g_Settings.visuals.name);
            ImGui::Checkbox("Snap Lines", &g_Settings.visuals.snapLines);

            ImGui::ColorEdit4("Box Color", g_Settings.visuals.boxColor);
            ImGui::ColorEdit4("Visible Color", g_Settings.visuals.visibleColor);

            ImGui::EndTabItem();
        }

        // === Вкладка Misc ===
        if (ImGui::BeginTabItem("Misc")) {
            g_Settings.menu.activeTab = 2;

            ImGui::Checkbox("Bunny Hop", &g_Settings.misc.bhop);
            ImGui::Checkbox("Third Person", &g_Settings.misc.thirdPerson);
            ImGui::SliderFloat("Aspect Ratio", &g_Settings.misc.aspectRatio, 0.0f, 3.0f, "%.2f");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("0.00 = default (disabled)");
            }

            ImGui::EndTabItem();
        }

        // === Вкладка Configs ===
        if (ImGui::BeginTabItem("Configs")) {
            g_Settings.menu.activeTab = 3;

            static char cfgPath[256] = "config.json";
            ImGui::InputText("Config Path", cfgPath, sizeof(cfgPath));

            if (ImGui::Button("Save Config")) {
                Config::Save(cfgPath);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Config")) {
                Config::Load(cfgPath);
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace Renderer
