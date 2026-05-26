#include "draw.hpp"

#include "app.hpp"

#include "generation.hpp"

#include "imgui.h"
#include "raylib.h"
#include "raymath.h"

#include <filesystem> // for 3D models paths
#include "utils/pathUtils.hpp" // for 3D models paths

// drawing objects :
void drawCubes(AppContext const &context, Matrix const &terrainCentering)
{
    if (context.objectPositions.empty())
    {
        return;
    }

    float const cubeHalfHeight{0.5f * context.cubeScale};

    for (glm::vec3 const &pos : context.objectPositions)
    {
        Matrix const objectTranslation{MatrixTranslate(
            pos.x * context.terrainSize.x,
            pos.z * context.terrainSize.y + cubeHalfHeight,
            pos.y * context.terrainSize.z)};
        Matrix const centeredTranslation{MatrixMultiply(objectTranslation, terrainCentering)};
        Matrix const scale{MatrixScale(context.cubeScale, context.cubeScale, context.cubeScale)};
        Matrix const transform{MatrixMultiply(scale, centeredTranslation)};
        DrawMesh(context.cube, context.cubeMaterial, transform);
    }
}

void drawObjects(AppContext const &context, Matrix const &terrainCentering)
{
    if (context.objectPositions.empty() || context.objectModel.meshCount == 0)
        return;

    for (glm::vec3 const &pos : context.objectPositions)
    {
        Matrix const objectTranslation{MatrixTranslate(
            pos.x * context.terrainSize.x,
            pos.z * context.terrainSize.y, // no half-height offset needed unless your model pivot is centered
            pos.y * context.terrainSize.z)};

        Matrix const centeredTranslation{MatrixMultiply(objectTranslation, terrainCentering)};
        Matrix const scale{MatrixScale(context.cubeScale, context.cubeScale, context.cubeScale)};
        Matrix const transform{MatrixMultiply(scale, centeredTranslation)};

        // each mesh of the model with its own material
        for (int i = 0; i < context.objectModel.meshCount; i++)
        {
            DrawMesh(
                context.objectModel.meshes[i],
                context.objectModel.materials[context.objectModel.meshMaterial[i]],
                transform);
        }
    }
}

void draw3DScene(AppContext &context)
{
    ClearBackground(RAYWHITE);

    BeginMode3D(context.camera);

    Matrix const terrainCentering{getTerrainCenteringMatrix(context)};
    Vector3 const terrainCenterOffset{terrainCentering.m12, terrainCentering.m13, terrainCentering.m14};

    DrawModel(context.model, terrainCenterOffset, 1.0f, WHITE);
    // for cubes :
    // drawCubes(context, terrainCentering);
    // for 3D objects :
    drawObjects(context, terrainCentering);
    DrawGrid(20, 1.0f);

    EndMode3D();
}

void drawImGui(AppContext &context)
{
    if (ImGui::Button("Generate random positions"))
    {
        generateObjectsPositions(context);
    }

    if (ImGui::CollapsingHeader("Terrain"))
    {
        ImGui::SliderInt("Seed", &context.imageGenerationParameters.noiseSeed, 0, 100000);
        ImGui::SliderFloat("Noise Scale", &context.imageGenerationParameters.noiseScale, 0.1f, 10.0f);
        ImGui::SliderInt("Resolution", &context.imageGenerationParameters.resolution, 64, 512);

        if (ImGui::Button("Regenerate Terrain"))
        {
            generateHeightmap(context);        // regenerate the map
            regenerateMeshFromImage(context);  // regenerate the colors
            generateObjectsPositions(context); // regenrate objects positions
        }
    }

    // biome selection
    if (ImGui::CollapsingHeader("Biome selection", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Button("Forest"))
        {
            // choice of 3D model :
            std::filesystem::path modelPath { pathUtils::make_absolute_path("resources/3DModels/forestTree/forestTree.gltf") };
            context.objectModel = LoadModel(modelPath.string().c_str());

            context.colors = {
                {-0.5f, color_from({49, 51, 84})},   // deep water
                {0.1f, color_from({101, 133, 166})}, // water
                {0.3f, color_from({238, 214, 175})}, // sand
                {0.5f, color_from({45, 107, 49})},   // grass
                {0.7f, color_from({45, 107, 49})},   // grass
                {1.0f, color_from({223, 237, 236})}, // snow
            };
            generateHeightmap(context);
            regenerateMeshFromImage(context); // regenerates the colors
            generateObjectsPositions(context); // refreshes 3D models
        }

        if (ImGui::Button("Desert"))
        {
            // choice of 3D model :
            std::filesystem::path modelPath { pathUtils::make_absolute_path("resources/3DModels/palmTree/palmTree.gltf") };
            context.objectModel = LoadModel(modelPath.string().c_str());
            context.cubeScale = 0.01;

            context.colors = {
                {-0.5f, color_from({33, 118, 196})}, // deep water
                {0.1f, color_from({66, 197, 245})},  // water
                {0.3f, color_from({255, 236, 209})}, // sand
                {0.5f, color_from({255, 125, 0})},   // ground
                {0.7f, color_from({255, 125, 0})},   // ground
                {1.0f, color_from({120, 41, 15})},   // peak
            };
            generateHeightmap(context);
            regenerateMeshFromImage(context); // regenerates the colors
            generateObjectsPositions(context); // refreshes 3D models
        }

        if (ImGui::Button("Snowy mountain"))
        {
            context.colors = {
                {-0.5f, color_from({33, 157, 199})}, // deep water
                {0.1f, color_from({140, 230, 255})}, // water
                {0.3f, color_from({247, 253, 255})}, // sand
                {0.5f, color_from({140, 230, 255})}, // ground
                {0.7f, color_from({191, 219, 247})}, // ground
                {1.0f, color_from({255, 255, 255})}, // peak
            };
            generateHeightmap(context);
            regenerateMeshFromImage(context); // regenerate the colors
        }

        if (ImGui::Button("Candy kingdom"))
        {
            context.colors = {
                {-0.5f, color_from({56, 176, 80})}, // deep water
                {0.1f, color_from({141, 224, 158})}, // water
                {0.3f, color_from({226, 21, 141})}, // sand
                {0.5f, color_from({253, 119, 173})}, // ground
                {0.7f, color_from({242, 181, 212})}, // ground
                {1.0f, color_from({250, 205, 228})}, // peak
            };
            generateHeightmap(context);
            regenerateMeshFromImage(context); // regenerate the colors
        }
    }

    if (ImGui::CollapsingHeader("Color picker"))
    {
        static const char *height[] = {
            "Deep water", "Water", "Sand", "Ground 1", "Ground 2", "Snow"};

        for (int i = 0; i < (int)context.colors.size(); i++)
        {
            float col[3] = {context.colors[i].color.r / 255.f,
                            context.colors[i].color.g / 255.f,
                            context.colors[i].color.b / 255.f};

            if (ImGui::ColorEdit3(height[i], col))
            {
                context.colors[i].color.r = (col[0] * 255);
                context.colors[i].color.g = (col[1] * 255);
                context.colors[i].color.b = (col[2] * 255);
            }
        }

        if (ImGui::Button("Regenerate Terrain"))
        {
            generateHeightmap(context);
            regenerateMeshFromImage(context);
            generateObjectsPositions(context);
        }
    }

    if (ImGui::CollapsingHeader("objects", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Cube Scale", &context.cubeScale, 0.01f, 1.0f);
    }
}

void drawRaylibUI(AppContext &context)
{
    int screenWidth{GetScreenWidth()};

    float wanted_size{400.f};
    float scale_factor{wanted_size / std::max(context.texture.width, context.texture.height)};
    float const preview_x{screenWidth - wanted_size - 20.f};
    float const preview_y{20.f};
    float const preview_w{context.texture.width * scale_factor};
    float const preview_h{context.texture.height * scale_factor};
    // DrawTexture(context.texture, screenWidth - context.texture.width - 20, 20, WHITE);
    DrawTextureEx(context.texture, {preview_x, preview_y}, 0.0f, scale_factor, WHITE);
    DrawRectangleLines(screenWidth - wanted_size - 20, 20, wanted_size, wanted_size, GREEN);

    // draw positions on top of the heightmap
    for (auto const &pos : context.objectPositions)
    {
        // Remap normalized coordinates [0..1] to the preview image in screen space.
        float const px{preview_x + Clamp(pos.x, 0.0f, 1.0f) * preview_w};
        float const py{preview_y + Clamp(pos.y, 0.0f, 1.0f) * preview_h};

        DrawCircleV({px, py}, 2.0f, RED);
    }

    DrawFPS(10, 10);
}
