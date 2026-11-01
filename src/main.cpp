// Copyright (c) 2026 Skitzelles - part of Game Vault, https://github.com/skitzellez/xbox-game-vault
// PolyForm Noncommercial License 1.0.0: not for sale, and this notice stays in every copy.
//
// Game Vault · Flash: plays a Flash game (.swf) on the Xbox with Ruffle (ruffle.rs, MIT / Apache-2.0), the Flash
// Player emulator, in the console's WebView2. Game Vault starts it with gamevault-flash:?file=<path of the .swf>
// [&base=<the game's folder>]. Every file the page asks for comes through WebResourceRequested at
// https://flash.gamevault/ : the page and Ruffle from the package's "web" folder, /game.swf from the game's path (USB
// drive or Game Vault's folders), and every other file the game loads from its folder (base=, else the .swf's). The
// controller is read here and sent to the page each frame (web/page.js turns it into keys and a mouse).
// View + Menu held goes back to Game Vault.

#include <windows.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/Microsoft.Web.WebView2.Core.h>
#include <cstdio>
#include <delayimp.h>
#include <string>

using namespace winrt;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Gaming::Input;
using namespace Windows::Storage;
using namespace Windows::UI::Core;
using namespace Microsoft::Web::WebView2::Core;

namespace {

const wchar_t kHost[] = L"https://flash.gamevault";

std::wstring g_game;     // the .swf
std::wstring g_base;     // the game's folder: its other files (more .swf, sounds, data) come from here (0.1.0.9)
std::wstring g_logPath;  // LocalState\flash.log

void Log(const std::wstring& line)
{
    static bool fresh = true;
    if (g_logPath.empty()) {
        g_logPath = std::wstring(ApplicationData::Current().LocalFolder().Path()) + L"\\flash.log";
    }
    FILE* f = _wfopen(g_logPath.c_str(), fresh ? L"wb" : L"ab");
    fresh = false;
    if (!f) return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    std::string utf8 = to_string(line);
    fprintf(f, "%02d:%02d:%02d.%03d %s\r\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, utf8.c_str());
    fclose(f);
}

std::wstring Mime(const std::wstring& path)
{
    auto ends = [&](const wchar_t* e) {
        size_t n = wcslen(e);
        return path.size() >= n && _wcsicmp(path.c_str() + path.size() - n, e) == 0;
    };
    if (ends(L".html")) return L"text/html; charset=utf-8";
    if (ends(L".js")) return L"text/javascript; charset=utf-8";
    if (ends(L".wasm")) return L"application/wasm";
    if (ends(L".swf")) return L"application/x-shockwave-flash";
    if (ends(L".svg")) return L"image/svg+xml";
    if (ends(L".png")) return L"image/png";
    if (ends(L".css")) return L"text/css";
    return L"application/octet-stream";
}

std::wstring QueryValue(const Uri& uri, const wchar_t* name)
{
    try {
        return std::wstring(uri.QueryParsed().GetFirstValueByName(name));
    } catch (...) {
        return L"";
    }
}

struct App : implements<App, IFrameworkViewSource, IFrameworkView> {
    CoreWindow m_window{ nullptr };
    CoreWebView2Environment m_env{ nullptr };
    CoreWebView2Controller m_ctl{ nullptr };
    // the page is drawn into this visual of our window (composition hosting): the windowed kind makes a child
    // window, which the Xbox never shows (black screen, 0.1.0.4)
    Windows::UI::Composition::Compositor m_comp{ nullptr };
    Windows::UI::Composition::CompositionTarget m_target{ nullptr };
    Windows::UI::Composition::ContainerVisual m_root{ nullptr };
    CoreWebView2 m_web{ nullptr };
    bool m_started = false, m_ready = false, m_quitting = false;
    ULONGLONG m_comboSince = 0;
    std::wstring m_lastPad;

    IFrameworkView CreateView() { return *this; }

    void Initialize(CoreApplicationView const& view)
    {
        view.Activated({ this, &App::OnActivated });
    }

    void Load(hstring const&) {}
    void Uninitialize() {}

    void SetWindow(CoreWindow const& window)
    {
        m_window = window;
        Windows::UI::ViewManagement::ApplicationViewScaling::TrySetDisableLayoutScaling(true);
        Windows::UI::ViewManagement::ApplicationView::GetForCurrentView().SetDesiredBoundsMode(
            Windows::UI::ViewManagement::ApplicationViewBoundsMode::UseCoreWindow);
        window.SizeChanged([this](auto&&, auto&&) { Resize(); });
        // B is a game button here, not "back"
        Windows::UI::Core::SystemNavigationManager::GetForCurrentView().BackRequested(
            [](auto&&, BackRequestedEventArgs const& a) { a.Handled(true); });
    }

    void OnActivated(CoreApplicationView const&, IActivatedEventArgs const& args)
    {
        if (args.Kind() == ActivationKind::Protocol) {
            Uri uri = args.as<ProtocolActivatedEventArgs>().Uri();
            g_game = QueryValue(uri, L"file");
            g_base = QueryValue(uri, L"base");
            Log(L"started by " + std::wstring(uri.AbsoluteUri()));
        } else {
            Log(L"started without a game");
        }
        if (g_game.empty()) {
            // no game given: the test file in the package
            g_game = std::wstring(Package::Current().InstalledLocation().Path()) + L"\\web\\test.swf";
        }
        // (without base=, the .swf's own folder: a game made of several files finds the others next to it)
        if (g_base.empty()) g_base = g_game.substr(0, g_game.find_last_of(L'\\'));
        while (!g_base.empty() && (g_base.back() == L'\\' || g_base.back() == L'/')) g_base.pop_back();
        Log(L"game: " + g_game + L" (its files from " + g_base + L")");
        m_window.Activate();
        if (m_started) {
            if (m_web) m_web.Reload();   // another game while running
            return;
        }
        m_started = true;
        StartWebView();
    }

    void Resize()
    {
        if (!m_ctl) return;
        Rect b = m_window.Bounds();
        m_ctl.Bounds(Rect{ 0, 0, b.Width, b.Height });
    }

    fire_and_forget StartWebView()
    {
        auto lifetime = get_strong();
        try {
            CoreWebView2EnvironmentOptions opts;
            // Flash games start their music at once: no "click to play" from the browser
            opts.AdditionalBrowserArguments(L"--autoplay-policy=no-user-gesture-required");
            m_env = co_await CoreWebView2Environment::CreateWithOptionsAsync(L"", L"", opts);
            Log(L"WebView2 " + std::wstring(m_env.BrowserVersionString()));
            auto ref = CoreWebView2ControllerWindowReference::CreateFromCoreWindow(m_window);
            m_comp = Windows::UI::Composition::Compositor();
            m_target = m_comp.CreateTargetForCurrentView();
            m_root = m_comp.CreateContainerVisual();
            m_root.RelativeSizeAdjustment({ 1.0f, 1.0f });
            m_target.Root(m_root);
            auto cc = co_await m_env.CreateCoreWebView2CompositionControllerAsync(ref);
            cc.RootVisualTarget(m_root);
            m_ctl = cc;
            Log(L"controller made (composition)");
            Resize();
            m_ctl.IsVisible(true);
            m_web = m_ctl.CoreWebView2();
            auto settings = m_web.Settings();
            settings.AreDefaultContextMenusEnabled(false);
            settings.IsStatusBarEnabled(false);
            settings.IsZoomControlEnabled(false);
            settings.AreBrowserAcceleratorKeysEnabled(false);
            m_web.AddWebResourceRequestedFilter(std::wstring(kHost) + L"/*", CoreWebView2WebResourceContext::All);
            m_web.WebResourceRequested([this](auto&&, CoreWebView2WebResourceRequestedEventArgs const& a) { Serve(a); });
            m_web.WebMessageReceived([](auto&&, CoreWebView2WebMessageReceivedEventArgs const& a) {
                try {
                    Log(L"page: " + std::wstring(a.TryGetWebMessageAsString()));
                } catch (...) {
                }
            });
            m_web.NavigationCompleted([this](auto&&, CoreWebView2NavigationCompletedEventArgs const& a) {
                m_ready = a.IsSuccess();
                Log(std::wstring(L"page loaded: ") + (a.IsSuccess() ? L"ok" : L"failed ") +
                    std::to_wstring((int)a.WebErrorStatus()));
                m_ctl.MoveFocus(CoreWebView2MoveFocusReason::Programmatic);
            });
            m_web.ProcessFailed([](auto&&, CoreWebView2ProcessFailedEventArgs const& a) {
                Log(L"browser process failed: " + std::to_wstring((int)a.ProcessFailedKind()));
            });
            m_web.NewWindowRequested([](auto&&, CoreWebView2NewWindowRequestedEventArgs const& a) { a.Handled(true); });
            Log(L"opening the page");
            m_web.Navigate(std::wstring(kHost) + L"/index.html");
        } catch (hresult_error const& e) {
            Log(L"WebView2 could not start: " + std::wstring(e.message()));
        }
    }

    fire_and_forget Serve(CoreWebView2WebResourceRequestedEventArgs args)
    {
        auto lifetime = get_strong();
        apartment_context ui;
        auto deferral = args.GetDeferral();
        std::wstring uri = std::wstring(args.Request().Uri());
        std::wstring path = uri.substr(wcslen(kHost));
        size_t q = path.find_first_of(L"?#");
        if (q != std::wstring::npos) path.resize(q);
        std::wstring file;
        if (path == L"/game.swf") {
            file = g_game;
        } else if (path.find(L"..") == std::wstring::npos) {
            // the page and Ruffle from the package; anything else the game asks for ("newfades/184.swf",
            // "sounds/hit.mp3", "data.xml") from the game's folder, as Flash read it next to the projector (0.1.0.9:
            // before, those came from the package too and multi-file games could not load their parts)
            std::wstring decoded = std::wstring(Uri::UnescapeComponent(path));
            std::wstring rel;
            for (wchar_t c : decoded) rel += c == L'/' ? L'\\' : c;
            bool ours = path == L"/" || path == L"/index.html" || path == L"/page.js" || path.rfind(L"/ruffle/", 0) == 0;
            if (ours || g_base.empty()) {
                file = std::wstring(Package::Current().InstalledLocation().Path()) + L"\\web" + rel;
            } else {
                file = g_base + rel;
            }
        }
        Windows::Storage::Streams::IRandomAccessStream stream{ nullptr };
        try {
            if (!file.empty()) {
                StorageFile f = co_await StorageFile::GetFileFromPathAsync(file);
                stream = co_await f.OpenReadAsync();
            }
        } catch (hresult_error const& e) {
            Log(L"cannot open " + file + L": " + std::wstring(e.message()));
        }
        co_await ui;
        if (stream) {
            args.Response(m_env.CreateWebResourceResponse(stream, 200, L"OK",
                                                          L"Content-Type: " + Mime(file) +
                                                              L"\r\nAccess-Control-Allow-Origin: *"));
        } else {
            Log(L"not found: " + path);
            args.Response(m_env.CreateWebResourceResponse(nullptr, 404, L"Not Found", L""));
        }
        deferral.Complete();
    }

    // the first controller, sent to the page as {"b":buttons,"lx","ly","rx","ry","lt","rt"}
    void Pad()
    {
        auto pads = Gamepad::Gamepads();
        if (pads.Size() == 0) return;
        GamepadReading r = pads.GetAt(0).GetCurrentReading();
        uint32_t b = (uint32_t)r.Buttons;
        const uint32_t combo = (uint32_t)(GamepadButtons::View | GamepadButtons::Menu);
        if ((b & combo) == combo) {
            if (!m_comboSince) m_comboSince = GetTickCount64();
            if (!m_quitting && GetTickCount64() - m_comboSince > 600) {
                m_quitting = true;
                Log(L"View + Menu: back to Game Vault");
                Windows::System::Launcher::LaunchUriAsync(Uri(L"gamevault:?from=flash"));
                CoreApplication::Exit();
            }
            return;
        }
        m_comboSince = 0;
        if (!m_ready) return;
        // D-pad / left stick = arrow keys and the buttons = keys, sent as real input (DevTools Input domain):
        // keyboard events made in the page are marked untrusted, and Ruffle did not react to them (0.1.0.7)
        const float t = 0.5f;
        uint32_t bits = b | (r.LeftThumbstickY > t ? 64u : 0u) | (r.LeftThumbstickY < -t ? 128u : 0u) |
                        (r.LeftThumbstickX < -t ? 256u : 0u) | (r.LeftThumbstickX > t ? 512u : 0u);
        for (const auto& k : kKeys) {
            bool on = (bits & k.bit) != 0, was = (m_down & k.bit) != 0;
            if (on != was) SendKey(k, on);
        }
        m_down = bits;
        // right stick = a mouse pointer, RT (or LT) = click
        const float dead = 0.15f;
        float mx = fabs(r.RightThumbstickX) > dead ? (float)r.RightThumbstickX : 0.f;
        float my = fabs(r.RightThumbstickY) > dead ? (float)r.RightThumbstickY : 0.f;
        Rect w = m_window.Bounds();
        if (m_cx < 0) m_cx = w.Width / 2, m_cy = w.Height / 2;
        bool moved = false;
        if (mx || my) {
            float speed = 15.f * (std::max)(fabs(mx), fabs(my));   // pixels a frame at full tilt (~900 a second)
            float len = sqrtf(mx * mx + my * my);
            m_cx = (std::min)(w.Width - 1, (std::max)(0.f, m_cx + mx / len * speed));
            m_cy = (std::min)(w.Height - 1, (std::max)(0.f, m_cy - my / len * speed));
            SendMouse(L"mouseMoved");
            moved = true;
        }
        bool click = r.RightTrigger > 0.5 || r.LeftTrigger > 0.5;
        if (click != m_click) {
            m_click = click;
            SendMouse(click ? L"mousePressed" : L"mouseReleased");
            moved = true;
        }
        if (moved) {
            wchar_t msg[96];
            swprintf(msg, 96, L"{\"cx\":%.0f,\"cy\":%.0f}", m_cx, m_cy);   // the page draws the pointer
            try {
                m_web.PostWebMessageAsJson(msg);
            } catch (...) {
            }
        }
    }

    struct Key { uint32_t bit; const wchar_t* key; const wchar_t* code; int vk; const wchar_t* text; };
    static constexpr Key kKeys[] = {
        {64, L"ArrowUp", L"ArrowUp", 38, nullptr},     {128, L"ArrowDown", L"ArrowDown", 40, nullptr},
        {256, L"ArrowLeft", L"ArrowLeft", 37, nullptr}, {512, L"ArrowRight", L"ArrowRight", 39, nullptr},
        {4, L" ", L"Space", 32, L" "},                  {8, L"x", L"KeyX", 88, L"x"},
        {16, L"z", L"KeyZ", 90, L"z"},                  {32, L"c", L"KeyC", 67, L"c"},
        {1, L"Enter", L"Enter", 13, L"\\r"},           {1024, L"Shift", L"ShiftLeft", 16, nullptr},
        {2048, L"Control", L"ControlLeft", 17, nullptr},
    };
    uint32_t m_down = 0;
    float m_cx = -1, m_cy = -1;
    bool m_click = false, m_firstKey = true;

    void SendKey(const Key& k, bool down)
    {
        wchar_t j[256];
        swprintf(j, 256, L"{\"type\":\"%s\",\"key\":\"%s\",\"code\":\"%s\",\"windowsVirtualKeyCode\":%d,"
                         L"\"nativeVirtualKeyCode\":%d%s%s%s}",
                 down ? (k.text ? L"keyDown" : L"rawKeyDown") : L"keyUp", k.key, k.code, k.vk, k.vk,
                 down && k.text ? L",\"text\":\"" : L"", down && k.text ? k.text : L"", down && k.text ? L"\"" : L"");
        if (m_firstKey) {
            m_firstKey = false;
            Log(L"controller: first key " + std::wstring(k.code));
        }
        try {
            m_web.CallDevToolsProtocolMethodAsync(L"Input.dispatchKeyEvent", j);
        } catch (...) {
        }
    }

    void SendMouse(const wchar_t* type)
    {
        bool button = wcscmp(type, L"mouseMoved") != 0;
        wchar_t j[256];
        swprintf(j, 256, L"{\"type\":\"%s\",\"x\":%.0f,\"y\":%.0f,\"button\":\"%s\",\"buttons\":%d,\"clickCount\":%d}",
                 type, m_cx, m_cy, button || m_click ? L"left" : L"none", m_click ? 1 : 0, button ? 1 : 0);
        try {
            m_web.CallDevToolsProtocolMethodAsync(L"Input.dispatchMouseEvent", j);
        } catch (...) {
        }
    }

    void Run()
    {
        m_window.Activate();
        CoreDispatcher d = m_window.Dispatcher();
        while (!m_quitting) {
            d.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
            Pad();
            Sleep(16);
        }
    }
};

} // namespace

// WebView2 (EmbeddedBrowserWebView.dll) loads some Windows functions on first use that the Xbox does not have:
// USER32!ChangeWindowMessageFilterEx closed the app on the console (dump 2026-10-27). A missing one raises
// 0xC06D007F with a DelayLoadInfo; we answer with a stand-in that reports success, as the loader allows.
BOOL WINAPI DoneStandIn() { return TRUE; }
// in the console's kernel (api-ms-win-core-errorhandling), only left out of the app headers
extern "C" __declspec(dllimport) PVOID WINAPI AddVectoredExceptionHandler(ULONG, PVECTORED_EXCEPTION_HANDLER);

LONG WINAPI DelayLoadStandIn(EXCEPTION_POINTERS* e)
{
    DWORD code = e->ExceptionRecord->ExceptionCode;
    if ((code != VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND) &&
         code != VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)) ||
        e->ExceptionRecord->NumberParameters < 1)
        return EXCEPTION_CONTINUE_SEARCH;
    auto* dli = reinterpret_cast<DelayLoadInfo*>(e->ExceptionRecord->ExceptionInformation[0]);
    char what[160];
    snprintf(what, sizeof(what), "%s!%s", dli->szDll ? dli->szDll : "?",
             dli->dlp.fImportByName ? dli->dlp.szProcName : "(ordinal)");
    Log(L"not on the Xbox, stand-in used: " + std::wstring(what, what + strlen(what)));
    dli->pfnCur = reinterpret_cast<FARPROC>(DoneStandIn);
    return EXCEPTION_CONTINUE_EXECUTION;
}

// a crash that no catch saw: its code and address into the log, so a report says where
LONG WINAPI LastChance(EXCEPTION_POINTERS* e)
{
    wchar_t msg[128];
    swprintf(msg, 128, L"CRASH %08X at %p", (unsigned)e->ExceptionRecord->ExceptionCode, e->ExceptionRecord->ExceptionAddress);
    Log(msg);
    return EXCEPTION_CONTINUE_SEARCH;
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    SetUnhandledExceptionFilter(LastChance);
    AddVectoredExceptionHandler(1, DelayLoadStandIn);
    // CoreApplication::Run makes the window's thread itself: this one must stay in the multithreaded apartment
    // (single-threaded here failed every start on the console, before anything was logged)
    init_apartment();
    Log(L"Game Vault · Flash starting");
    try {
        CoreApplication::Run(make<App>());
    } catch (hresult_error const& e) {
        Log(L"could not start: " + std::wstring(e.message()));
    }
    return 0;
}
