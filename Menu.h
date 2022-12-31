#pragma once

#include <cinttypes>
#include <d3d9.h>

class Menu {
public:
    static void initMenu(HMODULE hDLL, LPDIRECT3DDEVICE9 pDevice);
    static void uninitMenu();

    static void render();

private:
    static uint32_t           s_nCurrentPage;
    static LPDIRECT3DTEXTURE9 s_pCursorTexture;
    static HMODULE            s_hDLL;

    static void setStyle();

    static void drawCursor();
    static void show_cursor(bool show);

    static WNDPROC s_pWindowProc;
    static bool    s_bMenuState;
    static LRESULT __stdcall WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
