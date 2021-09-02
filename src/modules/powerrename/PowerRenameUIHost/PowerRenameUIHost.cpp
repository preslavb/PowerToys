// PowerRenameUIHost.cpp : Defines the entry point for the application.
//
#include "pch.h"

#include "framework.h"
#include "PowerRenameUIHost.h"

#include <PowerRenameEnum.h>
#include <PowerRenameItem.h>
#include <PowerRenameManager.h>

#include <queue>

#define MAX_LOADSTRING 100

const wchar_t c_WindowClass[] = L"PowerRename";
//extern HINSTANCE g_hInst;

void AppWindow::CreateUIHost(IPowerRenameManager* prManager, IUnknown* dataSource)
{
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    winrt::PowerRenameUI_new::App app;
    const auto result = AppWindow::Show(prManager, dataSource);
    app.Close();
}

int AppWindow::Show(IPowerRenameManager* prManager, IUnknown* dataSource)
{
    auto window = AppWindow(HINSTANCE{}, prManager, dataSource);
    window.CreateAndShowWindow(SW_SHOWNORMAL);
    return window.MessageLoop(window.m_accelerators.get());
}

LRESULT AppWindow::MessageHandler(UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (message)
    {
        HANDLE_MSG(WindowHandle(), WM_CREATE, OnCreate);
        HANDLE_MSG(WindowHandle(), WM_COMMAND, OnCommand);
        HANDLE_MSG(WindowHandle(), WM_DESTROY, OnDestroy);
        HANDLE_MSG(WindowHandle(), WM_SIZE, OnResize);
    default:
        return base_type::MessageHandler(message, wParam, lParam);
    }

    return base_type::MessageHandler(message, wParam, lParam);
}

AppWindow::AppWindow(HINSTANCE hInstance, IPowerRenameManager* prManager, IUnknown* dataSource) noexcept :
    m_instance(hInstance), m_prManager{ prManager }, m_dataSource{ dataSource }
{
}

void AppWindow::CreateAndShowWindow(int nCmdShow)
{
    m_accelerators.reset(LoadAcceleratorsW(m_instance, MAKEINTRESOURCE(IDC_POWERRENAMEUIHOST)));

    WNDCLASSEXW wcex = { sizeof(wcex) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = m_instance;
    wcex.hIcon = LoadIconW(m_instance, MAKEINTRESOURCE(IDC_POWERRENAMEUIHOST));
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_POWERRENAMEUIHOST);
    wcex.lpszClassName = c_WindowClass;
    wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    RegisterClassExW(&wcex); // don't test result, handle error at CreateWindow

    wchar_t title[64];
    LoadStringW(m_instance, IDS_APP_TITLE, title, ARRAYSIZE(title));

    HWND window = CreateWindowW(c_WindowClass, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, m_instance, this);
    THROW_LAST_ERROR_IF(!window);

    ShowWindow(window, nCmdShow);
    UpdateWindow(window);
    SetFocus(window);
}

bool AppWindow::OnCreate(HWND, LPCREATESTRUCT) noexcept
{
    m_mainUserControl = winrt::PowerRenameUI_new::MainWindow();
    m_xamlIsland = CreateDesktopWindowsXamlSource(WS_TABSTOP, m_mainUserControl);

    OnInitDlg();

    return true;
}

void AppWindow::OnCommand(HWND, int id, HWND hwndCtl, UINT codeNotify) noexcept
{
    switch (id)
    {
    case IDM_ABOUT:
        DialogBoxW(m_instance, MAKEINTRESOURCE(IDD_ABOUTBOX), WindowHandle(), [](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) -> INT_PTR {
            switch (message)
            {
            case WM_INITDIALOG:
            {
                return TRUE;
            }
            case WM_COMMAND:
                if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
                {
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                }
                break;
            }
            return FALSE;
        });
        break;

    case IDM_EXIT:
        OnCloseDlg();
        break;
    }
}

void AppWindow::OnDestroy(HWND hwnd) noexcept
{
    //m_buttonClickRevoker.revoke();
    base_type::OnDestroy(hwnd);
}

void AppWindow::OnResize(HWND, UINT state, int cx, int cy) noexcept
{
    const auto newHeight = cy;
    const auto newWidth = cx;
    const auto islandHeight = newHeight - (50 * 2) - 10;
    const auto islandWidth = newWidth - (10 * 2);
    //SetWindowPos(m_button1.get(), 0, ButtonWidth * 2, ButtonMargin, ButtonWidth, ButtonHeight, SWP_SHOWWINDOW);
    //SetWindowPos(m_xamlButton1, m_button1.get(), newWidth - (ButtonWidth * 2), ButtonMargin, ButtonWidth, ButtonHeight, SWP_SHOWWINDOW);
    SetWindowPos(m_xamlIsland, NULL, 0, 60, islandWidth, islandHeight, SWP_SHOWWINDOW);
    //SetWindowPos(m_button2.get(), m_xamlIsland, (ButtonMargin + newWidth - ButtonWidth) / 2, newHeight - ButtonMargin - ButtonHeight, ButtonWidth, ButtonHeight, SWP_SHOWWINDOW);
}

void AppWindow::OnInitDlg()
{
    if (m_dataSource)
    {
        // Populate the manager from the data object
        if (FAILED(EnumerateItems(m_dataSource)))
        {
            // Failed during enumeration.  Close the dialog.
            OnCloseDlg();
            return;
        }
    }
}

void AppWindow::OnCloseDlg()
{
    PostQuitMessage(0);
}

HRESULT AppWindow::EnumerateItems(IUnknown* dataObject)
{
    HRESULT hr = S_OK;
    // Enumerate the data object and populate the manager
    if (m_prManager)
    {
        m_disableCountUpdate = true;

        // Ensure we re-create the enumerator
        m_prEnum = nullptr;
        hr = CPowerRenameEnum::s_CreateInstance(dataObject, m_prManager, IID_PPV_ARGS(&m_prEnum));
        if (SUCCEEDED(hr))
        {
            //TODO: Create progress dialog
            //m_prpui.Start();
            hr = m_prEnum->Start();
            //m_prpui.Stop();
        }

        m_disableCountUpdate = false;

        if (SUCCEEDED(hr))
        {
            UINT itemCount = 0;
            m_prManager->GetItemCount(&itemCount);
            PopulateExplorerItems();
            //_UpdateCounts();
        }
    }

    return hr;
}

void AppWindow::PopulateExplorerItems()
{
    auto explorerItems = m_mainUserControl.ExplorerItems();
    UINT count = 0;
    m_prManager->GetVisibleItemCount(&count);

    UINT currDepth = 0;
    std::queue<UINT> parents{};
    UINT prevId = 0;
    parents.push(0);

    for (UINT i = 0; i < count; ++i)
    {
        CComPtr<IPowerRenameItem> renameItem;
        if (SUCCEEDED(m_prManager->GetVisibleItemByIndex(count, &renameItem)))
        {
            int id = 0;
            renameItem->GetId(&id);

            PWSTR originalName = nullptr;
            renameItem->GetOriginalName(&originalName);

            UINT depth = 0;
            renameItem->GetDepth(&depth);

            bool isFolder = false;
            bool isSubFolderContent = false;
            winrt::check_hresult(renameItem->GetIsFolder(&isFolder));

            if (currDepth == depth)
            {
                m_mainUserControl.AddExplorerItem(id, originalName, isFolder ? 0 : 1, parents.back());
                prevId = id;
            }
            else if (depth > currDepth)
            {
                parents.push(prevId);
                m_mainUserControl.AddExplorerItem(id, originalName, isFolder ? 0 : 1, parents.back());
                currDepth = depth;
                prevId = id;
            }
            else
            {
                while (currDepth > depth)
                {
                    parents.pop();
                }
                m_mainUserControl.AddExplorerItem(id, originalName, isFolder ? 0 : 1, parents.back());
                prevId = id;
            }
        }
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    //g_hostHInst = hInstance;
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    winrt::PowerRenameUI_new::App app;
    CComPtr<IPowerRenameManager> spsrm;
    HRESULT hr = CPowerRenameManager::s_CreateInstance(&spsrm);
    if (SUCCEEDED(hr))
    {
        // Create the factory for our items
        CComPtr<IPowerRenameItemFactory> spsrif;
        hr = CPowerRenameItem::s_CreateInstance(nullptr, IID_PPV_ARGS(&spsrif));
        hr = spsrm->PutRenameItemFactory(spsrif);

        if (SUCCEEDED(hr))
        {
            CComPtr<IUnknown> dataSource{};
            const auto result = AppWindow::Show(spsrm, dataSource);
        }
    }
        app.Close();
}
