// Windows editor shell: a child window that blits the software-rendered DrumForge GUI (no GDI+/D2D dependency).
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>

#include <algorithm>
#include <cmath>

#include "../gui/gui.h"
#include "base/source/fobject.h"
#include "controller.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "public.sdk/source/common/pluginview.h"

extern HINSTANCE ghInst;   // set by the SDK's dllmain.cpp

using namespace Steinberg;

namespace df {

namespace {
const wchar_t* kClassName = L"DrumForgeEditorView";
const UINT_PTR kTimerId = 1;

float systemScale()
{
    HDC dc = GetDC(nullptr);
    int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
    if (dc) ReleaseDC(nullptr, dc);
    return std::max(1.0f, std::min(3.0f, std::floor(dpi / 96.0f * 4.0f + 0.5f) / 4.0f));
}
}  // namespace

class WinEditor : public CPluginView, public IPlugViewContentScaleSupport, public gui::IHost {
public:
    explicit WinEditor(Controller* c) : CPluginView(nullptr), ctrl(c), ed(this)
    {
        applyScale(systemScale());
    }
    ~WinEditor() override { destroyWindow(); }

    // ---- IPlugViewContentScaleSupport
    tresult PLUGIN_API setContentScaleFactor(ScaleFactor factor) SMTG_OVERRIDE
    {
        float s = std::max(1.0f, std::min(3.0f, std::floor(factor * 4.0f + 0.5f) / 4.0f));
        if (std::fabs(s - ed.scale()) < 0.01f) return kResultTrue;
        applyScale(s);
        if (hwnd) {
            SetWindowPos(hwnd, nullptr, 0, 0, ed.pixelWidth(), ed.pixelHeight(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        if (plugFrame) { ViewRect r = rect; plugFrame->resizeView(this, &r); }
        return kResultTrue;
    }

    // ---- gui::IHost -> controller
    float get(int i) override { return ctrl->plain(i); }
    void begin(int i) override { ctrl->beginEditIdx(i); }
    void set(int i, float v) override { ctrl->performEditPlain(i, v); }
    void end(int i) override { ctrl->endEditIdx(i); }
    int program() override { return ctrl->program(); }
    void selectProgram(int i) override { ctrl->selectProgram(i); }
    float meter(int p) override { return ctrl->meter(p); }
    void trigger(int note, int vel) override { ctrl->trigger(note, vel); }
    UserBank& userBank() override { return ctrl->userBank(); }
    void userSlotChanged(int slot) override { ctrl->userSlotChanged(slot); }
    float groovePhase() override { return ctrl->groovePhase(); }
    bool saveKitText(const std::string& text) override { return kitDialog(true, const_cast<std::string&>(text)); }
    bool loadKitText(std::string& text) override { return kitDialog(false, text); }

    DEFINE_INTERFACES
        DEF_INTERFACE(IPlugViewContentScaleSupport)
    END_DEFINE_INTERFACES(CPluginView)
    REFCOUNT_METHODS(CPluginView)

    void attachedToParent() override
    {
        registerClass();
        hwnd = CreateWindowExW(0, kClassName, L"DrumForge", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0,
                               ed.pixelWidth(), ed.pixelHeight(), (HWND)systemWindow, nullptr, ghInst, this);
        if (hwnd) {
            ed.tick(); ed.render();
            SetTimer(hwnd, kTimerId, 33, nullptr);
        }
    }
    void removedFromParent() override { destroyWindow(); }
    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) SMTG_OVERRIDE
    {
        return (type && std::strcmp(type, kPlatformTypeHWND) == 0) ? kResultTrue : kResultFalse;
    }
    tresult PLUGIN_API onSize(ViewRect* newSize) SMTG_OVERRIDE
    {
        if (newSize) rect = *newSize;
        return kResultTrue;
    }
    tresult PLUGIN_API canResize() SMTG_OVERRIDE { return kResultFalse; }

private:
    Controller* ctrl;
    gui::Editor ed;
    HWND hwnd = nullptr;

    // Save / Open dialog for kit files (*.dfkit), starting in Documents\\DrumForge Kits
    bool kitDialog(bool save, std::string& text)
    {
        wchar_t file[MAX_PATH] = L"My Kit.dfkit";
        if (!save) file[0] = 0;
        wchar_t dir[MAX_PATH] = L"";
        PWSTR docs = nullptr;
        if (SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs) == S_OK && docs) {
            std::wstring d = std::wstring(docs) + L"\\DrumForge Kits";
            CoTaskMemFree(docs);
            CreateDirectoryW(d.c_str(), nullptr);
            wcsncpy(dir, d.c_str(), MAX_PATH - 1);
        }
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = L"DrumForge kits (*.dfkit)\0*.dfkit\0All files\0*.*\0";
        ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH;
        ofn.lpstrInitialDir = dir[0] ? dir : nullptr;
        ofn.lpstrDefExt = L"dfkit";
        ofn.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
        if (!(save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn))) return false;
        HANDLE h = CreateFileW(file, save ? GENERIC_WRITE : GENERIC_READ, FILE_SHARE_READ, nullptr, save ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) return false;
        bool ok = false;
        if (save) {
            DWORD n = 0;
            ok = WriteFile(h, text.data(), (DWORD)text.size(), &n, nullptr) && n == text.size();
        } else {
            DWORD sz = GetFileSize(h, nullptr), n = 0;
            if (sz != INVALID_FILE_SIZE && sz < (1u << 20)) {
                text.resize(sz);
                ok = ReadFile(h, &text[0], sz, &n, nullptr) && n == sz;
            }
        }
        CloseHandle(h);
        return ok;
    }

    void applyScale(float s)
    {
        ed.setScale(s);
        ViewRect r(0, 0, ed.pixelWidth(), ed.pixelHeight());
        setRect(r);
    }
    void destroyWindow()
    {
        if (hwnd) { KillTimer(hwnd, kTimerId); SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0); DestroyWindow(hwnd); hwnd = nullptr; }
    }
    static void registerClass()
    {
        static bool done = false;
        if (done) return;
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_DBLCLKS | CS_OWNDC;
        wc.lpfnWndProc = &WinEditor::wndProc;
        wc.hInstance = ghInst;
        wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        done = true;
    }
    void refresh()
    {
        if (ed.tick()) { ed.render(); if (hwnd) InvalidateRect(hwnd, nullptr, FALSE); }
    }
    void paint()
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = ed.pixelWidth();
        bmi.bmiHeader.biHeight = -ed.pixelHeight();   // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        SetDIBitsToDevice(dc, 0, 0, ed.pixelWidth(), ed.pixelHeight(), 0, 0, 0, ed.pixelHeight(), ed.surface().pixels(), &bmi, DIB_RGB_COLORS);
        EndPaint(hwnd, &ps);
    }

    static LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
    {
        if (msg == WM_NCCREATE) {
            auto* cs = (CREATESTRUCTW*)lp;
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return DefWindowProcW(h, msg, wp, lp);
        }
        auto* self = (WinEditor*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (!self) return DefWindowProcW(h, msg, wp, lp);
        const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        switch (msg) {
            case WM_ERASEBKGND: return 1;
            case WM_PAINT: self->paint(); return 0;
            case WM_TIMER: self->refresh(); return 0;
            case WM_LBUTTONDOWN: SetCapture(h); SetFocus(h); self->ed.mouseDown((float)GET_X_LPARAM(lp), (float)GET_Y_LPARAM(lp), 0, shift, false); self->refresh(); return 0;
            case WM_LBUTTONDBLCLK: SetCapture(h); self->ed.mouseDown((float)GET_X_LPARAM(lp), (float)GET_Y_LPARAM(lp), 0, shift, true); self->refresh(); return 0;
            case WM_RBUTTONDOWN: SetFocus(h); self->ed.mouseDown((float)GET_X_LPARAM(lp), (float)GET_Y_LPARAM(lp), 1, shift, false); self->refresh(); return 0;
            case WM_LBUTTONUP: if (GetCapture() == h) ReleaseCapture(); self->ed.mouseUp((float)GET_X_LPARAM(lp), (float)GET_Y_LPARAM(lp)); self->refresh(); return 0;
            case WM_MOUSEMOVE: {
                TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, h, 0};
                TrackMouseEvent(&tme);
                self->ed.mouseMove((float)GET_X_LPARAM(lp), (float)GET_Y_LPARAM(lp), shift);
                self->refresh();
                return 0; }
            case WM_MOUSELEAVE: self->ed.mouseLeave(); self->refresh(); return 0;
            case WM_MOUSEWHEEL: {
                POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
                ScreenToClient(h, &pt);
                self->ed.wheel((float)pt.x, (float)pt.y, (float)GET_WHEEL_DELTA_WPARAM(wp) / 120.0f, shift);
                self->refresh();
                return 0; }
            case WM_KEYDOWN: self->ed.keyDown((int)wp); self->refresh(); return 0;
            case WM_GETDLGCODE: return DLGC_WANTARROWS | DLGC_WANTALLKEYS;
        }
        return DefWindowProcW(h, msg, wp, lp);
    }
};

IPlugView* createEditor(Controller* c) { return new WinEditor(c); }

}  // namespace df
