#pragma once

#define FLUFF_NOEXCEPT noexcept

#if __cplusplus > 201703L
/// C++20 features
#define FLUFF_LIKELY [[likely]]
#define FLUFF_UNLIKELY [[unlikely]]
#else
// Not defined before C++20
#define FLUFF_LIKELY
#define FLUFF_UNLIKELY
#endif
