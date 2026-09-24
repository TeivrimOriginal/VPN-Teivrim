#pragma once
#include <windows.h>

// Мастер 3 шага One-Click: шаг 1 Install, 2 Port Forward, 3 QR
void ShowWizard(HWND parent);
bool IsWizardNeeded(); // true если wg0.conf отсутствует
