#include <cmath>
#include "export.hpp"

#if defined(__GNUC__)
extern "C" OPENTRACK_COMPAT_EXPORT bool __attribute__ ((noinline)) nanp(double value)
#elif defined(_WIN32)
extern "C" OPENTRACK_COMPAT_EXPORT __declspec(noinline) bool nanp(double value)
#else
extern "C" OPENTRACK_COMPAT_EXPORT bool nanp(double value)
#endif
{
    using std::isnan;
    using std::isinf;

    const volatile double x = value;
    return isnan(x) || isinf(x);
}
