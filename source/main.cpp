#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <box2d/box2d.h>
#include <vector>
#include "Bike.h"
#include "Input.h"
#include "Terrain.h"
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------

constexpr float SCREEN_WIDTH = 1280.0f;
constexpr float SCREEN_HEIGHT = 720.0f;

constexpr float PIXELS_PER_METER = 30.0f;

constexpr float SCREEN_CENTER_X = SCREEN_WIDTH / 2.0f;
constexpr float SCREEN_CENTER_Y = SCREEN_HEIGHT / 2.0f;

constexpr float CAMERA_TARGET_X = 400.0f;

constexpr float GROUND_HALF_WIDTH = 400.0f;
constexpr float GROUND_HEIGHT = 2.0f;

constexpr float FINISH_X = 315.0f;
constexpr float CHECKPOINT_X = 155.0f;




void ProcessInput(
    bool& running,
    InputState& input)
{

    SDL_Event event;

    input.jumpPressed = false;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            running = false;
        }
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.scancode == SDL_SCANCODE_SPACE &&
                !event.key.repeat)
            {
                input.jumpPressed = true;
            }
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
    TTF_Font* stuntFont,
    bool levelComplete,
    float levelTime,
    float airTime,
    const char* stuntText,
    float stuntTextTimer,
    int score,
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

    char airTimeText[64];

    SDL_snprintf(
        airTimeText,
        sizeof(airTimeText),
        "Air Time: %.2f",
        airTime
    );

    char scoreText[64];

    SDL_snprintf(
        scoreText,
        sizeof(scoreText),
        "Score: %d",
        score
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

        DrawText(
            renderer,
            font,
            scoreText,
            SCREEN_WIDTH - 180.0f,
            55.0f,
            white
        );

        if (airTime > 0.0f)
        {
            DrawText(
                renderer,
                font,
                airTimeText,
                SCREEN_WIDTH / 2.0f - 80.0f,
                40.0f,
                white
            );
        }
        
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

    if (stuntTextTimer > 0.0f)
    {
        DrawText(
            renderer,
            stuntFont,
            stuntText,
            SCREEN_WIDTH / 2.0f - 100.0f,
            80.0f,
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
    TTF_Font* stuntFont,
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
    float airTime,
    const char* stuntText,
    float stuntTextTimer,
    int score,
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

    SDL_FRect visualGround =
    {
        groundRect.x,
        groundRect.y,
        groundRect.w,
        SCREEN_HEIGHT - groundRect.y
    };

    SDL_RenderFillRect(
        renderer,
        &visualGround
    );

    // Light dirt near surface.
    SDL_FRect topDirt =
    {
        groundRect.x,
        groundRect.y,
        groundRect.w,
        25.0f
    };

    SDL_SetRenderDrawColor(
        renderer,
        145,
        92,
        45,
        255
    );

    SDL_RenderFillRect(
        renderer,
        &topDirt
    );


    // Medium dirt layer.
    SDL_FRect middleDirt =
    {
        groundRect.x,
        groundRect.y + 25.0f,
        groundRect.w,
        25.0f
    };

    SDL_SetRenderDrawColor(
        renderer,
        120,
        72,
        35,
        255
    );

    SDL_RenderFillRect(
        renderer,
        &middleDirt
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


                const float baseGroundY =
                    groundRect.y;


// =====================================================
// CONTINUOUS HILL DIRT
// =====================================================

// Deep earth.
// This goes from the hill surface all the way down,
// so the hill visually becomes part of the ground.
                SDL_SetRenderDrawColor(
                    renderer,
                    95,
                    55,
                    30,
                    255
                );

                SDL_RenderLine(
                    renderer,
                    static_cast<float>(x),
                    surfaceY,
                    static_cast<float>(x),
                    groundRect.y
                );


                float middleBottom =
                    std::min(
                        surfaceY + 40.0f,
                        groundRect.y
                    );

                SDL_SetRenderDrawColor(
                    renderer,
                    125,
                    75,
                    38,
                    255
                );

                SDL_RenderLine(
                    renderer,
                    static_cast<float>(x),
                    surfaceY,
                    static_cast<float>(x),
                    middleBottom
                );


                float topBottom =
                    std::min(
                        surfaceY + 15.0f,
                        groundRect.y
                    );

                SDL_SetRenderDrawColor(
                    renderer,
                    155,
                    100,
                    50,
                    255
                );

                SDL_RenderLine(
                    renderer,
                    static_cast<float>(x),
                    surfaceY,
                    static_cast<float>(x),
                    topBottom
                );
                
            }
        }


        // -------------------------------------------------
        // GRASS SURFACE
        // -------------------------------------------------

        // Dark grass shadow.
        SDL_SetRenderDrawColor(
            renderer,
            35,
            110,
            45,
            255
        );

        SDL_RenderLine(
            renderer,
            startScreen.x,
            startScreen.y + 4.0f,
            endScreen.x,
            endScreen.y + 4.0f
        );


        // Bright grass surface.
        SDL_SetRenderDrawColor(
            renderer,
            70,
            180,
            75,
            255
        );

        for (int thickness = -1;
            thickness <= 1;
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
// =====================================================
// NATURAL GRASS TUFTS
// =====================================================

        float terrainDX =
            endScreen.x - startScreen.x;

        float terrainDY =
            endScreen.y - startScreen.y;

        float terrainLength =
            std::sqrt(
                terrainDX * terrainDX +
                terrainDY * terrainDY
            );

        if (terrainLength > 0.0f)
        {
            // Direction pointing away from the terrain surface.
            float normalX =
                terrainDY / terrainLength;

            float normalY =
                -terrainDX / terrainLength;

            SDL_SetRenderDrawColor(
                renderer,
                45,
                135,
                50,
                255
            );

            // Add several small grass clumps.
            for (float t = 0.06f;
                t < 1.0f;
                t += 0.05f)
            {
                float grassX =
                    startScreen.x +
                    terrainDX * t;

                float grassY =
                    startScreen.y +
                    terrainDY * t;

                float grassHeight =
                    3.0f +
                    static_cast<int>(t * 1000.0f) % 5;

                // Middle blade.
                SDL_RenderLine(
                    renderer,
                    grassX,
                    grassY,
                    grassX + normalX * grassHeight,
                    grassY + normalY * grassHeight
                );

                // Left blade.
                SDL_RenderLine(
                    renderer,
                    grassX,
                    grassY,
                    grassX +
                    normalX * grassHeight -
                    2.0f,
                    grassY +
                    normalY * grassHeight
                );

                // Right blade.
                SDL_RenderLine(
                    renderer,
                    grassX,
                    grassY,
                    grassX +
                    normalX * grassHeight +
                    2.0f,
                    grassY +
                    normalY * grassHeight
                );
            }
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
// FLAT GROUND GRASS TUFTS
// =====================================================

    SDL_SetRenderDrawColor(
        renderer,
        45,
        135,
        50,
        255
    );

    int grassIndex = 0;

    for (float x = groundRect.x + 10.0f;
        x < groundRect.x + groundRect.w;
        x += 12.0f, ++grassIndex)
    {
        float grassY = groundRect.y;

        float grassHeight =
            3.0f +
            grassIndex % 5;

        SDL_RenderLine(
            renderer,
            x,
            grassY,
            x,
            grassY - grassHeight
        );

        SDL_RenderLine(
            renderer,
            x,
            grassY,
            x - 2.0f,
            grassY - grassHeight
        );

        SDL_RenderLine(
            renderer,
            x,
            grassY,
            x + 2.0f,
            grassY - grassHeight
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
// SIMPLE BIKE DEBUG VISUALS
// =====================================================

// Draw physics chassis.
    DrawRotatedChassis(
        renderer,
        chassisBodyId,
        cameraX
    );


    // Wheel size based directly on Box2D physics.
    const float wheelRadiusPixels =
        BIKE_WHEEL_RADIUS *
        PIXELS_PER_METER;


    // Rear wheel.
    SDL_SetRenderDrawColor(
        renderer,
        30,
        30,
        30,
        255
    );

    DrawFilledCircle(
        renderer,
        rearWheelScreen.x,
        rearWheelScreen.y,
        wheelRadiusPixels
    );


    // Front wheel.
    DrawFilledCircle(
        renderer,
        frontWheelScreen.x,
        frontWheelScreen.y,
        wheelRadiusPixels
    );

    // =====================================================
    // UI
    // =====================================================


    DrawUI(
        renderer,
        font,
        stuntFont,
        levelComplete,
        levelTime,
        airTime,
        stuntText,
        stuntTextTimer,
        score,
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
            "assets/Bike/rider_chassis.png"
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
            "assets/Fonts/Bangers-Regular.ttf",
            28.0f
        );

    TTF_Font* stuntFont =
        TTF_OpenFont(
            "assets/Fonts/Bangers-Regular.ttf",
            48.0f
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

    float airTime = 0.0f;
    bool wasBikeGrounded = true;

    float previousBikeAngle = 0.0f;
    float accumulatedRotation = 0.0f;

    bool positiveFlipCompleted = false;
    bool negativeFlipCompleted = false;
    int flipCount = 0;

    // No best time exists yet so -1
    float bestTime = -1.0f;
    bool newBestTime = false;

    char stuntText[64] = "";
    float stuntTextTimer = 0.0f;
    int score = 0;

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


        // ---------------------------------------------
        // STUNT POPUP TIMER
        // ---------------------------------------------

        if (stuntTextTimer > 0.0f)
        {
            stuntTextTimer -= deltaTime;

            if (stuntTextTimer < 0.0f)
            {
                stuntTextTimer = 0.0f;
            }
        }


        // ---------------------------------------------
        // LEVEL TIMER
        // ---------------------------------------------

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

            airTime = 0.0f;
            wasBikeGrounded = true;
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
            // BUNNY HOP
            // ---------------------------------------------

            const float jumpVelocityChange = 3.5f;

            if (input.jumpPressed &&
                bikeGrounded &&
                !levelComplete)
            {
                // Get each body's mass.
                const float chassisMass =
                    b2Body_GetMass(bike.chassisBodyId);

                const float rearWheelMass =
                    b2Body_GetMass(bike.rearWheelBodyId);

                const float frontWheelMass =
                    b2Body_GetMass(bike.frontWheelBodyId);


                // Give every part of the bike the same
                // upward velocity change.
                b2Body_ApplyLinearImpulseToCenter(
                    bike.chassisBodyId,
                    b2Vec2{
                        0.0f,
                        chassisMass * jumpVelocityChange
                    },
                    true
                );

                b2Body_ApplyLinearImpulseToCenter(
                    bike.rearWheelBodyId,
                    b2Vec2{
                        0.0f,
                        rearWheelMass * jumpVelocityChange
                    },
                    true
                );

                b2Body_ApplyLinearImpulseToCenter(
                    bike.frontWheelBodyId,
                    b2Vec2{
                        0.0f,
                        frontWheelMass * jumpVelocityChange
                    },
                    true
                );

            }

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

            bool justLeftGround =
                wasBikeGrounded &&
                !bikeGrounded;

            bool justLanded =
                !wasBikeGrounded &&
                bikeGrounded;
            if (justLeftGround)
            {
                SDL_Log("LEFT GROUND");
            }

            if (justLanded)
            {
                SDL_Log(
                    "LANDED | AirTime: %.2f | Rotation: %.2f",
                    airTime,
                    accumulatedRotation
                );
            }

            float currentBikeAngle =
                b2Rot_GetAngle(
                    b2Body_GetRotation(
                        bike.chassisBodyId
                    )
                );

            if (justLeftGround)
            {
                accumulatedRotation = 0.0f;
                previousBikeAngle = currentBikeAngle;

                positiveFlipCompleted = false;
                negativeFlipCompleted = false;

                flipCount = 0;
            }
            if (!bikeGrounded)
            {
                float angleDifference =
                    currentBikeAngle -
                    previousBikeAngle;

                constexpr float PI =
                    3.14159265f;

                // Fix angle wrap from +PI to -PI.
                if (angleDifference > PI)
                {
                    angleDifference -=
                        2.0f * PI;
                }

                if (angleDifference < -PI)
                {
                    angleDifference +=
                        2.0f * PI;
                }

                accumulatedRotation +=
                    angleDifference;

                constexpr float FLIP_THRESHOLD = 5.5f;

                if (accumulatedRotation >= FLIP_THRESHOLD)
                {
                    positiveFlipCompleted = true;
                }

                if (accumulatedRotation <= -FLIP_THRESHOLD)
                {
                    negativeFlipCompleted = true;
                }

                previousBikeAngle =
                    currentBikeAngle;
            }
            // ---------------------------------------------
            // AIR TIME
            // ---------------------------------------------

            if (!bikeGrounded)
            {
                airTime += physicsTimeStep;
            }

            if (!wasBikeGrounded && bikeGrounded)
            {
                // =========================
                // AIR TIME LANDING
                // =========================

                if (airTime >= 0.5f)
                {
                    SDL_Log(
                        "AIR TIME: %.2f seconds",
                        airTime
                    );
                }
                if (justLanded)
                {
                    SDL_Log(
                        "LANDED | AirTime: %.2f | Rotation: %.2f",
                        airTime,
                        accumulatedRotation
                    );

                    if (positiveFlipCompleted)
                    {
                        score += 500;

                        SDL_Log(
                            "FRONT FLIP DETECTED | SCORE: %d",
                            score
                        );

                        SDL_snprintf(
                            stuntText,
                            sizeof(stuntText),
                            "FRONT FLIP! +500"
                        );

                        stuntTextTimer = 1.2f;
                    }

                    if (negativeFlipCompleted)
                    {
                        score += 500;

                        SDL_Log(
                            "BACKFLIP DETECTED | SCORE: %d",
                            score
                        );

                        SDL_snprintf(
                            stuntText,
                            sizeof(stuntText),
                            "BACKFLIP! +500"
                        );

                        stuntTextTimer = 1.2f;
                    }

                    airTime = 0.0f;
                    accumulatedRotation = 0.0f;
                }

                // Reset jump data.
                airTime = 0.0f;
                accumulatedRotation = 0.0f;
            }

            wasBikeGrounded = bikeGrounded;

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
            stuntFont,
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
            airTime,
            stuntText,
            stuntTextTimer,
            score,
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