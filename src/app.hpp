#pragma once

#include "raylib.h"
#include "glm/glm.hpp"
#include <vector>
#include <unordered_map> // for modified objectModels

#include "utils/raylibUtils.hpp"

struct ImageGenerationParameters 
{
    int noiseSeed { 0 };
    float noiseScale { 4.0f };
    int resolution { 256 };
};

struct PointsGenerationParameters 
{
    // TODO(student): add parameters for points generation (ex: poisson disk radius, etc).
    float r {0.05}; // Poisson disk radius, represents the minimal distance between 2 points (2 samples)
    int k {30}; // maximum number of attempts before a candidate is rejected in the Poisson algorithm
    int max_points {1000}; // maximum number of points that can be generated in the end on the map
};

//colors depending on height
struct ColorStop 
{ 
    float height; 
    Color color; 
};

// all available biomes
enum class Biome
{
    Forest,
    Desert,
    SnowyMountain,
    CandyKingdom
};

// to place 3D models depending on the height : 
enum class BiomeModel // all model types used in biomes
{
    PalmTree,
    ForestTree,
    Rock,
    Snowman,
    House,
    Candy,
    Cake
};

// BiomeModel is the key in the unordered_map used in objectModel
// hash function :
namespace std
{
    template<>
    struct hash<BiomeModel> 
    {
        std::size_t operator()(const BiomeModel& b) const 
        {
            return std::hash<int>{}(static_cast<int>(b)); // converting BiomeModel variable in int and using standard hashing for ints
        }
    };
}

struct Object // informations about 3D models objects
{
    glm::vec3 position; // its position
    BiomeModel modelType; // its type
};


struct AppContext 
{
    Camera camera {};

    // Store the heightmap as a raylib Image, which is easy to sample from CPU side when generating object positions.
    Image heightmapImage {};

    // This is the image we use for texturing the terrain. It can be the same as heightmapImage, but it doesn't have to be (for example, you could use a color image with RGB channels representing different material types instead of height).
    Image image {};

    // The generated texture from the image, stored here so we can easily bind it when generating the model.
    Texture2D texture {};

    //colors for of the map
    std::vector<ColorStop> colors = {
    { -0.5f, color_from({ 49, 51, 84 }) },  // deep water
    { 0.1f, color_from({ 101, 133, 166 }) },  // water
    { 0.3f, color_from({ 238, 214, 175 }) },  // sand
    { 0.5f, color_from({ 45, 107, 49  }) },  // grass
    { 0.7f, color_from({ 45, 107, 49  }) },  // grass
    { 1.0f, color_from({ 223, 237, 236 }) },  // snow
    };

    glm::vec3 terrainSize { 16.0f, 5.0f, 16.0f };

    // The generated terrain mesh and model.
    Mesh mesh {};
    Model model {};
    // selected biome
    Biome currentBiome {Biome::Forest}; // by default, first biome is Forest

    std::vector<Object> objectPositions {}; // now stores the position and the type of the model

    // A simple cube mesh and material we use to draw objects on the terrain.
    Mesh cube {};
    Material cubeMaterial {};
    float cubeScale { 1.0f }; 
    
    std::unordered_map<BiomeModel, float> objectScales {};

    std::unordered_map<BiomeModel, Model> objectModels {}; // now stores models and their type

    // Parameters for object positions generation
    PointsGenerationParameters pointsGenerationParameters;

    // Parameters for island generation
    ImageGenerationParameters imageGenerationParameters;
};

Matrix getTerrainCenteringMatrix(AppContext const& context);
float sampleHeightmap(AppContext const& context, float u, float v);
void unload(AppContext& context);
void regenerateMeshFromImage(AppContext& context);