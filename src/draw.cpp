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

    for (Object const &obj : context.objectPositions)
    {
        Matrix const objectTranslation{MatrixTranslate(
            obj.position.x * context.terrainSize.x,
            obj.position.z * context.terrainSize.y + cubeHalfHeight,
            obj.position.y * context.terrainSize.z)};
        Matrix const centeredTranslation{MatrixMultiply(objectTranslation, terrainCentering)};
        Matrix const scale{MatrixScale(context.cubeScale, context.cubeScale, context.cubeScale)};
        Matrix const transform{MatrixMultiply(scale, centeredTranslation)};
        DrawMesh(context.cube, context.cubeMaterial, transform);
    }
}

void drawObjects(AppContext const &context, Matrix const &terrainCentering)
{
    if (context.objectPositions.empty() || context.objectModels.empty())
        return;

    for (Object const &obj : context.objectPositions)
    {
        Matrix const objectTranslation{MatrixTranslate(
            obj.position.x * context.terrainSize.x,
            obj.position.z * context.terrainSize.y, // no half-height offset needed unless your model pivot is centered
            obj.position.y * context.terrainSize.z)};

        Matrix const centeredTranslation{MatrixMultiply(objectTranslation, terrainCentering)};

        float modelScale = 1.0f; // default value
        if (context.objectScales.contains(obj.modelType))
            modelScale = context.objectScales.at(obj.modelType);
        modelScale *= context.cubeScale;

        Matrix const scale{MatrixScale(modelScale, modelScale, modelScale)};
        Matrix const transform{MatrixMultiply(scale, centeredTranslation)};

        // model corresponding to the type of the object
        auto it = context.objectModels.find(obj.modelType);
        if (it == context.objectModels.end()) // if no model corresponds to the type
        {
            continue;
        }

        Model const &model = it->second;

        // each mesh of the model with its own material
        for (int i = 0; i < model.meshCount; i++)
        {
            DrawMesh(
                model.meshes[i],
                model.materials[model.meshMaterial[i]],
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
            context.currentBiome = Biome::Forest; // setting biome

            // choice of 3D model :
            std::filesystem::path modelPath { pathUtils::make_absolute_path("resources/3DModels/forestTree/forestTree.gltf") };
            context.objectModels[BiomeModel::ForestTree] = LoadModel(modelPath.string().c_str());
            context.objectScales[BiomeModel::ForestTree] = 0.1f; // rescaling

            // rock
            std::filesystem::path rockPath {pathUtils::make_absolute_path("resources/3DModels/desertRock/rock.obj")};
            context.objectModels[BiomeModel::Rock] = LoadModel(rockPath.string().c_str());
            context.objectScales[BiomeModel::Rock] = 1.5f; // rescaling

            // loading textures
            Texture2D texRock = LoadTexture(pathUtils::make_absolute_path("resources/3DModels/desertRock/textures/Material_baseColor.png").string().c_str());
            if (context.objectModels[BiomeModel::Rock].materialCount > 0)
                SetMaterialTexture(&context.objectModels[BiomeModel::Rock].materials[0], MATERIAL_MAP_DIFFUSE, texRock);

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
            drawObjects(context, getTerrainCenteringMatrix(context));
        }

        if (ImGui::Button("Desert"))
        {            
            context.currentBiome = Biome::Desert; // setting biome

            // choice of 3D model :
            // palmTree
            std::filesystem::path treePath {pathUtils::make_absolute_path("resources/3DModels/desertTree/palmTree.obj")};
            context.objectModels[BiomeModel::PalmTree] = LoadModel(treePath.string().c_str());
            context.objectScales[BiomeModel::PalmTree] = 1.5f; // rescaling

            // loading textures
            Texture2D texLeaf = LoadTexture(pathUtils::make_absolute_path("resources/3DModels/desertTree/textures/mIdrTreePalmLeaf_baseColor.png").string().c_str());
            Texture2D texTrunk = LoadTexture(pathUtils::make_absolute_path("resources/3DModels/desertTree/textures/mIdrTreePalmTrunk_baseColor.png").string().c_str());

            if (context.objectModels[BiomeModel::PalmTree].materialCount > 0)
                SetMaterialTexture(&context.objectModels[BiomeModel::PalmTree].materials[0], MATERIAL_MAP_DIFFUSE, texLeaf);
            if (context.objectModels[BiomeModel::PalmTree].materialCount > 1)
                SetMaterialTexture(&context.objectModels[BiomeModel::PalmTree].materials[1], MATERIAL_MAP_DIFFUSE, texTrunk);
            // rock
            std::filesystem::path rockPath {pathUtils::make_absolute_path("resources/3DModels/desertRock/rock.obj")};
            context.objectModels[BiomeModel::Rock] = LoadModel(rockPath.string().c_str());
            context.objectScales[BiomeModel::Rock] = 0.75f; // rescaling

            // loading textures
            Texture2D texRock = LoadTexture(pathUtils::make_absolute_path("resources/3DModels/desertRock/textures/Material_baseColor.png").string().c_str());
            if (context.objectModels[BiomeModel::Rock].materialCount > 0)
                SetMaterialTexture(&context.objectModels[BiomeModel::Rock].materials[0], MATERIAL_MAP_DIFFUSE, texRock);


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
            context.currentBiome = Biome::SnowyMountain; // setting biome

            // snowman
            std::filesystem::path snowmanPath {pathUtils::make_absolute_path("resources/3DModels/snowman/snowman.obj")};
            context.objectModels[BiomeModel::Snowman] = LoadModel(snowmanPath.string().c_str());
            context.objectScales[BiomeModel::Snowman] = 1.25f; // rescaling

            // loading textures
            Texture2D texSnowman = LoadTexture(pathUtils::make_absolute_path("resources/3DModels/snowman/textures/winter_snowman_baseColor.png").string().c_str());
            if (context.objectModels[BiomeModel::Snowman].materialCount > 0)
                SetMaterialTexture(&context.objectModels[BiomeModel::Snowman].materials[0], MATERIAL_MAP_DIFFUSE, texSnowman);
            // house
            std::filesystem::path housePath {pathUtils::make_absolute_path("resources/3DModels/house/house.obj")};
            context.objectModels[BiomeModel::House] = LoadModel(housePath.string().c_str());
            context.objectScales[BiomeModel::House] = 0.25f; // rescaling

            // loading textures
            Texture2D texHouse = LoadTexture(pathUtils::make_absolute_path("resources/3DModels/house/textures/Medieval_baseColor.png").string().c_str());
            if (context.objectModels[BiomeModel::House].materialCount > 0)
                SetMaterialTexture(&context.objectModels[BiomeModel::House].materials[0], MATERIAL_MAP_DIFFUSE, texHouse);

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
            generateObjectsPositions(context); // refreshes 3D models
        }

        if (ImGui::Button("Candy kingdom"))
        {
            context.currentBiome = Biome::CandyKingdom; // setting biome

            // cake
            std::filesystem::path cakePath {pathUtils::make_absolute_path("resources/3DModels/cake/cake.obj")};
            context.objectModels[BiomeModel::Cake] = LoadModel(cakePath.string().c_str());
            context.objectScales[BiomeModel::Cake] = 0.05f; // rescaling

            // loading textures
            Texture2D texCake = LoadTexture(pathUtils::make_absolute_path("resources/3DModels/cake/textures/Mdl_37_Mtl_m_fg_cake_baseColor.png").string().c_str());
            if (context.objectModels[BiomeModel::Cake].materialCount > 0)
                SetMaterialTexture(&context.objectModels[BiomeModel::Cake].materials[0], MATERIAL_MAP_DIFFUSE, texCake);

            
            // candy
            std::filesystem::path candyPath {pathUtils::make_absolute_path("resources/3DModels/candy/candy.obj")};
            context.objectModels[BiomeModel::Candy] = LoadModel(candyPath.string().c_str());
            context.objectScales[BiomeModel::Candy] = 0.05f; // rescaling

            // loading textures
            Texture2D texCandy = LoadTexture(pathUtils::make_absolute_path("resources/3DModels/candy/textures/Mdl_39_Mtl_m_fg_candy_baseColor.png").string().c_str());
            if (context.objectModels[BiomeModel::Candy].materialCount > 0)
                SetMaterialTexture(&context.objectModels[BiomeModel::Candy].materials[0], MATERIAL_MAP_DIFFUSE, texCandy);
    
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
            generateObjectsPositions(context); // refreshes 3D models
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
        ImGui::SliderFloat("Scale", &context.cubeScale, 0.01f, 3.0f);
    }
}

void drawRaylibUI(AppContext &context)
{
    int screenWidth{GetScreenWidth()};

    float wanted_size{200.f};
    float scale_factor{wanted_size / std::max(context.texture.width, context.texture.height)};
    float const preview_x{screenWidth - wanted_size - 20.f};
    float const preview_y{20.f};
    float const preview_w{context.texture.width * scale_factor};
    float const preview_h{context.texture.height * scale_factor};
    // DrawTexture(context.texture, screenWidth - context.texture.width - 20, 20, WHITE);
    DrawTextureEx(context.texture, {preview_x, preview_y}, 0.0f, scale_factor, WHITE);
    DrawRectangleLines(screenWidth - wanted_size - 20, 20, wanted_size, wanted_size, GREEN);

    // draw positions on top of the heightmap
    for (auto const &obj : context.objectPositions)
    {
        // Remap normalized coordinates [0..1] to the preview image in screen space.
        float const px{preview_x + Clamp(obj.position.x, 0.0f, 1.0f) * preview_w};
        float const py{preview_y + Clamp(obj.position.y, 0.0f, 1.0f) * preview_h};

        DrawCircleV({px, py}, 2.0f, RED);
    }

    DrawFPS(10, 10);
}
