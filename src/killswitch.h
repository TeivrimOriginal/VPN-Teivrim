#pragma once
#include <windows.h>
#include <string>

bool IsKillSwitchEnabled();
bool EnableKillSwitch();
bool DisableKillSwitch();
// Called at GUI startup to restore persistent KS if marker exists
bool RestoreKillSwitchIfNeeded();
