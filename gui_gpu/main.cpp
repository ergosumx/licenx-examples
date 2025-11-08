#include <windows.h>
#include <shellapi.h>

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "licenx/licenx.h"

#include "feature_catalog.h"

namespace
{
    constexpr int kControlStatus = 100;
    constexpr int kButtonRender = 101;
    constexpr int kButtonTraining = 102;

    struct AppState
    {
        licenx::Status initStatus{licenx::Status::InternalError};
        licenx::Diagnostics diagnostics{};
        std::optional<licenx::ProductState> productState;
    };

    std::wstring Utf16FromUtf8(const std::string& value)
    {
        if (value.empty())
        {
            return {};
        }

        const int length = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
        if (length <= 0)
        {
            return {};
        }

        std::wstring result(static_cast<std::size_t>(length), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), length);
        return result;
    }

    std::string Utf8FromWide(std::wstring_view value)
    {
        if (value.empty())
        {
            return {};
        }

        const int required = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (required <= 0)
        {
            return {};
        }

        std::string result(static_cast<std::size_t>(required), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), required, nullptr, nullptr);
        return result;
    }

    std::wstring DescribeStatus(const AppState& state)
    {
        if (state.initStatus == licenx::Status::Ok)
        {
            return L"License validated successfully.";
        }

        std::wstring message = L"Restricted mode: ";
        message += Utf16FromUtf8(state.diagnostics.message);
        if (!state.diagnostics.licensePath.empty())
        {
            message += L" (";
            message += Utf16FromUtf8(state.diagnostics.licensePath);
            message += L")";
        }
        return message;
    }

    std::wstring DefaultLicensePath()
    {
        wchar_t buffer[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        std::filesystem::path exePath{buffer};
        std::filesystem::path path = exePath.parent_path() / L".." / L"licenses" / L"gpu-pro.licx";
        return path.lexically_normal().wstring();
    }

    std::optional<std::wstring> FindCommandLineArgument(int argc, wchar_t** argv, std::wstring_view flag)
    {
        for (int i = 1; i < argc - 1; ++i)
        {
            if (flag == argv[i])
            {
                return std::wstring{argv[i + 1]};
            }
        }
        return std::nullopt;
    }

    void SetDevOverride()
    {
        _putenv_s("ERGOX_LICENX_DEV_OVERRIDE", "licenx.dev.override.confirmed");
    }

    void HandleFeature(HWND hwnd, AppState& state, std::size_t featureIndex, std::wstring_view displayName)
    {
        const bool licensed = (state.initStatus == licenx::Status::Ok) && state.productState.has_value();
        licenx_examples::FeatureState featureState = licenx_examples::FeatureState::Disabled;
        if (licensed)
        {
            featureState = licenx_examples::ResolveFeature(*state.productState, featureIndex);
        }

        std::wstring message;
        UINT icon = MB_ICONINFORMATION;

        if (!licensed)
        {
            icon = MB_ICONWARNING;
            message = L"Feature unavailable. Reason: ";
            message += Utf16FromUtf8(state.diagnostics.message);
            message += L"\nApplication continues in restricted mode.";
        }
        else if (featureState == licenx_examples::FeatureState::Disabled)
        {
            icon = MB_ICONWARNING;
            message = L"Feature disabled by license. Falling back to offline preview.";
        }
        else if (featureState == licenx_examples::FeatureState::Extended)
        {
            message = L"Extended mode active – unlocking premium rendering path.";
        }
        else
        {
            message = L"Standard mode active.";
        }

        MessageBoxW(hwnd, message.c_str(), std::wstring(displayName).c_str(), MB_OK | icon);
    }

    void InitialiseControls(HWND hwnd, AppState& state)
    {
        const std::wstring status = DescribeStatus(state);
        CreateWindowExW(0, L"STATIC", status.c_str(), WS_CHILD | WS_VISIBLE, 20, 20, 360, 40, hwnd,
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlStatus)), GetModuleHandleW(nullptr), nullptr);

        CreateWindowExW(0, L"BUTTON", L"Render Preview", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                        20, 80, 160, 40, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kButtonRender)), GetModuleHandleW(nullptr), nullptr);

        CreateWindowExW(0, L"BUTTON", L"Model Training", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        200, 80, 160, 40, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kButtonTraining)), GetModuleHandleW(nullptr), nullptr);
    }

    LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        switch (message)
        {
        case WM_CREATE:
        {
            auto* createStruct = reinterpret_cast<LPCREATESTRUCTW>(lParam);
            state = reinterpret_cast<AppState*>(createStruct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
            InitialiseControls(hwnd, *state);
            return 0;
        }
        case WM_COMMAND:
        {
            if (!state)
            {
                break;
            }

            switch (LOWORD(wParam))
            {
            case kButtonRender:
                HandleFeature(hwnd, *state, licenx_examples::GpuFeatureCatalog::kRenderPreviewIndex, L"Render Preview");
                return 0;
            case kButtonTraining:
                HandleFeature(hwnd, *state, licenx_examples::GpuFeatureCatalog::kModelTrainingIndex, L"Model Training");
                return 0;
            default:
                break;
            }
            break;
        }
        case WM_DESTROY:
            licenx::Shutdown();
            PostQuitMessage(0);
            return 0;
        default:
            break;
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    int argc = 0;
    PWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    const std::wstring licensePath = [&]() {
        if (argv)
        {
            if (auto explicitPath = FindCommandLineArgument(argc, argv, L"--license"))
            {
                return explicitPath.value();
            }
        }
        return DefaultLicensePath();
    }();

    if (argv)
    {
        LocalFree(argv);
    }

    SetDevOverride();

    licenx::Config config;
    config.licensePath = Utf8FromWide(licensePath);

    AppState state;
    state.initStatus = licenx::Initialize(config);
    state.diagnostics = licenx::GetDiagnostics();
    state.productState = licenx_examples::GetProductStateSafe();

    const wchar_t className[] = L"LicenxExampleWindow";

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = MainWindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    RegisterClassW(&windowClass);

    HWND hwnd = CreateWindowExW(0, className, L"LicenX GPU Feature Demo", WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, 420, 200, nullptr, nullptr, hInstance, &state);

    if (!hwnd)
    {
        MessageBoxW(nullptr, L"Failed to create main window", L"LicenX GPU Demo", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
