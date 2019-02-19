#include "math.h"
#include "fmath.h"

inline float fast_sqrtf(float x)
{
	return sqrtf(x);
}


inline int fast_floorf(float x)
{
    return floorf(x);
}

inline int fast_ceilf(float x)
{
    return ceilf(x);
}

inline int fast_roundf(float x)
{
    return roundf(x);
}

inline float fast_atanf(float x)
{
    return atanf(x);
}

inline float fast_atan2f(float y, float x)
{
    return atan2f(y, x);
}

inline float fast_expf(float x)
{
    return expf(x);
}

inline float fast_cbrtf(float d)
{
    return cbrtf(d);
}

inline float fast_fabsf(float d)
{
    return fabsf(d);
}

inline float fast_log(float x)
{
    return log(x);
}
inline float fast_log2(float x)
{
    return log2(x);
}

inline float fast_powf(float a, float b)
{
    return powf(a, b);
}

