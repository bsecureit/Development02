// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "AppHost.h"
#include "../types/inc/Viewport.hpp"
#include "../types/inc/utils.hpp"
#include "../types/inc/User32Utils.hpp"

#include "resource.h"

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::Foundation::Numerics;
using namespace ::Microsoft::Console;
using namespace ::Microsoft::Console::Types;

AppHost::AppHost() noexcept :
    _app{},
    _logic{ nullptr }, // don't make one, we're going to take a ref on app's
    _window{ nullptr }
{
    _logic = _app.Logic(); // get a ref to app's logic

    _useNonClientArea = _logic.GetShowTabsInTitlebar();

    // If there were commandline args to our process, try and process them here.
    // Do this before AppLogic::Create, otherwise this will have no effect
    _HandleCommandlineArgs();

    if (_useNonClientArea)
    {
        _window = std::make_unique<NonClientIslandWindow>(_logic.GetRequestedTheme());
    }
    else
    {
        _window = std::make_unique<IslandWindow>();
    }

    // Tell the window to callback to us when it's about to handle a WM_CREATE
    auto pfn = std::bind(&AppHost::_HandleCreateWindow,
                         this,
                         std::placeholders::_1,
                         std::placeholders::_2,
                         std::placeholders::_3);
    _window->SetCreateCallback(pfn);

    _window->SetSnapDimensionCallback(std::bind(&winrt::TerminalApp::AppLogic::CalcSnappedDimension,
                                                _logic,
                                                std::placeholders::_1,
                                                std::placeholders::_2));

    _window->MakeWindow();
}

AppHost::~AppHost()
{
    // destruction order is important for proper teardown here
    _window = nullptr;
    _app.Close();
    _app = nullptr;
}

// Method Description:
// - Retrieve the normalized command line arguments, and pass them to
//   the app logic for processing.
// - If the logic determined there's an error while processing that commandline,
//   display a message box to the user with the text of the error, and exit.
//    * We display a message box because we're a Win32 application (not a
//      console app), and the shell has undoubtedly returned to the foreground
//      of the console. Text emitted here might mix unexpectedly with output
//      from the shell process.
// Arguments:
// - <none>
// Return Value:
// - <none>
void AppHost::_HandleCommandlineArgs()
{
    const auto args{ _NormalizedArgs() };
    if (!args.empty())
    {
        const auto result = _logic.SetStartupCommandline({ args });
        const auto message = _logic.EarlyExitMessage();
        if (!message.empty())
        {
            const auto displayHelp = result == 0;
            const auto messageTitle = displayHelp ? IDS_HELP_DIALOG_TITLE : IDS_ERROR_DIALOG_TITLE;
            const auto messageIcon = displayHelp ? MB_ICONWARNING : MB_ICONERROR;
            // TODO:GH#4134: polish this dialog more, to make the text more
            // like msiexec /?
            MessageBoxW(nullptr,
                        message.data(),
                        GetStringResource(messageTitle).data(),
                        MB_OK | messageIcon);

            ExitProcess(result);
        }
    }
}

// Method Description:
// - Retrieve the command line arguments passed,
//   prepend the full name of the application in case it was missing (GH#4170),
//   and return the arguments as vector of hstrings.
// Arguments:
// - <none>
// Return Value:
// - Program arguments as vector of hstrings. If the function fails it returns an empty vector.
std::vector<winrt::hstring> AppHost::_NormalizedArgs() const noexcept
{
    try
    {
        // get the full name of the terminal app
        auto hProc{ GetCurrentProcess() };
        DWORD appPathLen{};
        DWORD capacity{ 256UL }; // begin with a reasonable string size to avoid any additional iteration of the do-while loop
        std::wstring appPath{};
        do
        {
            if (capacity > (std::numeric_limits<DWORD>::max() >> 1))
            {
                return {};
            }

            capacity <<= 1;
            appPath.resize(static_cast<size_t>(capacity));
            appPathLen = GetModuleFileNameExW(hProc, nullptr, appPath.data(), capacity);
        } while (appPathLen >= capacity - 1UL);

        appPath.resize(static_cast<size_t>(appPathLen));

        // get the program arguments
        std::vector<std::wstring> wstrArgs{};
        if (!_GetArgs(wstrArgs))
        {
            return {};
        }

        // check if the first argument is the own call of the terminal app
        std::vector<winrt::hstring> hstrArgs{};
        hstrArgs.reserve(wstrArgs.size() + 1);
        if (wstrArgs.empty())
        {
            hstrArgs.emplace_back(appPath);
        }
        else
        {
            // If the terminal app is in the current directory or in the
            // PATH environment then it might be called with its base name only.
            // So, the base name is the only part of the path we can compare.
            // But it's even worse. The base name could be `WindowsTerminal` if
            // called from within the IDE, it could be `wt` if the alias was called,
            // or `wtd` for the alias of a developer's build. Thus, we only know
            // that the base name has to have a length of at least two characters,
            // and it has to begin with 'w' or 'W'.
            const auto arg0Name = _GetBaseName(wstrArgs.front());
            if (arg0Name.length() < 2U || std::towlower(arg0Name.front()) != L'w')
            {
                hstrArgs.emplace_back(appPath);
            }
        }

        std::for_each(wstrArgs.cbegin(), wstrArgs.cend(), [&hstrArgs](const auto& elem) { hstrArgs.emplace_back(elem); });
        return hstrArgs;
    }
    catch (...)
    {
        return {};
    }
}

// Method Description:
// - Retrieve the command line.
// - Use our own algorithm to tokenize it because `CommandLineToArgvW` treats \"
//   as an escape sequence to preserve the quotation mark (GH#4571)
// - Populate the arguments as vector of wstrings.
// Arguments:
// - args: Reference to a vector of wstrings which receives the arguments.
// Return Value:
// - `true` if the function succeeds, `false` if retrieving the coomand line failed.
// Mothods used may throw.
bool AppHost::_GetArgs(std::vector<std::wstring>& args) const
{
    // get the command line
    const wchar_t* const cmdLnPtr{ GetCommandLineW() };
    if (!cmdLnPtr)
    {
        return false;
    }

    std::wstring cmdLn{ cmdLnPtr };
    auto cmdLnEnd{ cmdLn.end() }; // end of still valid content in the command line
    auto iter{ cmdLn.begin() }; // iterator pointing to the current position in the command line
    auto argBegin{ iter }; // iterator pointing to the begin of an argument
    auto quoted{ false }; // indicates whether a substring is quoted
    auto within{ false }; // indicates whether the current character is inside of an argument

    args.reserve(gsl::narrow_cast<size_t>(64U)); // pre-allocate some space for the string handles
    while (iter < cmdLnEnd)
    {
        auto increment{ true }; // used to avoid iterator incrementation if a quotation mark has been removed
        switch (*iter)
        {
        case L' ': // space and tab are the usual separators for arguments
        case L'\t':
            if (!quoted && within)
            {
                within = false;
                args.emplace_back(argBegin, iter);
            }
            break;

        case L'"': // quotation marks need special handling
            std::move(iter + 1, cmdLnEnd, iter); // move the rest of the command line to the left to overwrite the quote
            --cmdLnEnd; // the string isn't shorter but its valid content is, we can't just resize without potentially invalidate iterators
            quoted = !quoted;
            increment = false; // indicate that iter shall not be incremented because it already points to a new character
        default: // any other character (including quotation mark)
            if (!within)
            {
                within = true;
                argBegin = iter;
            }
            break;
        }

        if (increment)
        {
            ++iter;
        }
    }

    if (within)
    {
        args.emplace_back(argBegin, iter);
    }

    return true;
}

// Method Description:
// - Take a file path and return the file name without file extension.
// Arguments:
// - path: A wstring_view containing the file path.
// Return Value:
// - A wstring_view containing the base name of the file.
std::wstring_view AppHost::_GetBaseName(std::wstring_view path) const noexcept
{
    auto lastPoint = path.find_last_of(L'.');
    const auto lastBackslash = path.find_last_of(L'\\');
    auto lastSlash = path.find_last_of(L'/');
    lastSlash = (lastBackslash != std::wstring::npos && lastSlash != std::wstring::npos) ? std::max(lastBackslash, lastSlash) : std::min(lastBackslash, lastSlash);
    if ((lastSlash != std::wstring::npos && lastPoint < lastSlash) || lastPoint == std::wstring::npos)
    {
        lastPoint = path.size();
    }

    lastSlash = lastSlash == std::wstring::npos ? static_cast<size_t>(0U) : lastSlash + 1U;
    return { &path.at(lastSlash), lastPoint - lastSlash };
}

// Method Description:
// - Initializes the XAML island, creates the terminal app, and sets the
//   island's content to that of the terminal app's content. Also registers some
//   callbacks with TermApp.
// !!! IMPORTANT!!!
// This must be called *AFTER* WindowsXamlManager::InitializeForCurrentThread.
// If it isn't, then we won't be able to create the XAML island.
// Arguments:
// - <none>
// Return Value:
// - <none>
void AppHost::Initialize()
{
    _window->Initialize();

    if (_useNonClientArea)
    {
        // Register our callbar for when the app's non-client content changes.
        // This has to be done _before_ App::Create, as the app might set the
        // content in Create.
        _logic.SetTitleBarContent({ this, &AppHost::_UpdateTitleBarContent });
    }

    // Register the 'X' button of the window for a warning experience of multiple
    // tabs opened, this is consistent with Alt+F4 closing
    _window->WindowCloseButtonClicked([this]() { _logic.WindowCloseButtonClicked(); });

    // Add an event handler to plumb clicks in the titlebar area down to the
    // application layer.
    _window->DragRegionClicked([this]() { _logic.TitlebarClicked(); });

    _logic.RequestedThemeChanged({ this, &AppHost::_UpdateTheme });
    _logic.ToggleFullscreen({ this, &AppHost::_ToggleFullscreen });

    _logic.Create();

    _logic.TitleChanged({ this, &AppHost::AppTitleChanged });
    _logic.LastTabClosed({ this, &AppHost::LastTabClosed });

    _window->UpdateTitle(_logic.Title());

    // Set up the content of the application. If the app has a custom titlebar,
    // set that content as well.
    _window->SetContent(_logic.GetRoot());
    _window->OnAppInitialized();
}

// Method Description:
// - Called when the app's title changes. Fires off a window message so we can
//   update the window's title on the main thread.
// Arguments:
// - sender: unused
// - newTitle: the string to use as the new window title
// Return Value:
// - <none>
void AppHost::AppTitleChanged(const winrt::Windows::Foundation::IInspectable& /*sender*/, winrt::hstring newTitle)
{
    _window->UpdateTitle(newTitle);
}

// Method Description:
// - Called when no tab is remaining to close the window.
// Arguments:
// - sender: unused
// - LastTabClosedEventArgs: unused
// Return Value:
// - <none>
void AppHost::LastTabClosed(const winrt::Windows::Foundation::IInspectable& /*sender*/, const winrt::TerminalApp::LastTabClosedEventArgs& /*args*/)
{
    _window->Close();
}

// Method Description:
// - Resize the window we're about to create to the appropriate dimensions, as
//   specified in the settings. This will be called during the handling of
//   WM_CREATE. We'll load the settings for the app, then get the proposed size
//   of the terminal from the app. Using that proposed size, we'll resize the
//   window we're creating, so that it'll match the values in the settings.
// Arguments:
// - hwnd: The HWND of the window we're about to create.
// - proposedRect: The location and size of the window that we're about to
//   create. We'll use this rect to determine which monitor the window is about
//   to appear on.
// - launchMode: A LaunchMode enum reference that indicates the launch mode
// Return Value:
// - None
void AppHost::_HandleCreateWindow(const HWND hwnd, RECT proposedRect, winrt::TerminalApp::LaunchMode& launchMode)
{
    launchMode = _logic.GetLaunchMode();

    // Acquire the actual intial position
    winrt::Windows::Foundation::Point initialPosition = _logic.GetLaunchInitialPositions(proposedRect.left, proposedRect.top);
    proposedRect.left = gsl::narrow_cast<long>(initialPosition.X);
    proposedRect.top = gsl::narrow_cast<long>(initialPosition.Y);

    long adjustedHeight = 0;
    long adjustedWidth = 0;
    if (launchMode == winrt::TerminalApp::LaunchMode::DefaultMode)
    {
        // Find nearest montitor.
        HMONITOR hmon = MonitorFromRect(&proposedRect, MONITOR_DEFAULTTONEAREST);

        // Get nearest monitor information
        MONITORINFO monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(hmon, &monitorInfo);

        // This API guarantees that dpix and dpiy will be equal, but neither is an
        // optional parameter so give two UINTs.
        UINT dpix = USER_DEFAULT_SCREEN_DPI;
        UINT dpiy = USER_DEFAULT_SCREEN_DPI;
        // If this fails, we'll use the default of 96.
        GetDpiForMonitor(hmon, MDT_EFFECTIVE_DPI, &dpix, &dpiy);

        // We need to check if the top left point of the titlebar of the window is within any screen
        RECT offScreenTestRect;
        offScreenTestRect.left = proposedRect.left;
        offScreenTestRect.top = proposedRect.top;
        offScreenTestRect.right = offScreenTestRect.left + 1;
        offScreenTestRect.bottom = offScreenTestRect.top + 1;

        bool isTitlebarIntersectWithMonitors = false;
        EnumDisplayMonitors(
            nullptr, &offScreenTestRect, [](HMONITOR, HDC, LPRECT, LPARAM lParam) -> BOOL {
                auto intersectWithMonitor = reinterpret_cast<bool*>(lParam);
                *intersectWithMonitor = true;
                // Continue the enumeration
                return FALSE;
            },
            reinterpret_cast<LPARAM>(&isTitlebarIntersectWithMonitors));

        if (!isTitlebarIntersectWithMonitors)
        {
            // If the title bar is out-of-screen, we set the initial position to
            // the top left corner of the nearest monitor
            proposedRect.left = monitorInfo.rcWork.left;
            proposedRect.top = monitorInfo.rcWork.top;
        }

        auto initialSize = _logic.GetLaunchDimensions(dpix);

        const short islandWidth = Utils::ClampToShortMax(
            static_cast<long>(ceil(initialSize.X)), 1);
        const short islandHeight = Utils::ClampToShortMax(
            static_cast<long>(ceil(initialSize.Y)), 1);

        // Get the size of a window we'd need to host that client rect. This will
        // add the titlebar space.
        const auto nonClientSize = _window->GetTotalNonClientExclusiveSize(dpix);
        adjustedWidth = islandWidth + nonClientSize.cx;
        adjustedHeight = islandHeight + nonClientSize.cy;
    }

    const COORD origin{ gsl::narrow<short>(proposedRect.left),
                        gsl::narrow<short>(proposedRect.top) };
    const COORD dimensions{ Utils::ClampToShortMax(adjustedWidth, 1),
                            Utils::ClampToShortMax(adjustedHeight, 1) };

    const auto newPos = Viewport::FromDimensions(origin, dimensions);
    bool succeeded = SetWindowPos(hwnd,
                                  nullptr,
                                  newPos.Left(),
                                  newPos.Top(),
                                  newPos.Width(),
                                  newPos.Height(),
                                  SWP_NOACTIVATE | SWP_NOZORDER);

    // Refresh the dpi of HWND becuase the dpi where the window will launch may be different
    // at this time
    _window->RefreshCurrentDPI();

    // If we can't resize the window, that's really okay. We can just go on with
    // the originally proposed window size.
    LOG_LAST_ERROR_IF(!succeeded);

    TraceLoggingWrite(
        g_hWindowsTerminalProvider,
        "WindowCreated",
        TraceLoggingDescription("Event emitted upon creating the application window"),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
        TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance));
}

// Method Description:
// - Called when the app wants to set its titlebar content. We'll take the
//   UIElement and set the Content property of our Titlebar that element.
// Arguments:
// - sender: unused
// - arg: the UIElement to use as the new Titlebar content.
// Return Value:
// - <none>
void AppHost::_UpdateTitleBarContent(const winrt::Windows::Foundation::IInspectable&, const winrt::Windows::UI::Xaml::UIElement& arg)
{
    if (_useNonClientArea)
    {
        (static_cast<NonClientIslandWindow*>(_window.get()))->SetTitlebarContent(arg);
    }
}

// Method Description:
// - Called when the app wants to change its theme. We'll forward this to the
//   IslandWindow, so it can update the root UI element of the entire XAML tree.
// Arguments:
// - sender: unused
// - arg: the ElementTheme to use as the new theme for the UI
// Return Value:
// - <none>
void AppHost::_UpdateTheme(const winrt::Windows::Foundation::IInspectable&, const winrt::Windows::UI::Xaml::ElementTheme& arg)
{
    _window->OnApplicationThemeChanged(arg);
}

void AppHost::_ToggleFullscreen(const winrt::Windows::Foundation::IInspectable&,
                                const winrt::TerminalApp::ToggleFullscreenEventArgs&)
{
    _window->ToggleFullscreen();
}
