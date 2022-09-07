#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <mmsystem.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;
#include <comdef.h>

#include <tchar.h>
#include <strsafe.h>

#include "AviUtl/aviutl_exedit_sdk/aviutl.hpp"
#include "AviUtl/aviutl_exedit_sdk/exedit.hpp"
#include "Common/Tracer.h"
#include "Common/Dialog.h"
