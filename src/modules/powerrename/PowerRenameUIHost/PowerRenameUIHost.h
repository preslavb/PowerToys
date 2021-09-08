#pragma once

#include "resource.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.system.h>
#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include <winrt/windows.ui.xaml.hosting.h>
#pragma pop_macro("GetCurrentTime")
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#include <winrt/windows.ui.xaml.controls.h>
#include <winrt/Windows.ui.xaml.media.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/PowerRenameUI_new.h>

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::Xaml::Controls;
