#pragma once

#include "Prerequisites.h"

/**
 * @struct Particle
 * @brief Estado CPU de una particula individual.
 */
struct Particle {
    EU::Vector3 position = EU::Vector3(0.0f, 0.0f, 0.0f);
    EU::Vector3 velocity = EU::Vector3(0.0f, 0.0f, 0.0f);

    float age = 0.0f;
    float lifetime = 1.0f;
    float startSize = 0.2f;
    float endSize = 0.2f;
    float rotation = 0.0f;
    float angularVelocity = 0.0f;
    bool active = false;

    float normalizedAge() const {
        if (lifetime <= 0.000001f) {
            return 1.0f;
        }
        const float value = age / lifetime;
        return (std::max)(0.0f, (std::min)(1.0f, value));
    }

    float currentSize() const {
        const float t = normalizedAge();
        return startSize + (endSize - startSize) * t;
    }
};
