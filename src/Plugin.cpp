#include "Plugin.h"

int Plugin::GetAPIVersion() const
{
    return 7;
}

IPluginItem* Plugin::GetItem(int index)
{
    (void)index;
    return nullptr;
}

void Plugin::DataRequired()
{
}

const wchar_t* Plugin::GetInfo(PluginInfoIndex index)
{
    switch (index)
    {
    case TMI_NAME:        return L"DDC/CI Monitor Off";
    case TMI_DESCRIPTION: return L"Turn off the monitor via DDC/CI protocol";
    case TMI_AUTHOR:      return L"";
    case TMI_COPYRIGHT:   return L"";
    case TMI_VERSION:     return L"1.0.0";
    case TMI_URL:         return L"";
    default:              return L"";
    }
}

int Plugin::GetCommandCount()
{
    return 1;
}

const wchar_t* Plugin::GetCommandName(int command_index)
{
    if (command_index == 0)
        return L"Turn off monitor";
    return nullptr;
}

void Plugin::OnPluginCommand(int command_index, void* hWnd, void* para)
{
    (void)command_index;
    (void)hWnd;
    (void)para;
    // Will be implemented in Task 4
}

void Plugin::OnInitialize(ITrafficMonitor* pApp)
{
    m_pApp = pApp;
}