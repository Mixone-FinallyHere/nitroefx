#pragma once
#include "types.h"

#include <random>
#include <memory>

#include <glm/glm.hpp>

class SPLRandom {
public:
    static u64 nextU64() {
        return getInstance()->dist();
    }

    static u32 nextU32() {
        return getInstance()->dist32();
    }

    static f32 nextF32() {
        return getInstance()->distf();
    }

    static f32 nextF32N() {
        return nextF32() * 2.0f - 1.0f;
    }

    static glm::vec3 unitVector() {
        return glm::normalize(glm::vec3(nextF32N(), nextF32N(), nextF32N()));
    }

    static glm::vec3 unitXY() {
        return glm::normalize(glm::vec3(nextF32N(), nextF32N(), 0.0f));
    }


    // variance must be in the range [0, 1]
    // Generates a random float in the range (n * (variance / 2)) around n
    // So the value range is [n * (1 - variance / 2), n * (1 + variance / 2)]
    static f32 scaledRange(f32 n, f32 variance) {
        variance = glm::clamp(variance, 0.0f, 1.0f);
        const f32 min = n * (1.0f - variance / 2.0f);
        const f32 max = n * (1.0f + variance / 2.0f);
        return min + nextF32() * (max - min);
    }

    // Generates a random float in the range [n, n * (1 + variance)]
    static f32 scaledRange2(f32 n, f32 variance) {
        const f32 min = n;
        const f32 max = n * (1.0f + variance);

        return min + nextF32() * (max - min);
    }

    static f32 range(f32 min, f32 max) {
        return min + nextF32() * (max - min);
    }

    static f32 aroundZero(f32 range) {
        return SPLRandom::range(-range, range);
    }

private:
    SPLRandom() : m_gen(m_rd()), m_distf(0.0f, 1.0f) {}
    SPLRandom(const SPLRandom&) = delete;
    SPLRandom& operator=(const SPLRandom&) = delete;
    SPLRandom(SPLRandom&&) = delete;
    SPLRandom& operator=(SPLRandom&&) = delete;

    static SPLRandom* getInstance() {
        if (!s_instance) {
            s_instance = new SPLRandom();
        }

        return s_instance;
    }

    u64 dist() {
        return m_dist(m_gen);
    }

    u32 dist32() {
        return m_dist32(m_gen);
    }

    f32 distf() {
        return m_distf(m_gen);
    }

private:
    static inline SPLRandom* s_instance;

    std::random_device m_rd;
    std::mt19937_64 m_gen;
    std::uniform_int_distribution<u64> m_dist;
    std::uniform_int_distribution<u32> m_dist32;
    std::uniform_real_distribution<f32> m_distf;
};

