#include "generation.hpp"

#include "noise.hpp"
#include "raylib.h"

#include "utils/raylibUtils.hpp"
#include <algorithm> // for std::clamp


std::vector<glm::vec2> generate2DPositions([[maybe_unused]] PointsGenerationParameters const& params) {
    std::vector<glm::vec2> positions {};

    positions.reserve(1000);
    // Naive random generation
   /*  for (int i {0}; i < 1000; ++i)
    {
        positions.emplace_back(
            static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX),
            static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX)
        );
    }
 */
    // TODO(student): implement Poisson disk sampling to replace the above naive random generation
    // points output should be in [0..1] range, where (0,0) is one corner of the terrain and (1,1) is the opposite corner, so they can be easily scaled to terrain size and sampled from heightmap.
   
    int cell_size {params.r/sqrt(2)};
    std::vector<glm::vec2> grid {}; // active list
    grid.reserve(1000);

    // randomly choosing the first point and adding it to the output list positions
    glm::vec2 first_point {static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX), static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX)};
    positions.emplace_back(first_point);
    // and to the grid 
    grid.emplace_back(first_point);

   while (!grid.empty()) // while the active list is not empty
   {
        // choosing a random point from the active list : 
        int random_i {static_cast<int>(GetRandomValue(0, 1000))};
        glm::vec2 current_point {grid[random_i]};
        int wrong_distance {0}; // counts how many potential new_points do not follow the rule of the min distance when generating them inside the loop

        for (int i {0}; i < params.k < i++)
        {
            // generate new point around current_point (within the annulus surrounding the point)
            glm::vec2 new_point {generateRandomPointAround(current_point, params.r)};
            bool keep_point;

            for (glm::vec2 point : positions) 
            {
                if (sqrt((new_point.x - current_point.x)*(new_point.x  - current_point.x) + (new_point.y - current_point.y)*(new_point.y - current_point.y)) < params.r) // if the distance between this new point and others is lower than the minimal distance between points
                {
                    !keep_point;
                    wrong_distance++;
                    break; // this new_point cannot exist
                }
            }

            if (!keep_point) // if the point couldn't exist
            {
                break; // try to generate another point within the annulus
            }
            // if the distance between new_point and all other points is of at least params.r : 
            positions.emplace_back(new_point);
            grid.emplace_back(new_point);
        }

        if (wrong_distance == params.k) // after k tries, no generated new point follows the rules
        {
            grid.pop_back(); // current_point is deleted from the active list
        }
   }

    return positions;
}

glm::vec2 generateRandomPointAround(glm::vec2 point, float r)
{
    float r1 = static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX);
    float r2 = static_cast<float>(GetRandomValue(0, INT_MAX)) / static_cast<float>(INT_MAX);

    float radius = r * (r1 + 1.f);
    float angle  = 2.f * M_PI * r2;

    glm::vec2 new_point = {
        point.x + radius * cosf(angle),
        point.y + radius * sinf(angle)
    };

    return new_point;
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

            return (perlinNoiseSeeded(p * context.imageGenerationParameters.noiseScale, context.imageGenerationParameters.noiseSeed) * 0.5f + 0.5f);
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