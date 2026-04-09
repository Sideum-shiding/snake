#include "../include/renderer.h"
#include "../include/settings.h"
#include "../include/config.h"

#include "../thirdparty/imgui/imgui.h"
#include "../thirdparty/imgui/backends/imgui_impl_win32.h"
#include "../thirdparty/imgui/backends/imgui_impl_dx11.h"

#include <d3d11.h>
#include <algorithm>

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

    // Темная тема / Fatality стиль
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 4.0f;
    style.GrabRounding      = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.ChildRounding     = 4.0f;
    style.WindowPadding     = ImVec2(0, 0); // Обнуляем паддинг для отрисовки сайдбара вплотную
    style.FramePadding      = ImVec2(6, 4);
    style.ItemSpacing       = ImVec2(8, 8);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]       = ImColor(15, 15, 15, 255);    // 0x0F0F0F
    colors[ImGuiCol_ChildBg]        = ImColor(20, 20, 20, 255);
    colors[ImGuiCol_Border]         = ImColor(35, 35, 35, 255);
    colors[ImGuiCol_FrameBg]        = ImColor(30, 30, 30, 255);
    colors[ImGuiCol_FrameBgHovered] = ImColor(40, 40, 40, 255);
    colors[ImGuiCol_FrameBgActive]  = ImColor(45, 45, 45, 255);
    colors[ImGuiCol_CheckMark]      = ImColor(138, 43, 226, 255);  // 0x8A2BE2
    colors[ImGuiCol_SliderGrab]     = ImColor(138, 43, 226, 255);
    colors[ImGuiCol_SliderGrabActive]=ImColor(158, 63, 246, 255);
    colors[ImGuiCol_Button]         = ImColor(30, 30, 30, 255);
    colors[ImGuiCol_ButtonHovered]  = ImColor(40, 40, 40, 255);
    colors[ImGuiCol_ButtonActive]   = ImColor(138, 43, 226, 255);
    colors[ImGuiCol_Header]         = ImColor(138, 43, 226, 150);
    colors[ImGuiCol_HeaderHovered]  = ImColor(138, 43, 226, 200);
    colors[ImGuiCol_HeaderActive]   = ImColor(138, 43, 226, 255);
    colors[ImGuiCol_Text]           = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

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
    // Анимация Fade-in / Fade-out (Alpha channel)
    static float alpha = 0.0f;
    if (g_Settings.menu.open) {
        alpha = std::min(alpha + ImGui::GetIO().DeltaTime * 6.0f, 1.0f);
    } else {
        alpha = std::max(alpha - ImGui::GetIO().DeltaTime * 6.0f, 0.0f);
    }

    if (alpha <= 0.01f) return;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

    ImGui::SetNextWindowSize(ImVec2(650, 450), ImGuiCond_Once);
    ImGui::Begin("SnakeMenu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Размеры Sidebar
    float sidebarWidth = 140.0f;
    
    // Отрисовка фона Sidebar'a (темнее основного фона)
    draw->AddRectFilled(pos, ImVec2(pos.x + sidebarWidth, pos.y + size.y), ImColor(12, 12, 12, 255), 4.0f, ImDrawFlags_RoundCornersLeft);
    draw->AddLine(ImVec2(pos.x + sidebarWidth, pos.y), ImVec2(pos.x + sidebarWidth, pos.y + size.y), ImColor(35, 35, 35, 255));

    // Логотип в левом верхнем углу
    draw->AddText(ImGui::GetFont(), 20.0f, ImVec2(pos.x + 35, pos.y + 20), ImColor(138, 43, 226, 255), "SNAKE");

    // Вкладки в Sidebar
    ImGui::SetCursorPos(ImVec2(10, 70));
    ImGui::BeginGroup();
    const char* tabs[] = { "Legit", "Visuals", "Movement", "Misc", "Configs" };
    for (int i = 0; i < 5; ++i) {
        bool isSelected = (g_Settings.menu.activeTab == i);

        // Цвета кнопки вкладок
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.54f, 0.17f, 0.89f, 0.3f)); // Слабо фиолетовый фон
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));      // Белый текст
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.54f, 0.17f, 0.89f, 0.4f)); 
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.54f, 0.17f, 0.89f, 0.5f)); 
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));    // Прозрачный фон
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));      // Серый текст
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.1f, 0.1f, 0.5f)); 
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 0.5f)); 
        }

        if (ImGui::Button(tabs[i], ImVec2(sidebarWidth - 20, 40))) {
            g_Settings.menu.activeTab = i;
        }

        // Если выбрано - рисуем акцентную фиолетовую линию слева
        if (isSelected) {
            ImVec2 btnPos = ImGui::GetItemRectMin();
            ImVec2 btnSize = ImGui::GetItemRectSize();
            draw->AddRectFilled(btnPos, ImVec2(btnPos.x + 3.0f, btnPos.y + btnSize.y), ImColor(138, 43, 226, 255));
        }

        ImGui::PopStyleColor(4);
        ImGui::Spacing();
    }
    ImGui::EndGroup();

    // Правая часть (Main Content)
    ImGui::SetCursorPos(ImVec2(sidebarWidth + 15, 15));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.06f, 0.06f, 0.0f)); // Прозрачный фон
    
    ImGui::BeginChild("MainContent", ImVec2(size.x - sidebarWidth - 30, size.y - 30), false, ImGuiWindowFlags_NoScrollbar);

    switch (g_Settings.menu.activeTab) {
        case 0: // Legit
            ImGui::Columns(2, nullptr, false);
            
            // GroupBox - Aimbot
            ImGui::BeginChild("AimbotGroup", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) { ImGui::Text("Aimbot"); ImGui::EndMenuBar(); }
            ImGui::Spacing();
            ImGui::Checkbox("Enable Aimbot", &g_Settings.aimbot.enabled);
            ImGui::SliderFloat("FOV", &g_Settings.aimbot.fov, 1.0f, 30.0f, "%.1f deg");
            ImGui::SliderFloat("Smooth", &g_Settings.aimbot.smooth, 1.0f, 20.0f, "%.1f");
            ImGui::Checkbox("Visible Only", &g_Settings.aimbot.visibleOnly);
            ImGui::Combo("Hitbox", &g_Settings.aimbot.hitbox, "Head\0Neck\0Chest\0");
            ImGui::EndChild();
            
            ImGui::NextColumn();

            // GroupBox - Triggerbot
            ImGui::BeginChild("TriggerbotGroup", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) { ImGui::Text("Triggerbot"); ImGui::EndMenuBar(); }
            ImGui::Spacing();
            ImGui::Checkbox("Enable Triggerbot", &g_Settings.triggerbot.enabled);
            ImGui::SliderInt("Delay (ms)", &g_Settings.triggerbot.delay_ms, 0, 200);
            ImGui::Checkbox("Friendly Fire", &g_Settings.triggerbot.friendlyFire);
            ImGui::EndChild();
            
            ImGui::Columns(1);
            break;

        case 1: // Visuals
            ImGui::Columns(2, nullptr, false);

            // GroupBox - Player ESP
            ImGui::BeginChild("ESPGroup", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) { ImGui::Text("Player ESP"); ImGui::EndMenuBar(); }
            ImGui::Spacing();
            ImGui::Checkbox("Enable ESP", &g_Settings.visuals.enabled);
            ImGui::Checkbox("Draw Box", &g_Settings.visuals.box);
            ImGui::Checkbox("Draw Name", &g_Settings.visuals.name);
            ImGui::Checkbox("Draw Health", &g_Settings.visuals.healthBar);
            ImGui::Checkbox("Snap Lines", &g_Settings.visuals.snapLines);
            ImGui::EndChild();

            ImGui::NextColumn();

            // GroupBox - ESP Colors
            ImGui::BeginChild("ColorsGroup", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) { ImGui::Text("Colors"); ImGui::EndMenuBar(); }
            ImGui::Spacing();
            ImGui::ColorEdit4("Invisible", g_Settings.visuals.boxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            ImGui::ColorEdit4("Visible", g_Settings.visuals.visibleColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            ImGui::EndChild();

            ImGui::Columns(1);
            break;

        case 2: // Movement
            ImGui::BeginChild("MovementGroup", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) { ImGui::Text("Movement"); ImGui::EndMenuBar(); }
            ImGui::Spacing();
            ImGui::Checkbox("Bunny Hop", &g_Settings.misc.bhop);
            ImGui::EndChild();
            break;

        case 3: // Misc
            ImGui::BeginChild("MiscGroup", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) { ImGui::Text("Miscellaneous"); ImGui::EndMenuBar(); }
            ImGui::Spacing();
            ImGui::Checkbox("Third Person", &g_Settings.misc.thirdPerson);
            ImGui::SliderFloat("Aspect Ratio", &g_Settings.misc.aspectRatio, 0.0f, 3.0f, "%.2f");
            ImGui::SetItemTooltip("Set to 0.00 to disable.");
            ImGui::EndChild();
            break;

        case 4: // Configs
            ImGui::BeginChild("ConfigGroup", ImVec2(0, 0), true, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar()) { ImGui::Text("Configuration"); ImGui::EndMenuBar(); }
            ImGui::Spacing();
            
            static char cfgPath[256] = "config.json";
            ImGui::InputText("Config Path", cfgPath, sizeof(cfgPath));
            
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("Save Config", ImVec2(120, 30))) {
                Config::Save(cfgPath);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Config", ImVec2(120, 30))) {
                Config::Load(cfgPath);
            }
            ImGui::EndChild();
            break;
    }

    ImGui::EndChild(); // MainContent
    ImGui::PopStyleColor();

    ImGui::End(); // SnakeMenu
    ImGui::PopStyleVar(); // alpha
}

} // namespace Renderer
