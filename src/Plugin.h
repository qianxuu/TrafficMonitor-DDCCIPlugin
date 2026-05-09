#pragma once

#include "PluginInterface.h"

class Plugin : public ITMPlugin
{
public:
    // ITMPlugin
    int          GetAPIVersion() const override;
    IPluginItem* GetItem(int index) override;
    void         DataRequired() override;
    const wchar_t* GetInfo(PluginInfoIndex index) override;
    int          GetCommandCount() override;
    const wchar_t* GetCommandName(int command_index) override;
    void         OnPluginCommand(int command_index, void* hWnd, void* para) override;
    void         OnInitialize(ITrafficMonitor* pApp) override;

private:
    ITrafficMonitor* m_pApp = nullptr;
};