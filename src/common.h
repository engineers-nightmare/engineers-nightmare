#pragma once

#ifndef _WIN32
#define __unused __attribute__(( unused ))
#else
#define __unused
#endif // _WIN32

#define PI 3.14159265f
#define DEG2RAD(x) (x * PI / 180.f)
#define RAD2DEG(x) (x * 180.f / PI)

