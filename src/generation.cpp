#include "generation.hpp"

#include "noise.hpp"
#include "raylib.h"

#include "utils/raylibUtils.hpp"
#include <algorithm> // for std::clamp
#include <cassert>
#include <cmath> // for std::ceil

glm::vec2 generateRandomPointAround(const glm::vec2 point, const float r)
{
    // generating 2 random points between 0 and 1 :
    float r1 = static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX);
    float r2 = static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX);

    float radius = r * (r1 + 1.f);
    float angle  = 2.f * M_PI * r2;

    glm::vec2 candidate = {
        point.x + radius * cosf(angle),
        point.y + radius * sinf(angle)
    };

    return candidate;
}

glm::vec2 imageToGrid(const glm::vec2 point, const float cellSize, const int w, const int h) // grid coordinates of a point
{
    int gridX {std::clamp(static_cast<int>(point.x / cellSize), 0, w - 1)};
    int gridY {std::clamp(static_cast<int>(point.y / cellSize), 0, h - 1)};

    return {static_cast<float>(gridX), static_cast<float>(gridY)};
}

int toCell(const glm::vec2 point, const float cellSize, const int w) // index of cell based on coordinates of the point
{
    int cx {std::clamp(static_cast<int>(point.x / cellSize), 0, w - 1)};
    int cy {std::clamp(static_cast<int>(point.y / cellSize), 0, w - 1)};

    return cy * w + cx;
}

float distance(const glm::vec2 point1, const glm::vec2 point2)
{
    return sqrt((point2.x - point1.x)*(point2.x  - point1.x) + (point2.y - point1.y)*(point2.y - point1.y));
}

bool isValid(const glm::vec2 candidate, const float cellSize, const float r, const std::vector<glm::vec2>& positions, const std::vector<std::vector<int>>& grid, const int w, const int h)
{
    if (candidate.x >= 0.f && candidate.x < 1.f && candidate.y >= 0.f && candidate.y < 1.f) // working in the normalized space [0;1]
    {
        glm::vec2 cellCoord {imageToGrid(candidate, cellSize, w, h)}; // grid coordinates of candidate

        int searchStartX {std::max(0, static_cast<int>(std::ceil(cellCoord.x -2)))};
        int searchEndX {std::min(static_cast<int>(std::ceil(cellCoord.x + 2)), w - 1)};

        int searchStartY {std::max(0, static_cast<int>(std::ceil(cellCoord.y - 2)))};
        int searchEndY {std::min(static_cast<int>(std::ceil(cellCoord.y + 2)), h - 1)};

        for (int x {searchStartX}; x <= searchEndX; x++) 
        {
            for (int y {searchStartY}; y <= searchEndY; y++) 
            {
                int i {grid[y][x]}; // index of the point in the cell grid(y,x)

                if (i != -1) // if there do is a point in that cell
                {
                    if (distance(candidate, positions[i]) < r)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    return false;
}

std::vector<glm::vec2> generate2DPositions([[maybe_unused]] PointsGenerationParameters const& params) {
    std::vector<glm::vec2> positions {}; // our result 
    
    // Naive random generation
/*     for (int i {0}; i < params.max_points; ++i)
    {
        positions.emplace_back(
            static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX),
            static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX)
        );
    } */
    
    // TODO(student): implement Poisson disk sampling to replace the above naive random generation
    // points output should be in [0..1] range, where (0,0) is one corner of the terrain and (1,1) is the opposite corner, so they can be easily scaled to terrain size and sampled from heightmap.
   
    // definition of the background grid : 
    float cellSize {params.r/(float)sqrt(2)};
    int w {static_cast<int>(1.0f / cellSize)};
    int h {w}; // working with squared cells
    std::vector<std::vector<int>> grid(h, std::vector<int>(w, -1)); // (nb of rows, (nb of columns, values))
    // -1 in a cell signifies no sample is in it, else, grid[y][x] stores the index of the sample located in that cell

    positions.reserve(params.max_points); // params.max_points is the maximum potential number of points that can be generated, in the end, on the map
    std::vector<int> active_list {}; // stores potential candidates

    // randomly choosing the first point/sample
    glm::vec2 first_point {static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX), static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX)};

    glm::vec2 gridCoordFirst = imageToGrid(first_point, cellSize, w, h);
    grid[static_cast<int>(gridCoordFirst.y)][static_cast<int>(gridCoordFirst.x)] = 0; // first_point is the center of the spawn of new points so index 0

    active_list.emplace_back(0);
    positions.emplace_back(first_point);

    while (!active_list.empty())
    {
        // choosing a random index from the active list : 
        int random_i {static_cast<int>(GetRandomValue(0, static_cast<int>(active_list.size()) - 1))};
        glm::vec2 current_point {positions[active_list[random_i]]};

        bool accepted {false};

        for (int i {0}; i < params.k ; i++)
        {
            // generating a new point around current_point (within the annulus surrounding the point)
            glm::vec2 candidate {generateRandomPointAround(current_point, params.r)};

            if (candidate.x < 0.f || candidate.x > 1.f || candidate.y < 0.f || candidate.y > 1.f) // out of bound
            {
                continue;
            }

            if (isValid(candidate, cellSize, params.r, positions, grid, w, h)) // the candidate follows the min distance rule for all of its neighbours in its neighbourhood ie 5*5 squares around
            {
                int candidate_i {static_cast<int>(positions.size())}; // last in positions

                positions.emplace_back(candidate);
                active_list.emplace_back(candidate_i);

                glm::vec2 gridCoordCand {imageToGrid(candidate, cellSize, w, h)};
                grid[gridCoordCand.y][gridCoordCand.x] = candidate_i;

                accepted = true;
                break; // a working neighbour candidate was found for this current_point
            }
        }

        if (!accepted) // after k tries, no generated candidate around current_point follows the rule of min distance
        {
            active_list[random_i] = active_list.back();
            active_list.pop_back(); // current_point is deleted from the active list
        }
    }

    return positions;
}

void generateObjectsPositions(AppContext& context) {
    std::vector<glm::vec2> const positions {generate2DPositions(context.pointsGenerationParameters)};

    context.objectPositions.clear();
    context.objectPositions.reserve(positions.size());
    for (glm::vec2 const& p : positions)
    {
        context.objectPositions.emplace_back(
            p.x, // x
            p.y, // y
            // sample height from heightmap for each point (asuming positions are normalized in [0..1] range)
            sampleHeightmap(context, p.x, p.y)
        );
    }
    // TODO(student): extension - filter positions by sampled height range.
}

float sampleHeightmap(AppContext const& context, float u, float v)
{
    if (!context.heightmapImage.data || context.heightmapImage.width <= 0 || context.heightmapImage.height <= 0) return 0.0f;

    int const px = std::clamp(static_cast<int>(u * static_cast<float>(context.heightmapImage.width - 1)), 0, context.heightmapImage.width - 1);
    int const py = std::clamp(static_cast<int>(v * static_cast<float>(context.heightmapImage.height - 1)), 0, context.heightmapImage.height - 1);

    // If the heightmap is in R32 format, we can directly read the height value as a float. 
    if (context.heightmapImage.format == PIXELFORMAT_UNCOMPRESSED_R32)
    {
        float const* heightData = static_cast<float const*>(context.heightmapImage.data);
        int const idx = py * context.heightmapImage.width + px;
        return std::clamp(heightData[idx], 0.0f, 1.0f);
    }

    // Otherwise, we assume it's in a color format and we read the red channel as height (with normalization from [0..255] to [0..1]).
    Color const c = GetImageColor(context.heightmapImage, px, py);
    return static_cast<float>(c.r)/255.0f;
}

void generateHeightmap(AppContext& context) {

    if (context.texture.id > 0) {
        UnloadTexture(context.texture);
        context.texture = {};
    }

    if(context.image.data) {
        UnloadImage(context.image);
        context.image = {};
    }

    if (context.heightmapImage.data) {
        UnloadImage(context.heightmapImage);
        context.heightmapImage = {};
    }

    int const resolution = std::max(1, context.imageGenerationParameters.resolution);

    context.heightmapImage = GenImageFromNoiseFunction<float>(resolution, resolution, PIXELFORMAT_UNCOMPRESSED_R32,
        [&](glm::vec2 const& p)->float {
            // TODO(student): implement stack based noise and island mask
            auto noiseFunction = [&] (glm::vec2 const& position) -> float {
                return perlinNoiseSeeded(position, context.imageGenerationParameters.noiseSeed);
            };

            return (octaveNoise(p * context.imageGenerationParameters.noiseScale,p, noiseFunction) * 0.5f + 0.5f);
            // return (perlinNoiseSeeded(p * context.imageGenerationParameters.noiseScale, context.imageGenerationParameters.noiseSeed) * 0.5f + 0.5f);
        });

    // exemple conversion from heightmap to color image
    context.image = TransformImage<float, Color>(context.heightmapImage, [&](float const& v, int const, int const) {
        if (v < 0.3f)
        {
            return color_from({ 70, 130, 180 }); // water
        }
        else if (v < 0.5f)
        {
            return color_from({ 238, 214, 175 }); // sand
        }
        else
        {
            return color_from({ 34, 139, 34 }); // grass
        }
        
    }, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    context.texture = LoadTextureFromImage(context.image);
    if (context.model.meshCount > 0) {
        context.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = context.texture;
    }
}