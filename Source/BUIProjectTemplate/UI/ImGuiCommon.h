#pragma once

#ifdef IMGUI_API
#define WITH_IMGUI 1
#else
#define WITH_IMGUI 0
#endif // IMGUI_API

#if WITH_IMGUI
#include <imgui.h>
#include "ImGuiDelegates.h"
#include "ImGuiModule.h"
#endif // WITH_IMGUI
