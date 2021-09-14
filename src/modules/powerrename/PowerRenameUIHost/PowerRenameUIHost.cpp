// PowerRenameUIHost.cpp : Defines the entry point for the application.
//
#include "pch.h"

#include "PowerRenameUIHost.h"
#include <settings.h>
#include <trace.h>

#define MAX_LOADSTRING 100

const wchar_t c_WindowClass[] = L"PowerRename";
HINSTANCE g_hostHInst;

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
    m_instance{ hInstance }, m_mngrEvents{ this }
{
    HRESULT hr = CPowerRenameManager::s_CreateInstance(&m_prManager);
    // Create the factory for our items
    CComPtr<IPowerRenameItemFactory> prItemFactory;
    hr = CPowerRenameItem::s_CreateInstance(nullptr, IID_PPV_ARGS(&prItemFactory));
    hr = m_prManager->PutRenameItemFactory(prItemFactory);
    hr = m_prManager->Advise(&m_mngrEvents, &m_cookie);

    if (SUCCEEDED(hr))
    {
        CComPtr<IShellItemArray> shellItemArray;
        LPCWSTR input[] = {
            L"C:\\Users\\stefa\\Projects\\test_dir\\asd",
            L"C:\\Users\\stefa\\Projects\\test_dir\\1.SSS",
            L"C:\\Users\\stefa\\Projects\\test_dir\\2.SSS"
        };

        CreateShellItemArrayFromPaths(3, input, &shellItemArray);
        CComPtr<IEnumShellItems> enumShellItems;
        hr = shellItemArray->EnumItems(&enumShellItems);
        if (SUCCEEDED(hr))
        {
            EnumerateShellItems(enumShellItems);
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

    m_window = CreateWindowW(c_WindowClass, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, m_instance, this);
    THROW_LAST_ERROR_IF(!m_window);

    ShowWindow(m_window, SW_SHOWNORMAL);
    UpdateWindow(m_window);
    SetFocus(m_window);
}

bool AppWindow::OnCreate(HWND, LPCREATESTRUCT) noexcept
{
    m_mainUserControl = winrt::PowerRenameUI_new::MainWindow();
    m_xamlIsland = CreateDesktopWindowsXamlSource(WS_TABSTOP, m_mainUserControl);

    PopulateExplorerItems();
    SetHandlers();

    m_mainUserControl.BtnRename().IsEnabled(false);

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
    std::stack<UINT> parents{};
    UINT prevId = 0;
    parents.push(0);

    for (UINT i = 0; i < count; ++i)
    {
        CComPtr<IPowerRenameItem> renameItem;
        if (SUCCEEDED(m_prManager->GetVisibleItemByIndex(i, &renameItem)))
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

            if (depth > currDepth)
            {
                parents.push(prevId);
                currDepth = depth;
            }
            else
            {
                while (currDepth > depth)
                {
                    parents.pop();
                    currDepth--;
                }
                currDepth = depth;
            }
            m_mainUserControl.AddExplorerItem(id, originalName, isFolder ? 0 : 1, parents.top());
            prevId = id;
        }
    }
}

HRESULT AppWindow::EnumerateShellItems(_In_ IEnumShellItems* enumShellItems)
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

void AppWindow::SearchReplaceChanged()
{
    // Pass updated search and replace terms to the IPowerRenameRegEx handler
    CComPtr<IPowerRenameRegEx> prRegEx;
    if (m_prManager && SUCCEEDED(m_prManager->GetRenameRegEx(&prRegEx)))
    {
        winrt::hstring searchTerm = m_mainUserControl.TextBoxSearch().Text();
        prRegEx->PutSearchTerm(searchTerm.c_str());

        winrt::hstring replaceTerm = m_mainUserControl.TextBoxReplace().Text();
        prRegEx->PutReplaceTerm(replaceTerm.c_str());
    }
}

void AppWindow::ValidateFlags(PowerRenameFlags flag)
{
    if (flag == Uppercase)
    {
        if (m_mainUserControl.TglBtnUpperCase().IsChecked())
        {
            m_mainUserControl.TglBtnLowerCase().IsChecked(false);
            m_mainUserControl.TglBtnTitleCase().IsChecked(false);
            //TODO(stefan): Add Capitalized checkbox
            //Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_CAPITALIZED), FALSE);
        }
    }
    else if (flag == Lowercase)
    {
        if (m_mainUserControl.TglBtnLowerCase().IsChecked())
        {
            m_mainUserControl.TglBtnUpperCase().IsChecked(false);
            m_mainUserControl.TglBtnTitleCase().IsChecked(false);
            //Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_CAPITALIZED), FALSE);
        }
    }
    else if (flag == Titlecase)
    {
        if (m_mainUserControl.TglBtnTitleCase().IsChecked())
        {
            m_mainUserControl.TglBtnUpperCase().IsChecked(false);
            m_mainUserControl.TglBtnLowerCase().IsChecked(false);
            //Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_CAPITALIZED), FALSE);
        }
    }
    //else if (checkBoxId == IDC_TRANSFORM_CAPITALIZED)
    //{
    //    if (Button_GetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_CAPITALIZED)) == BST_CHECKED)
    //    {
    //        Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_UPPERCASE), FALSE);
    //        Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_LOWERCASE), FALSE);
    //        Button_SetCheck(GetDlgItem(m_hwnd, IDC_TRANSFORM_TITLECASE), FALSE);
    //    }
    //}
    else if (flag == NameOnly)
    {
        if (m_mainUserControl.ChckBoxNameOnly().IsChecked())
        {
            m_mainUserControl.ChckBoxExtensionOnly().IsChecked(false);
        }
    }
    else if (flag == ExtensionOnly)
    {
        if (m_mainUserControl.ChckBoxExtensionOnly().IsChecked())
        {
            m_mainUserControl.ChckBoxNameOnly().IsChecked(false);
        }
    }
}

void AppWindow::UpdateFlag(PowerRenameFlags flag, UpdateFlagCommand command)
{
    DWORD flags{};
    m_prManager->GetFlags(&flags);

    if (command == UpdateFlagCommand::Set)
    {
        flags |= flag;        
    }
    else if (command == UpdateFlagCommand::Reset)
    {
        flags &= ~flag;
    }

    // Ensure we update flags
    if (m_prManager)
    {
        m_prManager->PutFlags(flags);
    }
}

void AppWindow::SetHandlers()
{
    // TextBox Search
    m_mainUserControl.TextBoxSearch().TextChanged([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        SearchReplaceChanged();
    });

    // TextBox Replace
    m_mainUserControl.TextBoxReplace().TextChanged([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        SearchReplaceChanged();
    });

    // ToggleButton UpperCase
    m_mainUserControl.TglBtnUpperCase().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(Uppercase);
        UpdateFlag(Uppercase, UpdateFlagCommand::Set);
    });
    m_mainUserControl.TglBtnUpperCase().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(Uppercase, UpdateFlagCommand::Reset);
    });

    // ToggleButton LowerCase
    m_mainUserControl.TglBtnLowerCase().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(Lowercase);
        UpdateFlag(Lowercase, UpdateFlagCommand::Set);
    });
    m_mainUserControl.TglBtnLowerCase().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(Lowercase, UpdateFlagCommand::Reset);
    });

    // ToggleButton TitleCase
    m_mainUserControl.TglBtnTitleCase().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(Titlecase);
        UpdateFlag(Titlecase, UpdateFlagCommand::Set);
    });
    m_mainUserControl.TglBtnTitleCase().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(Titlecase, UpdateFlagCommand::Reset);
    });

    // CheckBox Regex
    m_mainUserControl.ChckBoxRegex().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(UseRegularExpressions);
        UpdateFlag(UseRegularExpressions, UpdateFlagCommand::Set);
    });
    m_mainUserControl.ChckBoxRegex().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(UseRegularExpressions, UpdateFlagCommand::Reset);
    });

    // CheckBox CaseSensitive
    m_mainUserControl.ChckBoxCaseSensitive().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(CaseSensitive);
        UpdateFlag(UseRegularExpressions, UpdateFlagCommand::Set);
    });
    m_mainUserControl.ChckBoxCaseSensitive().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(CaseSensitive, UpdateFlagCommand::Reset);
    });

    // CheckBox NameOnly
    m_mainUserControl.ChckBoxNameOnly().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(NameOnly);
        UpdateFlag(NameOnly, UpdateFlagCommand::Set);
    });
    m_mainUserControl.ChckBoxNameOnly().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(NameOnly, UpdateFlagCommand::Reset);
    });

    // CheckBox ExtensionOnly
    m_mainUserControl.ChckBoxExtensionOnly().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(ExtensionOnly);
        UpdateFlag(ExtensionOnly, UpdateFlagCommand::Set);
    });
    m_mainUserControl.ChckBoxExtensionOnly().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(ExtensionOnly, UpdateFlagCommand::Reset);
    });

    // CheckBox MatchAllOccurences
    m_mainUserControl.ChckBoxMatchAll().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(MatchAllOccurences);
        UpdateFlag(MatchAllOccurences, UpdateFlagCommand::Set);
    });
    m_mainUserControl.ChckBoxMatchAll().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(MatchAllOccurences, UpdateFlagCommand::Reset);
    });

    // CheckBox ExcludeFiles
    m_mainUserControl.ChckBoxExcludeFiles().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(ExcludeFiles);
        UpdateFlag(ExcludeFiles, UpdateFlagCommand::Set);
    });
    m_mainUserControl.ChckBoxExcludeFiles().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(ExcludeFiles, UpdateFlagCommand::Reset);
    });

    // CheckBox ExcludeFolders
    m_mainUserControl.ChckBoxExcludeFolders().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(ExcludeFolders);
        UpdateFlag(ExcludeFolders, UpdateFlagCommand::Set);
    });
    m_mainUserControl.ChckBoxExcludeFolders().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(ExcludeFolders, UpdateFlagCommand::Reset);
    });

    // CheckBox ExcludeSubfolders
    m_mainUserControl.ChckBoxExcludeSubfolders().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(ExcludeSubfolders);
        UpdateFlag(ExcludeSubfolders, UpdateFlagCommand::Set);
    });
    m_mainUserControl.ChckBoxExcludeSubfolders().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(ExcludeSubfolders, UpdateFlagCommand::Reset);
    });

    // CheckBox EnumerateItems
    m_mainUserControl.ChckBoxEnumerateItems().Checked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        ValidateFlags(EnumerateItems);
        UpdateFlag(EnumerateItems, UpdateFlagCommand::Set);
    });
    m_mainUserControl.ChckBoxEnumerateItems().Unchecked([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        UpdateFlag(EnumerateItems, UpdateFlagCommand::Reset);
    });

    // BtnRename
    m_mainUserControl.BtnRename().Click([&](winrt::Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
        Rename();
    });
}

void AppWindow::Rename()
{
    if (m_prManager)
    {
        m_prManager->Rename(m_window);
    }

    // Persist the current settings.  We only do this when
    // a rename is actually performed.  Not when the user
    // closes/cancels the dialog.
    WriteSettings();
}

HRESULT AppWindow::WriteSettings()
{
    // Check if we should store our settings
    if (CSettingsInstance().GetPersistState())
    {
        DWORD flags = 0;
        m_prManager->GetFlags(&flags);
        CSettingsInstance().SetFlags(flags);

        winrt::hstring searchTerm = m_mainUserControl.TextBoxSearch().Text();
        CSettingsInstance().SetSearchText(std::wstring{ searchTerm });

        if (CSettingsInstance().GetMRUEnabled() && m_spSearchACL)
        {
            CComPtr<IPowerRenameMRU> spSearchMRU;
            if (SUCCEEDED(m_spSearchACL->QueryInterface(IID_PPV_ARGS(&spSearchMRU))))
            {
                spSearchMRU->AddMRUString(searchTerm.c_str());
            }
        }

        winrt::hstring replaceTerm = m_mainUserControl.TextBoxReplace().Text();
        CSettingsInstance().SetReplaceText(std::wstring{ replaceTerm });

        if (CSettingsInstance().GetMRUEnabled() && m_spReplaceACL)
        {
            CComPtr<IPowerRenameMRU> spReplaceMRU;
            if (SUCCEEDED(m_spReplaceACL->QueryInterface(IID_PPV_ARGS(&spReplaceMRU))))
            {
                spReplaceMRU->AddMRUString(replaceTerm.c_str());
            }
        }

        Trace::SettingsChanged();
    }

    return S_OK;
}

void AppWindow::UpdateCounts()
{
    // This method is CPU intensive.  We disable it during certain operations
    // for performance reasons.
    if (m_disableCountUpdate)
    {
        return;
    }

    UINT selectedCount = 0;
    UINT renamingCount = 0;
    if (m_prManager)
    {
        m_prManager->GetSelectedItemCount(&selectedCount);
        m_prManager->GetRenameItemCount(&renamingCount);
    }

    if (m_selectedCount != selectedCount ||
        m_renamingCount != renamingCount)
    {
        m_selectedCount = selectedCount;
        m_renamingCount = renamingCount;

        // Update counts UI elements if/when added

        // Update Rename button state
        m_mainUserControl.BtnRename().IsEnabled(renamingCount > 0);
    }
}

HRESULT AppWindow::OnItemAdded(_In_ IPowerRenameItem* renameItem) {
    //// Check if the user canceled the enumeration from the progress dialog UI
    //if (m_prpui.IsCanceled())
    //{
    //    m_prpui.Stop();
    //    if (m_sppre)
    //    {
    //        // Cancel the enumeration
    //        m_sppre->Cancel();
    //    }
    //}
    return S_OK;
}

HRESULT AppWindow::OnUpdate(_In_ IPowerRenameItem* renameItem) {
    int id;
    HRESULT hr = renameItem->GetId(&id);
    if (SUCCEEDED(hr))
    {
        PWSTR newName = nullptr;
        hr = renameItem->GetNewName(&newName);
        if (SUCCEEDED(hr))
        {
            hstring newNamehs = newName == nullptr ? hstring{} : newName;
            m_mainUserControl.UpdateExplorerItem(id, newNamehs);
        }
    }

    UpdateCounts();
    return S_OK;
}

HRESULT AppWindow::OnError(_In_ IPowerRenameItem* renameItem) {
    return S_OK;
}

HRESULT AppWindow::OnRegExStarted(_In_ DWORD threadId) {
    return S_OK;
}

HRESULT AppWindow::OnRegExCanceled(_In_ DWORD threadId) {\
    return S_OK;
}

HRESULT AppWindow::OnRegExCompleted(_In_ DWORD threadId) {
    return S_OK;
}

HRESULT AppWindow::OnRenameStarted() {
    return S_OK;
}

HRESULT AppWindow::OnRenameCompleted() {
    // Close the window
    PostMessage(m_window, WM_CLOSE, (WPARAM)0, (LPARAM)0); 
    return S_OK;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    g_hostHInst = hInstance;
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    winrt::PowerRenameUI_new::App app;
    const auto result = AppWindow::Show(hInstance);
    app.Close();
}
