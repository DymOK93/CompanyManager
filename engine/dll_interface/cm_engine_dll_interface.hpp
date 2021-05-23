#pragma once

#ifdef CM_ENGINE_EXPORT
#define CM_ENGINE_API __declspec(dllexport)
#else
#define CM_ENGINE_API __declspec(dllimport)
#endif
