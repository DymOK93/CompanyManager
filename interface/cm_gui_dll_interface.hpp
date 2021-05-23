#pragma once

#ifdef CM_GUI_EXPORT
#define CM_GUI_API __declspec(dllexport)
#else
#define CM_GUI_API __declspec(dllimport)
#endif
