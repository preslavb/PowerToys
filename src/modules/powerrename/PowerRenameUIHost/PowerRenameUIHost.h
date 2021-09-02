#pragma once

#include "resource.h"
#include "XamlBridge.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.system.h>
#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include<winrt/windows.ui.xaml.hosting.h>
#pragma pop_macro("GetCurrentTime")
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/windows.ui.xaml.controls.h>
#include <winrt/Windows.ui.xaml.media.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/PowerRenameUI_new.h>

#include <PowerRenameInterfaces.h>

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Xaml::Controls;

class AppWindow : public DesktopWindowT<AppWindow>
{
public:
    static void CreateUIHost(IPowerRenameManager* prManager, IUnknown* dataSource);
    static int Show(IPowerRenameManager* prManager, IUnknown* dataSource);
    LRESULT MessageHandler(UINT message, WPARAM wParam, LPARAM lParam) noexcept;

private:
    AppWindow(HINSTANCE hInstance, IPowerRenameManager* prManager, IUnknown* dataSource) noexcept;
    void CreateAndShowWindow(int nCmdShow);
    bool OnCreate(HWND, LPCREATESTRUCT) noexcept;
    void OnCommand(HWND, int id, HWND hwndCtl, UINT codeNotify) noexcept;
    void OnDestroy(HWND hwnd) noexcept;
    void OnResize(HWND, UINT state, int cx, int cy) noexcept;

    void OnInitDlg();
    void OnCloseDlg();
    HRESULT EnumerateItems(IUnknown* dataObject);
    void PopulateExplorerItems();

    wil::unique_haccel m_accelerators;
    const HINSTANCE m_instance;
    HWND m_xamlIsland{};
    winrt::PowerRenameUI_new::MainWindow m_mainUserControl{ nullptr };
    
    bool m_disableCountUpdate = false;
    CComPtr<IPowerRenameManager> m_prManager;
    CComPtr<IUnknown> m_dataSource;
    CComPtr<IPowerRenameEnum> m_prEnum;
};
