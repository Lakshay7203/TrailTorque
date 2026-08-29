#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <box2d/box2d.h>
#include <vector>
#include "Bike.h"
#include "Input.h"
#include "Terrain.h"

// ---------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------

constexpr float SCREEN_WIDTH = 1280.0f;
constexpr float SCREEN_HEIGHT = 720.0f;

constexpr float PIXELS_PER_METER = 30.0f;

constexpr float SCREEN_CENTER_X = SCREEN_WIDTH / 2.0f;
constexpr float SCREEN_CENTER_Y = SCREEN_HEIGHT / 2.0f;

constexpr float CAMERA_TARGET_X = 400.0f;

constexpr float GROUND_HALF_WIDTH = 100.0f;
constexpr float GROUND_HEIGHT = 2.0f;

constexpr float FINISH_X = 90.0f;
constexpr float CHECKPOINT_X = 35.0f;

void ProcessInput(
    bool& running,
    InputState& input)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            running = false;
        }
    }

    const bool* keyboardState =
        SDL_GetKeyboardState(nullptr);

    input.driveForward =
        keyboardState[SDL_SCANCODE_W];

    input.driveBackward =
        keyboardState[SDL_SCANCODE_S];

    input.leanBackward =
        keyboardState[SDL_SCANCODE_A];

    input.leanForward =
        keyboardState[SDL_SCANCODE_D];

    input.resetPressed =
        keyboardState[SDL_SCANCODE_R];
}

// ---------------------------------------------------------
// DRAW FILLED CIRCLE
// Temporary wheel rendering.
// ---------------------------------------------------------

void DrawFilledCircle(
    SDL_Renderer* renderer,
    float centerX,
    float centerY,
    float radius)
{
    for (float y = -radius; y <= radius; y += 1.0f)
    {
        for (float x = -radius; x <= radius; x += 1.0f)
        {
            if (x * x + y * y <= radius * radius)
            {
                SDL_RenderPoint(
                    renderer,
                    centerX + x,
                    centerY + y
                );
            }
        }
    }
}

void DrawRotatedChassis(
    SDL_Renderer* renderer,
    b2BodyId chassisBodyId,
    float cameraX)
{
    // Get the chassis position + rotation from Box2D.
    b2Transform transform =
        b2Body_GetTransform(chassisBodyId);

    // Chassis corners in LOCAL space.
    b2Vec2 localCorners[4] =
    {
        { -BIKE_CHASSIS_WIDTH / 2.0f, -BIKE_CHASSIS_HEIGHT / 2.0f },
        {  BIKE_CHASSIS_WIDTH / 2.0f, -BIKE_CHASSIS_HEIGHT / 2.0f },
        {  BIKE_CHASSIS_WIDTH / 2.0f,  BIKE_CHASSIS_HEIGHT / 2.0f },
        { -BIKE_CHASSIS_WIDTH / 2.0f,  BIKE_CHASSIS_HEIGHT / 2.0f }
    };

    SDL_Vertex vertices[4]{};

    const SDL_FColor green =
    {
        0.0f,
        180.0f / 255.0f,
        0.0f,
        1.0f
    };

    for (int i = 0; i < 4; ++i)
    {
        // Rotate + move each local corner using Box2D.
        b2Vec2 worldPoint =
            b2TransformPoint(
                transform,
                localCorners[i]
            );

        // Convert Box2D world position to SDL screen position.
        vertices[i].position.x =
            CAMERA_TARGET_X +
            (worldPoint.x - cameraX) *
            PIXELS_PER_METER;

        vertices[i].position.y =
            SCREEN_CENTER_Y -
            worldPoint.y *
            PIXELS_PER_METER;

        vertices[i].color = green;
    }

    // Two triangles make one rectangle.
    const int indices[6] =
    {
        0, 1, 2,
        0, 2, 3
    };

    SDL_RenderGeometry(
        renderer,
        nullptr,
        vertices,
        4,
        indices,
        6
    );
}

void DrawFinishLine(
    SDL_Renderer* renderer,
    float cameraX)
{
    const float finishScreenX =
        CAMERA_TARGET_X +
        (FINISH_X - cameraX) *
        PIXELS_PER_METER;

    // Our normal ground surface is y = -9 in Box2D.
    const float groundScreenY =
        SCREEN_CENTER_Y -
        (-9.0f * PIXELS_PER_METER);


    // =============================================
    // FLAG POLE
    // =============================================

    SDL_SetRenderDrawColor(
        renderer,
        40,
        40,
        40,
        255
    );

    SDL_FRect pole =
    {
        finishScreenX,
        groundScreenY - 170.0f,
        6.0f,
        170.0f
    };

    SDL_RenderFillRect(
        renderer,
        &pole
    );


    // =============================================
    // CHECKERED FLAG
    // =============================================

    const float flagWidth = 90.0f;
    const float flagHeight = 60.0f;

    const int columns = 6;
    const int rows = 4;

    const float cellWidth =
        flagWidth / columns;

    const float cellHeight =
        flagHeight / rows;


    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0;
            column < columns;
            ++column)
        {
            bool blackSquare =
                (row + column) % 2 == 0;

            if (blackSquare)
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    20,
                    20,
                    20,
                    255
                );
            }
            else
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    245,
                    245,
                    245,
                    255
                );
            }


            SDL_FRect square =
            {
                finishScreenX + 6.0f +
                    column * cellWidth,

                groundScreenY - 170.0f +
                    row * cellHeight,

                cellWidth,
                cellHeight
            };

            SDL_RenderFillRect(
                renderer,
                &square
            );
        }
    }
}

void DrawText(
    SDL_Renderer* renderer,
    TTF_Font* font,
    const char* text,
    float x,
    float y,
    SDL_Color color)
{
    SDL_Surface* textSurface =
        TTF_RenderText_Blended(
            font,
            text,
            0,
            color
        );

    if (!textSurface)
    {
        SDL_Log(
            "Text surface creation failed: %s",
            SDL_GetError()
        );

        return;
    }


    SDL_Texture* textTexture =
        SDL_CreateTextureFromSurface(
            renderer,
            textSurface
        );

    if (!textTexture)
    {
        SDL_Log(
            "Text texture creation failed: %s",
            SDL_GetError()
        );

        SDL_DestroySurface(textSurface);
        return;
    }


    SDL_FRect destination =
    {
        x,
        y,
        static_cast<float>(textSurface->w),
        static_cast<float>(textSurface->h)
    };


    SDL_RenderTexture(
        renderer,
        textTexture,
        nullptr,
        &destination
    );


    SDL_DestroyTexture(textTexture);
    SDL_DestroySurface(textSurface);
}

void DrawUI(
    SDL_Renderer* renderer,
    TTF_Font* font,
    bool levelComplete,
    float levelTime,
    float bestTime,
    bool newBestTime)
{
    char timerText[64];

    int minutes =
        static_cast<int>(levelTime) / 60;

    float seconds =
        levelTime -
        static_cast<float>(minutes * 60);

    SDL_snprintf(
        timerText,
        sizeof(timerText),
        "Time: %02d:%05.2f",
        minutes,
        seconds
    );

    SDL_Color white =
    {
        255,
        255,
        255,
        255
    };


    if (!levelComplete)
    {
        DrawText(
            renderer,
            font,
            "W/S - Drive",
            25.0f,
            25.0f,
            white
        );

        DrawText(
            renderer,
            font,
            "A/D - Lean",
            25.0f,
            55.0f,
            white
        );

        DrawText(
            renderer,
            font,
            "R - Reset",
            25.0f,
            85.0f,
            white
        );

        DrawText(
            renderer,
            font,
            timerText,
            SCREEN_WIDTH - 180.0f,
            25.0f,
            white
        );
    }
    else
    {
        DrawText(
            renderer,
            font,
            "LEVEL COMPLETE!",
            SCREEN_WIDTH / 2.0f - 110.0f,
            80.0f,
            white
        );

        char finishTimeText[64];
        char bestTimeText[64];

        int finishMinutes =
            static_cast<int>(levelTime) / 60;

        float finishSeconds =
            levelTime -
            static_cast<float>(finishMinutes * 60);


        int bestMinutes =
            static_cast<int>(bestTime) / 60;

        float bestSeconds =
            bestTime -
            static_cast<float>(bestMinutes * 60);


        SDL_snprintf(
            finishTimeText,
            sizeof(finishTimeText),
            "Finish Time: %02d:%05.2f",
            finishMinutes,
            finishSeconds
        );

        SDL_snprintf(
            bestTimeText,
            sizeof(bestTimeText),
            "Best Time: %02d:%05.2f",
            bestMinutes,
            bestSeconds
        );

        DrawText(
            renderer,
            font,
            finishTimeText,
            SCREEN_WIDTH / 2.0f - 105.0f,
            130.0f,
            white
        );

        DrawText(
            renderer,
            font,
            bestTimeText,
            SCREEN_WIDTH / 2.0f - 105.0f,
            170.0f,
            white
        );

        if (newBestTime)
        {
            DrawText(
                renderer,
                font,
                "NEW BEST!",
                SCREEN_WIDTH / 2.0f - 70.0f,
                210.0f,
                white
            );
        }

        DrawText(
            renderer,
            font,
            "Press R to restart",
            SCREEN_WIDTH / 2.0f - 105.0f,
            260.0f,
            white
        );
    }
}

void DrawCheckpoint(
    SDL_Renderer* renderer,
    float cameraX,
    bool checkpointReached)
{
    // Convert checkpoint world X into screen X.
    const float checkpointScreenX =
        CAMERA_TARGET_X +
        (CHECKPOINT_X - cameraX) *
        PIXELS_PER_METER;

    // Ground surface is y = -9 in Box2D.
    const float groundScreenY =
        SCREEN_CENTER_Y -
        (-9.0f * PIXELS_PER_METER);

    // Yellow before reaching it, green afterwards.
    if (checkpointReached)
    {
        SDL_SetRenderDrawColor(
            renderer,
            50,
            220,
            80,
            255
        );
    }
    else
    {
        SDL_SetRenderDrawColor(
            renderer,
            255,
            200,
            40,
            255
        );
    }

    SDL_FRect pole =
    {
        checkpointScreenX,
        groundScreenY - 110.0f,
        6.0f,
        110.0f
    };

    SDL_RenderFillRect(
        renderer,
        &pole
    );

    // Small flag.
    SDL_FRect flag =
    {
        checkpointScreenX + 6.0f,
        groundScreenY - 110.0f,
        55.0f,
        30.0f
    };

    SDL_RenderFillRect(
        renderer,
        &flag
    );
}

// ---------------------------------------------------------
// RENDER
// ---------------------------------------------------------

void Render(
    SDL_Renderer* renderer,
    TTF_Font* font,
    SDL_Texture* bikeTexture,
    SDL_Texture* wheelTexture,
    b2BodyId chassisBodyId,
    b2BodyId rearWheelBodyId,
    b2BodyId frontWheelBodyId,
    float cameraX,
    const SDL_FRect& groundRect,
    const SDL_FPoint& rearWheelScreen,
    const SDL_FPoint& frontWheelScreen,
    const std::vector<TerrainSegment>& terrainSegments,
    bool levelComplete,
    bool checkpointReached,
    float levelTime,
    float bestTime,
    bool newBestTime)
{
    // =====================================================
    // SKY
    // =====================================================

    SDL_SetRenderDrawColor(
        renderer,
        135,
        206,
        235,
        255
    );

    SDL_RenderClear(renderer);


    // =====================================================
// DISTANT MOUNTAINS - FAR LAYER
// =====================================================

// Moves very slowly compared with the camera.
    const float farMountainShift =
        -cameraX * 1.5f;

    SDL_Vertex farMountains[12]{};

    const SDL_FColor farMountainColor =
    {
        0.55f,
        0.68f,
        0.58f,
        1.0f
    };


    // Mountain 1.
    farMountains[0].position =
        SDL_FPoint{ -100.0f + farMountainShift, 530.0f };

    farMountains[1].position =
        SDL_FPoint{ 120.0f + farMountainShift, 350.0f };

    farMountains[2].position =
        SDL_FPoint{ 340.0f + farMountainShift, 530.0f };


    // Mountain 2.
    farMountains[3].position =
        SDL_FPoint{ 220.0f + farMountainShift, 530.0f };

    farMountains[4].position =
        SDL_FPoint{ 500.0f + farMountainShift, 320.0f };

    farMountains[5].position =
        SDL_FPoint{ 780.0f + farMountainShift, 530.0f };


    // Mountain 3.
    farMountains[6].position =
        SDL_FPoint{ 650.0f + farMountainShift, 530.0f };

    farMountains[7].position =
        SDL_FPoint{ 900.0f + farMountainShift, 370.0f };

    farMountains[8].position =
        SDL_FPoint{ 1150.0f + farMountainShift, 530.0f };


    // Mountain 4.
    farMountains[9].position =
        SDL_FPoint{ 1050.0f + farMountainShift, 530.0f };

    farMountains[10].position =
        SDL_FPoint{ 1300.0f + farMountainShift, 340.0f };

    farMountains[11].position =
        SDL_FPoint{ 1550.0f + farMountainShift, 530.0f };


    for (int i = 0; i < 12; ++i)
    {
        farMountains[i].color =
            farMountainColor;
    }


    const int farMountainIndices[12] =
    {
        0, 1, 2,
        3, 4, 5,
        6, 7, 8,
        9, 10, 11
    };


    SDL_RenderGeometry(
        renderer,
        nullptr,
        farMountains,
        12,
        farMountainIndices,
        12
    );


    // =====================================================
    // DISTANT MOUNTAINS - NEAR LAYER
    // =====================================================

    // Slightly faster movement = looks closer.
    const float nearMountainShift =
        -cameraX * 3.0f;

    SDL_Vertex nearMountains[9]{};

    const SDL_FColor nearMountainColor =
    {
        0.38f,
        0.55f,
        0.40f,
        1.0f
    };


    // Mountain 1.
    nearMountains[0].position =
        SDL_FPoint{ -150.0f + nearMountainShift, 560.0f };

    nearMountains[1].position =
        SDL_FPoint{ 100.0f + nearMountainShift, 410.0f };

    nearMountains[2].position =
        SDL_FPoint{ 350.0f + nearMountainShift, 560.0f };


    // Mountain 2.
    nearMountains[3].position =
        SDL_FPoint{ 300.0f + nearMountainShift, 560.0f };

    nearMountains[4].position =
        SDL_FPoint{ 620.0f + nearMountainShift, 390.0f };

    nearMountains[5].position =
        SDL_FPoint{ 940.0f + nearMountainShift, 560.0f };


    // Mountain 3.
    nearMountains[6].position =
        SDL_FPoint{ 850.0f + nearMountainShift, 560.0f };

    nearMountains[7].position =
        SDL_FPoint{ 1120.0f + nearMountainShift, 420.0f };

    nearMountains[8].position =
        SDL_FPoint{ 1390.0f + nearMountainShift, 560.0f };


    for (int i = 0; i < 9; ++i)
    {
        nearMountains[i].color =
            nearMountainColor;
    }


    const int nearMountainIndices[9] =
    {
        0, 1, 2,
        3, 4, 5,
        6, 7, 8
    };


    SDL_RenderGeometry(
        renderer,
        nullptr,
        nearMountains,
        9,
        nearMountainIndices,
        9
    );


    // =====================================================
    // BASE DIRT GROUND
    // =====================================================

    SDL_SetRenderDrawColor(
        renderer,
        120,
        75,
        35,
        255
    );

    SDL_RenderFillRect(
        renderer,
        &groundRect
    );


    // =====================================================
    // TERRAIN HILLS
    // =====================================================

    for (const TerrainSegment& terrainSegment : terrainSegments)
    {
        SDL_FPoint startScreen;
        SDL_FPoint endScreen;


        // Box2D world -> SDL screen.
        startScreen.x =
            CAMERA_TARGET_X +
            (terrainSegment.start.x - cameraX) *
            PIXELS_PER_METER;

        startScreen.y =
            SCREEN_CENTER_Y -
            terrainSegment.start.y *
            PIXELS_PER_METER;


        endScreen.x =
            CAMERA_TARGET_X +
            (terrainSegment.end.x - cameraX) *
            PIXELS_PER_METER;

        endScreen.y =
            SCREEN_CENTER_Y -
            terrainSegment.end.y *
            PIXELS_PER_METER;


        // -------------------------------------------------
        // FILL DIRT UNDER THE HILL
        // -------------------------------------------------

        SDL_SetRenderDrawColor(
            renderer,
            120,
            75,
            35,
            255
        );


        float differenceX =
            endScreen.x - startScreen.x;

        if (differenceX != 0.0f)
        {
            int startX =
                static_cast<int>(startScreen.x);

            int endX =
                static_cast<int>(endScreen.x);


            if (startX > endX)
            {
                int temporary = startX;
                startX = endX;
                endX = temporary;
            }


            for (int x = startX; x <= endX; ++x)
            {
                float percentage =
                    (static_cast<float>(x) - startScreen.x)
                    /
                    differenceX;

                float surfaceY =
                    startScreen.y +
                    (endScreen.y - startScreen.y) *
                    percentage;


                SDL_RenderLine(
                    renderer,
                    static_cast<float>(x),
                    surfaceY,
                    static_cast<float>(x),
                    SCREEN_HEIGHT
                );
            }
        }


        // -------------------------------------------------
        // GRASS SURFACE
        // -------------------------------------------------

        SDL_SetRenderDrawColor(
            renderer,
            60,
            160,
            70,
            255
        );


        for (int thickness = -2;
            thickness <= 2;
            ++thickness)
        {
            SDL_RenderLine(
                renderer,
                startScreen.x,
                startScreen.y + thickness,
                endScreen.x,
                endScreen.y + thickness
            );
        }
    }


    // =====================================================
    // FLAT GROUND GRASS
    // =====================================================

    SDL_SetRenderDrawColor(
        renderer,
        60,
        160,
        70,
        255
    );

    for (int thickness = 0;
        thickness < 5;
        ++thickness)
    {
        SDL_RenderLine(
            renderer,
            groundRect.x,
            groundRect.y + thickness,
            groundRect.x + groundRect.w,
            groundRect.y + thickness
        );
    }

    // =====================================================
// FINISH LINE
// =====================================================

    DrawFinishLine(
        renderer,
        cameraX
    );

    DrawCheckpoint(
        renderer,
        cameraX,
        checkpointReached
    );
// =====================================================
// BIKE CHASSIS SPRITE
// =====================================================

    b2Vec2 chassisPosition =
        b2Body_GetPosition(chassisBodyId);

    b2Rot chassisRotation =
        b2Body_GetRotation(chassisBodyId);

    float chassisAngleRadians =
        b2Rot_GetAngle(chassisRotation);

    double chassisAngleDegrees =
        chassisAngleRadians *
        180.0 /
        3.14159265;


    // Convert Box2D position -> SDL screen.
    float chassisScreenX =
        CAMERA_TARGET_X +
        (chassisPosition.x - cameraX) *
        PIXELS_PER_METER;

    float chassisScreenY =
        SCREEN_CENTER_Y -
        chassisPosition.y *
        PIXELS_PER_METER;


    // Size of visual bike sprite.
    // We will tune these after seeing it.
    SDL_FRect bikeRect;

    bikeRect.w = 145.0f;
    bikeRect.h = 48.0f;

    bikeRect.x =
        chassisScreenX -
        bikeRect.w / 2.0f;

    bikeRect.y =
        chassisScreenY -
        bikeRect.h / 2.0f;


    SDL_RenderTextureRotated(
        renderer,
        bikeTexture,
        nullptr,
        &bikeRect,
        -chassisAngleDegrees,
        nullptr,
        SDL_FLIP_NONE
    );

    // =====================================================
    // WHEEL SPRITES
    // =====================================================

    const float wheelDiameter =
        BIKE_WHEEL_RADIUS *
        2.0f *
        PIXELS_PER_METER *
        1.6f;


    // Rear wheel rotation.
    float rearWheelAngleRadians =
        b2Rot_GetAngle(
            b2Body_GetRotation(rearWheelBodyId)
        );

    double rearWheelAngleDegrees =
        rearWheelAngleRadians *
        180.0 /
        3.14159265;


    // Front wheel rotation.
    float frontWheelAngleRadians =
        b2Rot_GetAngle(
            b2Body_GetRotation(frontWheelBodyId)
        );

    double frontWheelAngleDegrees =
        frontWheelAngleRadians *
        180.0 /
        3.14159265;


    // Rear wheel rectangle.
    SDL_FRect rearWheelRect;

    rearWheelRect.w = wheelDiameter;
    rearWheelRect.h = wheelDiameter;

    rearWheelRect.x =
        rearWheelScreen.x -
        rearWheelRect.w / 2.0f;

    rearWheelRect.y =
        rearWheelScreen.y -
        rearWheelRect.h / 2.0f;


    // Front wheel rectangle.
    SDL_FRect frontWheelRect;

    frontWheelRect.w = wheelDiameter;
    frontWheelRect.h = wheelDiameter;

    frontWheelRect.x =
        frontWheelScreen.x -
        frontWheelRect.w / 2.0f +
        10.0f;

    frontWheelRect.y =
        frontWheelScreen.y -
        frontWheelRect.h / 2.0f -
        1.0f;


    // Draw rear wheel.
    SDL_RenderTextureRotated(
        renderer,
        wheelTexture,
        nullptr,
        &rearWheelRect,
        -rearWheelAngleDegrees,
        nullptr,
        SDL_FLIP_NONE
    );


    // Draw front wheel.
    SDL_RenderTextureRotated(
        renderer,
        wheelTexture,
        nullptr,
        &frontWheelRect,
        -frontWheelAngleDegrees,
        nullptr,
        SDL_FLIP_NONE
    );

    // =====================================================
// UI
// =====================================================


    DrawUI(
        renderer,
        font,
        levelComplete,
        levelTime,
		bestTime,
		newBestTime
    );

    // =====================================================
    // PRESENT
    // =====================================================

    SDL_RenderPresent(renderer);
}


// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------

int main(int argc, char* argv[])
{
    // -----------------------------------------------------
    // INITIALIZE SDL
    // -----------------------------------------------------

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log(
            "SDL initialization failed: %s",
            SDL_GetError()
        );

        return 1;
    }

    if (!TTF_Init())
    {
        SDL_Log(
            "SDL_ttf initialization failed: %s",
            SDL_GetError()
        );

        SDL_Quit();
        return 1;
    }


    // -----------------------------------------------------
    // CREATE WINDOW
    // -----------------------------------------------------

    SDL_Window* window = SDL_CreateWindow(
        "TrailTorque",
        static_cast<int>(SCREEN_WIDTH),
        static_cast<int>(SCREEN_HEIGHT),
        0
    );

    if (!window)
    {
        SDL_Log(
            "Window creation failed: %s",
            SDL_GetError()
        );

        SDL_Quit();
        return 1;
    }


    // -----------------------------------------------------
    // CREATE RENDERER
    // -----------------------------------------------------

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, nullptr);

    SDL_Texture* bikeTexture =
        IMG_LoadTexture(
            renderer,
            "assets/Bike/bike_chassis.png"
        );

    SDL_Texture* wheelTexture =
        IMG_LoadTexture(
            renderer,
            "assets/Bike/bike_wheel.png"
        );

    SDL_SetTextureScaleMode(
        bikeTexture,
        SDL_SCALEMODE_NEAREST
    );

    SDL_SetTextureScaleMode(
        wheelTexture,
        SDL_SCALEMODE_NEAREST
    );


    if (!bikeTexture)
    {
        SDL_Log(
            "Failed to load bike texture: %s",
            SDL_GetError()
        );
    }

    if (!wheelTexture)
    {
        SDL_Log(
            "Failed to load wheel texture: %s",
            SDL_GetError()
        );
    }

    if (!renderer)
    {
        SDL_Log(
            "Renderer creation failed: %s",
            SDL_GetError()
        );

        SDL_DestroyWindow(window);
        SDL_Quit();

        return 1;
    }



    TTF_Font* font =
        TTF_OpenFont(
            "C:/Windows/Fonts/arial.ttf",
            24.0f
        );

    if (!font)
    {
        SDL_Log(
            "Font loading failed: %s",
            SDL_GetError()
        );

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        TTF_Quit();
        SDL_Quit();

        return 1;
    }
    // =====================================================
    // BOX2D WORLD
    // =====================================================

    b2WorldDef worldDef =
        b2DefaultWorldDef();

    // Keep physics bodies awake while testing the bike controls.
    worldDef.enableSleep = false;

    // Box2D uses +Y upward.
    worldDef.gravity =
        b2Vec2{ 0.0f, -10.0f };

    b2WorldId worldId =
        b2CreateWorld(&worldDef);


    Terrain terrain =
        CreateTerrain(worldId);

    Bike bike =
        CreateBike(worldId);


    // =====================================================
    // PHYSICS SETTINGS
    // =====================================================

    const float physicsTimeStep =
        1.0f / 60.0f;

    const int subStepCount = 4;

    float physicsAccumulator = 0.0f;


    // =====================================================
    // GAME VARIABLES
    // =====================================================

    bool running = true;

    float cameraX = 0.0f;

    Uint64 previousTime =
        SDL_GetTicksNS();

    InputState input;

    bool bikeGrounded = true;

    bool levelComplete = false; 

    bool checkpointReached = false;

    float levelTime = 0.0f;

    // No best time exists yet so -1
    float bestTime = -1.0f;
    bool newBestTime = false;

    b2Vec2 respawnPosition =
        b2Vec2{ 0.0f, -7.0f };

    // =====================================================
    // MAIN GAME LOOP
    // =====================================================

    while (running)
    {
        // -------------------------------------------------
        // DELTA TIME
        // -------------------------------------------------

        const Uint64 currentTime =
            SDL_GetTicksNS();

        const float deltaTime =
            static_cast<float>(
                currentTime - previousTime
                )
            / 1000000000.0f;

        previousTime = currentTime;

        if (!levelComplete)
        {
            levelTime += deltaTime;
        }


        // -------------------------------------------------
        // INPUT
        // -------------------------------------------------

        ProcessInput(
            running,
            input
        );


        if (input.resetPressed)
        {

            if (levelComplete)
            {
                checkpointReached = false;

                respawnPosition =
                    b2Vec2{ 0.0f, -7.0f };

                levelTime = 0.0f;
            }

            ResetBike(
                bike,
                respawnPosition
            );

            cameraX = respawnPosition.x;

            bikeGrounded = true;

            levelComplete = false;
        }


        // -------------------------------------------------
        // FIXED PHYSICS UPDATE
        // -------------------------------------------------

        physicsAccumulator += deltaTime;

        while (physicsAccumulator >= physicsTimeStep)
        {
            UpdateBikeControls(
                bike,
                input,
                bikeGrounded,
                levelComplete
            );

            // ---------------------------------------------
            // RUN BOX2D
            // ---------------------------------------------

            b2World_Step(
                worldId,
                physicsTimeStep,
                subStepCount
            );

            bikeGrounded =
                IsBikeGrounded(bike);

            if (levelComplete)
            {
                SDL_SetWindowTitle(
                    window,
                    "TrailTorque - LEVEL COMPLETE!"
                );
            }

            else if (bikeGrounded)
            {
                SDL_SetWindowTitle(
                    window,
                    "TrailTorque - GROUND"
                );
            }
            else
            {
                SDL_SetWindowTitle(
                    window,
                    "TrailTorque - AIR"
                );
            
            }

            LimitBikeAngularSpeed(
                bike,
                bikeGrounded
            );


            physicsAccumulator -= physicsTimeStep;
        }


        // =================================================
        // READ CURRENT BOX2D POSITIONS
        // IMPORTANT: these must update EVERY FRAME.
        // =================================================

        b2Vec2 chassisPosition =
            b2Body_GetPosition(
                bike.chassisBodyId
            );

        if (!levelComplete &&
            chassisPosition.x >= FINISH_X)
        {
            levelComplete = true;

            newBestTime = false;

            if (bestTime < 0.0f ||
                levelTime < bestTime)
            {
                bestTime = levelTime;
                newBestTime = true;
            }
        }

        if (!checkpointReached &&
            chassisPosition.x >= CHECKPOINT_X)
        {
            checkpointReached = true;

            respawnPosition =
                b2Vec2{ CHECKPOINT_X, -7.0f };
        }

        // Camera gradually catches up to the bike.
        const float cameraFollowSpeed = 3.0f;

        cameraX +=
            (chassisPosition.x - cameraX) *
            cameraFollowSpeed *
            deltaTime;
     

        b2Vec2 rearWheelPosition =
            b2Body_GetPosition(
                bike.rearWheelBodyId
            );


        b2Vec2 frontWheelPosition =
            b2Body_GetPosition(
                bike.frontWheelBodyId
            );


        b2Vec2 groundPosition =
            b2Body_GetPosition(
                terrain.groundBodyId
            );


        // =================================================
        // REAR WHEEL -> SDL
        // =================================================

        SDL_FPoint rearWheelScreen;

        rearWheelScreen.x =
            CAMERA_TARGET_X +
            (rearWheelPosition.x - cameraX) *
            PIXELS_PER_METER;

        rearWheelScreen.y =
            SCREEN_CENTER_Y -
            rearWheelPosition.y *
            PIXELS_PER_METER;


        // =================================================
        // FRONT WHEEL -> SDL
        // =================================================

        SDL_FPoint frontWheelScreen;

        frontWheelScreen.x =
            CAMERA_TARGET_X +
            (frontWheelPosition.x - cameraX) *
            PIXELS_PER_METER;

        frontWheelScreen.y =
            SCREEN_CENTER_Y -
            frontWheelPosition.y *
            PIXELS_PER_METER;


        // =================================================
        // GROUND -> SDL
        // =================================================

        SDL_FRect groundRect;

        groundRect.w =
            GROUND_HALF_WIDTH * 2.0f *
            PIXELS_PER_METER;

        groundRect.h =
            GROUND_HEIGHT *
            PIXELS_PER_METER;


        groundRect.x =
            CAMERA_TARGET_X +
            (groundPosition.x - cameraX) *
            PIXELS_PER_METER -
            groundRect.w / 2.0f;


        groundRect.y =
            SCREEN_CENTER_Y -
            groundPosition.y *
            PIXELS_PER_METER -
            groundRect.h / 2.0f;


        // =================================================
        // RENDER
        // =================================================

        Render(
            renderer,
            font,
            bikeTexture,
            wheelTexture,
            bike.chassisBodyId,
            bike.rearWheelBodyId,
            bike.frontWheelBodyId,
            cameraX,
            groundRect,
            rearWheelScreen,
            frontWheelScreen,
            terrain.segments,
            levelComplete,
            checkpointReached,
            levelTime,
            bestTime,
            newBestTime
        );
    }


    // =====================================================
    // CLEANUP
    // =====================================================

    b2DestroyWorld(worldId);
    SDL_DestroyTexture(bikeTexture);
    SDL_DestroyTexture(wheelTexture);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);

    TTF_Quit();
    SDL_Quit();

    return 0;
}