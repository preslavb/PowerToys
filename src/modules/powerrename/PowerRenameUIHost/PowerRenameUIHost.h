#pragma once
#include "pch.h"

#include "resource.h"
#include "XamlBridge.h"

#include <PowerRenameEnum.h>
#include <PowerRenameItem.h>
#include <PowerRenameManager.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.system.h>
#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include <winrt/windows.ui.xaml.hosting.h>
#pragma pop_macro("GetCurrentTime")
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/windows.ui.xaml.controls.h>
#include <winrt/windows.ui.xaml.controls.primitives.h>
#include <winrt/Windows.ui.xaml.media.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/PowerRenameUI_new.h>

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Xaml::Controls;


class AppWindow : public DesktopWindowT<AppWindow>
{
public:
    static int Show(HINSTANCE hInstance);
    LRESULT MessageHandler(UINT message, WPARAM wParam, LPARAM lParam) noexcept;

private:
    AppWindow(HINSTANCE hInstance) noexcept;
    void CreateAndShowWindow();
    bool OnCreate(HWND, LPCREATESTRUCT) noexcept;
    void OnCommand(HWND, int id, HWND hwndCtl, UINT codeNotify) noexcept;
    void OnDestroy(HWND hwnd) noexcept;
    void OnResize(HWND, UINT state, int cx, int cy) noexcept;
    HRESULT CreateShellItemArrayFromPaths(UINT ct, LPCWSTR rgt[], IShellItemArray** ppsia);
    void PopulateExplorerItems();
    HRESULT EnumerateItems(_In_ IEnumShellItems* enumShellItems);
    void ValidateFlags();
    void UpdateFlags();

    wil::unique_haccel m_accelerators;
    const HINSTANCE m_instance;
    HWND m_xamlIsland{};
    winrt::PowerRenameUI_new::MainWindow m_mainUserControl{ nullptr };

    bool m_disableCountUpdate = false;
    CComPtr<IPowerRenameManager> m_prManager;
    CComPtr<IUnknown> m_dataSource;
    CComPtr<IPowerRenameEnum> m_prEnum;
};
