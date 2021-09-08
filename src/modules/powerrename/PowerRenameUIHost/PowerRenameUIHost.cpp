// PowerRenameUIHost.cpp : Defines the entry point for the application.
//
#include "pch.h"

#include "PowerRenameUIHost.h"

#define MAX_LOADSTRING 100

const wchar_t c_WindowClass[] = L"PowerRename";

int AppWindow::Show(HINSTANCE hInstance)
{
    auto window = AppWindow(hInstance);
    window.CreateAndShowWindow();
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

AppWindow::AppWindow(HINSTANCE hInstance) noexcept :
    m_instance(hInstance)
{
    HRESULT hr = CPowerRenameManager::s_CreateInstance(&m_prManager);
    // Create the factory for our items
    CComPtr<IPowerRenameItemFactory> prItemFactory;
    hr = CPowerRenameItem::s_CreateInstance(nullptr, IID_PPV_ARGS(&prItemFactory));
    hr = m_prManager->PutRenameItemFactory(prItemFactory);

    if (SUCCEEDED(hr))
    {
        CComPtr<IShellItemArray> shellItemArray;
        LPCWSTR input[] = {
            L"C:\\Users\\stefa\\Projects\\PowerToys\\installer\\PowerToysSetup\\x64\\Release",
            L"C:\\Users\\stefa\\Projects\\PowerToys\\installer\\PowerToysSetup\\x64\\Release\\PowerToysSetup-0.0.1-x64.msi",
            L"C:\\Users\\stefa\\Projects\\PowerToys\\installer\\PowerToysSetup\\x64\\Release\\PowerToysSetup-0.0.1-x64.wixpdb"
        };

        CreateShellItemArrayFromPaths(3, input, &shellItemArray);
        CComPtr<IEnumShellItems> enumShellItems;
        hr = shellItemArray->EnumItems(&enumShellItems);
        if (SUCCEEDED(hr))
        {
            EnumerateItems(enumShellItems);
        }
    }
}

void AppWindow::CreateAndShowWindow()
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

    ShowWindow(window, SW_SHOWNORMAL);
    UpdateWindow(window);
    SetFocus(window);
}

bool AppWindow::OnCreate(HWND, LPCREATESTRUCT) noexcept
{
    m_mainUserControl = winrt::PowerRenameUI_new::MainWindow();
    m_xamlIsland = CreateDesktopWindowsXamlSource(WS_TABSTOP, m_mainUserControl);

    m_mainUserControl.ChckBoxRegex().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlags();
    });

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
                return TRUE;

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
        PostQuitMessage(0);
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
    SetWindowPos(m_xamlIsland, NULL, 0, 60, islandWidth, islandHeight, SWP_SHOWWINDOW);
}

HRESULT AppWindow::CreateShellItemArrayFromPaths(
    UINT ct,
    LPCWSTR rgt[],
    IShellItemArray** ppsia)
{
    *ppsia = nullptr;
    PIDLIST_ABSOLUTE* rgpidl = new (std::nothrow) PIDLIST_ABSOLUTE[ct];
    HRESULT hr = rgpidl ? S_OK : E_OUTOFMEMORY;
    UINT cpidl;
    for (cpidl = 0; SUCCEEDED(hr) && cpidl < ct; cpidl++)
    {
        hr = SHParseDisplayName(rgt[cpidl], nullptr, &rgpidl[cpidl], 0, nullptr);
    }
    if (SUCCEEDED(hr))
    {
        hr = SHCreateShellItemArrayFromIDLists(cpidl, const_cast<LPCITEMIDLIST*>(rgpidl), ppsia);
    }
    for (UINT i = 0; i < cpidl; i++)
    {
        CoTaskMemFree(rgpidl[i]);
    }
    delete[] rgpidl;
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

HRESULT AppWindow::EnumerateItems(_In_ IEnumShellItems* enumShellItems)
{
    HRESULT hr = S_OK;
    // Enumerate the data object and populate the manager
    if (m_prManager)
    {
        m_disableCountUpdate = true;

        // Ensure we re-create the enumerator
        m_prEnum = nullptr;
        hr = CPowerRenameEnum::s_CreateInstance(nullptr, m_prManager, IID_PPV_ARGS(&m_prEnum));
        if (SUCCEEDED(hr))
        {
            // TODO(stefan): Add progress dialog
            //m_prpui.Start();
            hr = m_prEnum->Start(enumShellItems);
            //m_prpui.Stop();
        }

        m_disableCountUpdate = false;
    }

    return hr;
}

void AppWindow::ValidateFlags()
{
}

void AppWindow::UpdateFlags()
{
    DWORD flags{};
    // Ensure we update flags
    if (m_prManager)
    {
        m_prManager->PutFlags(flags);
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    winrt::PowerRenameUI_new::App app;
    const auto result = AppWindow::Show(hInstance);
    app.Close();
}
