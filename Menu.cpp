#include "Menu.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"
#include "plugin.h"
#include "Shlobj.h"
#include "patch.h"
#include "resource.h"
#include <d3dx9.h>
#include "AnimationFist.h"
#include "base64.h"

extern AnimationFist* g_pFist;

uint32_t           Menu::s_nCurrentPage;
LPDIRECT3DTEXTURE9 Menu::s_pCursorTexture;
HMODULE            Menu::s_hDLL;

void Menu::initMenu(HMODULE hDLL, LPDIRECT3DDEVICE9 pDevice) {
    s_hDLL = hDLL;

    s_pWindowProc = WNDPROC(SetWindowLongW(**(HWND**)0xC17054, GWL_WNDPROC, LONG(WndProcHandler)));
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(**(HWND**)0xC17054);

    char pathFonstwindows[MAX_PATH]{};
    SHGetSpecialFolderPathA(0, pathFonstwindows, CSIDL_FONTS, true);
    _snprintf_s(pathFonstwindows, sizeof(pathFonstwindows) - 1, "%s\\segoeui.ttf", pathFonstwindows);
    ImGui::GetIO().Fonts->AddFontFromFileTTF(pathFonstwindows, 16.5f, NULL,
                                             ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());

    setStyle();

    D3DXCreateTextureFromResourceA(pDevice, s_hDLL, MAKEINTRESOURCEA(IDB_CURSOR), &s_pCursorTexture);

    ImGui_ImplDX9_Init(pDevice);
}

void Menu::uninitMenu() {
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Menu::render() {
    if (!s_bMenuState || *reinterpret_cast<unsigned char*>(0xB7CB49) == 1)
        return;

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    const ImVec2 WINDOW_SIZE{633.0f, 281.0f};
    ImGui::SetNextWindowSize(WINDOW_SIZE);
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::Begin("AnimationFist | Change fist", &s_bMenuState, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar);
    
    // Верхняя панель
    {
        static bool s_isActivePopModal{};
        ImGui::BeginMenuBar();
        {
            if (ImGui::MenuItem(u8"Перезагрузить WebP файлы")) {
                if (g_pFist->getState() != AnimationFist::LOAD_STATE::BUSY) {
                    g_pFist->reloadDec();
                } else {
                    ImGui::OpenPopup("Error: Failed reload gifs");
                    s_isActivePopModal = true;
                }
            }

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
            if (ImGui::BeginPopupModal("Error: Failed reload gifs", &s_isActivePopModal, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar)) {
                ImGui::TextColored({0.79f, 0.0f, 0.0f, 1.0f}, u8"ОШИБКА: Ещё не все WebP файлы загрузились. Пожалуйста, подождите.");

                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();
        }
        ImGui::EndMenuBar();
    }

    /* Проверяем, все ли GIF файлы загрузились */
    if (auto state = g_pFist->getState(); state == AnimationFist::LOAD_STATE::SUCCEEDED) {
        const ImVec2 SIZE_IMAGE_BUTTON{100.0f, 100.0f};

        constexpr float SPACE_SIZE{135.0f};
        constexpr float TEXT_BUTTON_OFFSET{119.0f};

        constexpr int MAX_QUANT_BUTTON{4};
        ImVec2    buttonPos{54.0f, 96.0f};

        auto     lenFilesData = g_pFist->getCountFiles();
        uint32_t maxLenData = lenFilesData / MAX_QUANT_BUTTON + ((lenFilesData % MAX_QUANT_BUTTON) > 0 ? 1 : 0);

        uint32_t lenData = s_nCurrentPage * MAX_QUANT_BUTTON;
        for (uint32_t i{lenData}; i < lenFilesData && i < maxLenData * MAX_QUANT_BUTTON && i < MAX_QUANT_BUTTON * (s_nCurrentPage + 1); i++) {
            ImGui::SetCursorPos(buttonPos);
            AnimationFist::stWebPFileData dt;
            g_pFist->getFileData(dt, i);
            if (ImGui::ImageButton(dt.pDecoder->getTitle((LPDIRECT3DDEVICE9)RwD3D9GetCurrentD3DDevice()), SIZE_IMAGE_BUTTON)) {
                g_pFist->changeNameFist(dt.fileName);
            }

            {
                const int MAX_TEXTTITLE_LEN{16};

                auto &text = dt.fileName;
                text.erase(text.end() - 5, text.end());
                if (text.size() > MAX_TEXTTITLE_LEN) {
                    text.erase(text.begin() + MAX_TEXTTITLE_LEN, text.end());
                    auto ps = text.size() - 1;
                    text.reserve(MAX_TEXTTITLE_LEN + 3);
                    for (auto& c : {'.', '.', '.'}) {
                        text[ps++] = c;
                    }
                }

                ImGui::SetCursorPos({buttonPos.x + SIZE_IMAGE_BUTTON.x / 2 - ImGui::CalcTextSize(text.c_str()).x / 2, buttonPos.y + TEXT_BUTTON_OFFSET});
                ImGui::Text(text.c_str());
            }

            buttonPos.x += SPACE_SIZE;
        }

        const ImVec2 CHANGE_PAGE_BUTTON[2]{
            {18.0f, 127.0f},
            {589.0f, 127.0f}};
        const ImVec2 CHAGNE_PAGE_BUTTON_SIZE{17.0f, 42.0f};

        ImGui::SetCursorPos(CHANGE_PAGE_BUTTON[0]);
        if (ImGui::Button("<", CHAGNE_PAGE_BUTTON_SIZE) && 0 < s_nCurrentPage) {
            s_nCurrentPage--;
        }
        ImGui::SetCursorPos(CHANGE_PAGE_BUTTON[1]);
        if (ImGui::Button(">", CHAGNE_PAGE_BUTTON_SIZE) && s_nCurrentPage < maxLenData - 1) {
            s_nCurrentPage++;
        }
    } else if (state == AnimationFist::LOAD_STATE::BUSY) {
        ImGui::SetCursorPos({118.0f, 137.0f});
        ImGui::TextColored({0.79f, 0.0f, 0.0f, 1.0f}, u8"ВНИМАНИЕ: Идёт загрузка WebP файлов. Пожалуйста, подождите");
    } else if (state == AnimationFist::LOAD_STATE::EMPTY) {
        ImGui::SetCursorPos({118.0f, 137.0f});
        ImGui::TextColored({0.79f, 0.0f, 0.0f, 1.0f}, u8"У вас в папке models\\AnimationFist нет WebP анимаций.");
    } else {
        ImGui::SetCursorPos({118.0f, 137.0f});
        /*
            У вас в папке models\AnimationFist нету WebP анимаций.
            Если это не так, то Свяжитесь со мной, чтобы исправить ошибку.
            VK: vk.com/tim4ukys | Email: tim4ukys.dev@yandex.ru
        */
        static auto s_text = base64_decode("0KMg0LLQsNGBINCyINC/0LDQv9C60LUgIm1vZGVsc1xBbmltYXRpb25GaXN0IiDQvdC10"
            "YIgV2ViUCDRhNCw0LnQu9C+0LIuCgrQldGB0LvQuCDRjdGC0L4g0L3QtSDRgtCw0LosINGC0L4g0KHQstGP0LbQuNGC0LXRgdGMI"
            "NGB0L4g0LzQvdC+0LksINGH0YLQvtCx0Ysg0LjRgdC/0YDQsNCy0LjRgtGMINC+0YjQuNCx0LrRgy4KVks6IHZrLmNvbS90aW00d"
            "Wt5cyB8IEVtYWlsOiB0aW00dWt5cy5kZXZAeWFuZGV4LnJ1");
        ImGui::TextColored({0.79f, 0.0f, 0.0f, 1.0f}, s_text.c_str());
    }


    ImGui::End();

    drawCursor();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void Menu::drawCursor() {
    auto mouserPos = ImGui::GetMousePos();
    ImGui::GetForegroundDrawList()->AddImage(s_pCursorTexture, {mouserPos.x, mouserPos.y}, {mouserPos.x + 21, mouserPos.y + 27});
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool    Menu::s_bMenuState;
WNDPROC Menu::s_pWindowProc;
LRESULT __stdcall Menu::WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    switch (msg) {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if (wParam == VK_END)
            s_bMenuState ^= true;

        break;
    }

    static bool popen = false;

    if (s_bMenuState) {
        if (!popen) {
            show_cursor(true);
        }

        popen = true;
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    } else {
        if (popen) {
            popen = false;
            show_cursor(false);
        }
    }
    return CallWindowProcA(s_pWindowProc, hWnd, msg, wParam, lParam);
}

void Menu::show_cursor(bool show) {
    if (show) {
        patch__fill(0x541DF5, 5u, 0x90);
        patch__fill(0x53F417, 5u, 0x90);
        patch__setRaw(0x53F41F, "\x33\xC0\xf\x84", 4u);
        *(DWORD*)0xB73424 = 0;
        *(DWORD*)0xB73428 = 0;
        ((void (*)())0x541BD0)();
        ((void (*)())0x541DD0)();
        patch__setRaw(0x6194A0, "\xC3", 1u);

        //*(DWORD*)(*(DWORD*)g_sampBase.getAddress(0x26E8F4) + 0x61) = 2;

        ImGui::GetIO().MouseDrawCursor = true;
    } else {
        plugin::patch::SetRaw(0x541DF5, (void*)"\xE8\x46\xF3\xFE\xFF", 5); // call CControllerConfigManager::AffectPadFromKeyBoard
        plugin::patch::SetRaw(0x53F417, (void*)"\xE8\xB4\x7A\x20\x00", 5); // call CPad__getMouseState
        plugin::patch::SetRaw(0x53F41F, (void*)"\x85\xC0\x0F\x8C", 4);     // xor eax, eax -> test eax, eax
                                                            // jz loc_53F526 -> jl loc_53F526
        *(DWORD*)0xB73424 = 0;
        *(DWORD*)0xB73428 = 0;
        ((void (*)())0x541BD0)();
        ((void (*)())0x541DD0)();

        plugin::patch::SetUChar(0x6194A0, 0xE9);                    // jmp setup                    // jmp setup

        ImGui::GetIO().MouseDrawCursor = false;
    }

    
}
void Menu::setStyle() {
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::GetStyle().FrameRounding = 4.0f;
    ImGui::GetStyle().GrabRounding = 4.0f;
    ImGui::GetStyle().WindowTitleAlign = {0.5f, 0.5f};

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8.00f, 8.00f);
    style.FramePadding = ImVec2(5.00f, 2.00f);
    style.CellPadding = ImVec2(6.00f, 6.00f);
    style.ItemSpacing = ImVec2(6.00f, 6.00f);
    style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
    style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
    style.IndentSpacing = 25;
    style.ScrollbarSize = 15;
    style.GrabMinSize = 10;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 1;
    style.WindowRounding = 7;
    style.ChildRounding = 4;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ScrollbarRounding = 9;
    style.GrabRounding = 3;
    style.LogSliderDeadzone = 4;
    style.TabRounding = 4;
}