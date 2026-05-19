
#include "noise.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include <cstdint>
#include <functional>
#include <iostream>

namespace
{

    // Fast integer hash function
    std::uint32_t hashU32(std::uint32_t v)
    {
        v ^= v >> 16;
        v *= 0x7feb352du;
        v ^= v >> 15;
        v *= 0x846ca68bu;
        v ^= v >> 16;
        return v;
    }

    glm::vec2 seedToOffset2D(int seed)
    {
        std::uint32_t const base{static_cast<std::uint32_t>(seed)};
        std::uint32_t const hx{hashU32(base ^ 0x9e3779b9u)};
        std::uint32_t const hy{hashU32(base ^ 0x85ebca6bu)};

        float const fx{static_cast<float>(hx & 0x00ffffffu) / 16777216.0f};
        float const fy{static_cast<float>(hy & 0x00ffffffu) / 16777216.0f};

        // Large translation range so seeds land on very different 2D Perlin regions.
        return {
            fx * 4096.0f - 2048.0f,
            fy * 4096.0f - 2048.0f};
    }

} // namespace

float perlinNoise(glm::vec2 const &position)
{
    return glm::perlin(position);
}

float perlinNoiseSeeded(glm::vec2 const &position, int seed)
{
    // Cache computed offset because the same seed is used for many samples per frame.
    static int cachedSeed{};
    static glm::vec2 cachedOffset{};

    if (seed != cachedSeed)
    {
        cachedSeed = seed;
        cachedOffset = seedToOffset2D(seed);
    }

    return glm::perlin(position + cachedOffset);
}

float radialMask(glm::vec2 const &position)
{
    glm::vec2 center = {0.5f, 0.5f};//center of the map (supposing its size is 1)
    float d = glm::distance(position, center); //distance between the pixel and the center
    float p = 1-d*4; //put the value to 1 and inverse it
    p = glm::clamp(p, -1.f, 1.f);//keep the value between -1 and 1
    return p;
}

float octaveNoise(glm::vec2 const &position, glm::vec2 const &normalP, std::function<float(glm::vec2 const &)> noiseFunction)
{
    // TODO(student): Implement octave/fractal noise accumulation.
    float H = 1.0f; //smoothness of the land
    float G = exp2(-H);// amplitude decay
    float f = 1.0; //  frequency
    float a = 1.0;//  amplitude
    float t = 0;//  noise value
    for (int i = 0; i < 5; i++) //i= number of octaves
    {
        t += a * noiseFunction(f * position);//apply noise map
        f *= 2.0; // increase frequency
        a *= G; //increase amplitude
    }
    t += radialMask(normalP); //add the mask to give the land a form of an island
    return t;
    // Temporary fallback return directly from the provided noise function for testing.
    // return noiseFunction(position);
}