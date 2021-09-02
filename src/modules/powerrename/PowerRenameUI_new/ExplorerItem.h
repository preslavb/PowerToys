#pragma once
#include "ExplorerItem.g.h"

namespace winrt::PowerRenameUI_new::implementation
{
    struct ExplorerItem : ExplorerItemT<ExplorerItem>
    {
        enum class ExplorerItemType
        {
            Folder = 0,
            File = 1
        };
        
        ExplorerItem() = delete;

        ExplorerItem(int32_t id, hstring const& original, int32_t type);
        int32_t Id();
        hstring Original();
        void Original(hstring const& value);
        hstring Renamed();
        void Renamed(hstring const& value);
        int32_t Type();
        void Type(int32_t value);
        Windows::Foundation::Collections::IObservableVector<PowerRenameUI_new::ExplorerItem> Children();
        void Children(Windows::Foundation::Collections::IObservableVector<PowerRenameUI_new::ExplorerItem> const& value);
        winrt::event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token) noexcept;
    
    private:
        int32_t m_id;
        winrt::hstring m_original;
        winrt::hstring m_renamed;
        winrt::Windows::Foundation::Collections::IObservableVector<PowerRenameUI_new::ExplorerItem> m_children;
        int32_t m_type;
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::PowerRenameUI_new::factory_implementation
{
    struct ExplorerItem : ExplorerItemT<ExplorerItem, implementation::ExplorerItem>
    {
    };
}
