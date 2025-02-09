#pragma once
#include <random>

namespace random::detail {

inline std::random_device s_rd;
inline std::mt19937_64 s_gen(s_rd());
inline std::uniform_int_distribution<u64> s_dist;
inline std::uniform_int_distribution<u32> s_dist32;
inline std::uniform_real_distribution<f32> s_distf(0.0f, 1.0f);

}

namespace random {

inline u64 nextU64() {
    return detail::s_dist(detail::s_gen);
}

inline u32 nextU32() {
    return detail::s_dist32(detail::s_gen);
}

inline f32 nextF32() {
    return detail::s_distf(detail::s_gen);
}

inline f32 nextF32N() {
    return nextF32() * 2.0f - 1.0f;
}

inline glm::vec3 unitVector() {
    return glm::normalize(glm::vec3(nextF32N(), nextF32N(), nextF32N()));
}

inline glm::vec3 unitXY() {
    return glm::normalize(glm::vec3(nextF32N(), nextF32N(), 0.0f));
}


// Generates a random float in the range [n * ((1 + variance) / 2), n]
inline f32 scaledRange(f32 n, f32 variance) {
    const f32 min = n * ((1.0f + variance) / 2.0f);
    const f32 max = n;
    return min + nextF32() * (max - min);
}

// Generates a random float in the range [n, n * (1 + variance)]
inline f32 scaledRange2(f32 n, f32 variance) {
    const f32 min = n;
    const f32 max = n * (1.0f + variance);

    return min + nextF32() * (max - min);
}

inline f32 range(f32 min, f32 max) {
    return min + nextF32() * (max - min);
}

inline f32 aroundZero(f32 range) {
    return ::random::range(-range, range);
}

}

