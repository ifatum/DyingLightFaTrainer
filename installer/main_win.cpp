#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "ui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static ID3D11Device* device;
static ID3D11DeviceContext* context;
static IDXGISwapChain* swap_chain;
static ID3D11RenderTargetView* target;
static UINT resize_w, resize_h;

static void create_target() {
    ID3D11Texture2D* back = nullptr;
    swap_chain->GetBuffer(0, IID_PPV_ARGS(&back));
    device->CreateRenderTargetView(back, nullptr, &target);
    back->Release();
}

static bool create_device(HWND window) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = window;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    for (D3D_DRIVER_TYPE driver : {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP})
        if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, driver, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd, &swap_chain, &device, nullptr, &context))) {
            create_target();
            return true;
        }
    return false;
}

static LRESULT WINAPI window_proc(HWND window, UINT msg, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(window, msg, w, l)) return true;
    switch (msg) {
        case WM_SIZE:
            if (w != SIZE_MINIMIZED) resize_w = LOWORD(l), resize_h = HIWORD(l);
            return 0;
        case WM_GETMINMAXINFO: {
            float dpi = GetDpiForWindow(window) / 96.0f;
            ((MINMAXINFO*)l)->ptMinTrackSize = {(LONG)(900 * dpi), (LONG)(640 * dpi)};
            return 0;
        }
        case WM_DPICHANGED: {
            RECT* r = (RECT*)l;
            SetWindowPos(window, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        case WM_SYSCOMMAND:
            if ((w & 0xfff0) == SC_KEYMENU) return 0;
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(window, msg, w, l);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    WNDCLASSEXW wc{sizeof wc};
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"FaTrainerInstaller";
    RegisterClassExW(&wc);
    POINT origin{0, 0};
    HMONITOR monitor = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{sizeof info};
    GetMonitorInfoW(monitor, &info);
    UINT dpi = GetDpiForSystem();
    float scale = dpi / 96.0f;
    int width = (int)(1120 * scale), height = (int)(740 * scale);
    RECT area = info.rcWork;
    HWND window = CreateWindowExW(0, wc.lpszClassName, L"FaTrainer Installer (" FATRAINER_EDITION ")", WS_OVERLAPPEDWINDOW, area.left + (area.right - area.left - width) / 2,
                                  area.top + (area.bottom - area.top - height) / 2, width, height, nullptr, nullptr, instance, nullptr);
    BOOL dark = TRUE;
    const DWORD IMMERSIVE_DARK_MODE = 20;
    DwmSetWindowAttribute(window, IMMERSIVE_DARK_MODE, &dark, sizeof dark);
    if (!create_device(window)) {
        MessageBoxW(window, L"FaTrainer Installer could not start Direct3D 11 on this PC.", L"FaTrainer Installer", MB_ICONERROR);
        return 1;
    }
    platform::window = window;
    ShowWindow(window, show);
    UpdateWindow(window);
    ImGui::CreateContext();
    app::init(GetDpiForWindow(window) / 96.0f);
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, context);
    for (bool running = true; running;) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;
        if (IsIconic(window)) {
            Sleep(50);
            continue;
        }
        if (resize_w && resize_h) {
            target->Release();
            swap_chain->ResizeBuffers(0, resize_w, resize_h, DXGI_FORMAT_UNKNOWN, 0);
            resize_w = resize_h = 0;
            create_target();
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app::frame();
        ImGui::Render();
        const float ground[4] = {17 / 255.0f, 17 / 255.0f, 19 / 255.0f, 1};
        context->OMSetRenderTargets(1, &target, nullptr);
        context->ClearRenderTargetView(target, ground);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swap_chain->Present(1, 0);
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    target->Release();
    swap_chain->Release();
    context->Release();
    device->Release();
    DestroyWindow(window);
    return 0;
}
