#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <box2d/box2d.h>

// ---------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------

constexpr float SCREEN_WIDTH = 1280.0f;
constexpr float SCREEN_HEIGHT = 720.0f;

constexpr float PIXELS_PER_METER = 30.0f;

constexpr float SCREEN_CENTER_X = SCREEN_WIDTH / 2.0f;
constexpr float SCREEN_CENTER_Y = SCREEN_HEIGHT / 2.0f;

constexpr float CHASSIS_WIDTH = 2.4f;
constexpr float CHASSIS_HEIGHT = 0.5f;

constexpr float WHEEL_RADIUS = 0.4f;


// ---------------------------------------------------------
// INPUT
// ---------------------------------------------------------

void ProcessInput(
    bool& running,
    b2JointId rearWheelJointId)
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


    // W = drive forward.
    if (keyboardState[SDL_SCANCODE_W])
    {
        b2WheelJoint_SetMotorSpeed(
            rearWheelJointId,
            -20.0f
        );
    }

    // S = drive backward.
    else if (keyboardState[SDL_SCANCODE_S])
    {
        b2WheelJoint_SetMotorSpeed(
            rearWheelJointId,
            20.0f
        );
    }

    // No key = stop motor.
    else
    {
        b2WheelJoint_SetMotorSpeed(
            rearWheelJointId,
            0.0f
        );
    }
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


// ---------------------------------------------------------
// RENDER
// ---------------------------------------------------------

void Render(
    SDL_Renderer* renderer,
    const SDL_FRect& chassisRect,
    const SDL_FRect& groundRect,
    const SDL_FPoint& rearWheelScreen,
    const SDL_FPoint& frontWheelScreen)
{
    // Sky-blue background.
    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
    SDL_RenderClear(renderer);


    // -----------------------------------------------------
    // GROUND
    // -----------------------------------------------------

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_RenderFillRect(
        renderer,
        &groundRect
    );


    // -----------------------------------------------------
    // CHASSIS
    // -----------------------------------------------------

    // Green bike chassis.
    SDL_SetRenderDrawColor(renderer, 0, 180, 0, 255);

    SDL_RenderFillRect(
        renderer,
        &chassisRect
    );


    // -----------------------------------------------------
    // WHEELS
    // -----------------------------------------------------

    // Black wheels.
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);

    const float wheelRadiusPixels =
        WHEEL_RADIUS * PIXELS_PER_METER;

    DrawFilledCircle(
        renderer,
        rearWheelScreen.x,
        rearWheelScreen.y,
        wheelRadiusPixels
    );

    DrawFilledCircle(
        renderer,
        frontWheelScreen.x,
        frontWheelScreen.y,
        wheelRadiusPixels
    );


    // Show completed frame.
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


    // =====================================================
    // GROUND
    // =====================================================

    b2BodyDef groundBodyDef =
        b2DefaultBodyDef();

    groundBodyDef.position =
        b2Vec2{ 0.0f, -10.0f };

    b2BodyId groundBodyId =
        b2CreateBody(
            worldId,
            &groundBodyDef
        );


    b2ShapeDef groundShapeDef =
        b2DefaultShapeDef();

    groundShapeDef.material.friction = 0.9f;

    // b2MakeBox uses HALF width and HALF height.
    // Full ground size = 40m x 2m.
    b2Polygon groundBox =
        b2MakeBox(20.0f, 1.0f);

    b2ShapeId groundShapeId =
        b2CreatePolygonShape(
            groundBodyId,
            &groundShapeDef,
            &groundBox
        );

    b2Shape_SetFriction(
        groundShapeId,
        1.0f
    );


    // =====================================================
    // BIKE CHASSIS
    // =====================================================

    b2BodyDef chassisBodyDef =
        b2DefaultBodyDef();

    chassisBodyDef.type =
        b2_dynamicBody;

    chassisBodyDef.position =
        b2Vec2{ 0.0f, -7.0f };

    // Temporary while building the bike.
    // Later we remove this so the bike can lean/rotate.
    chassisBodyDef.fixedRotation = true;


    b2BodyId chassisBodyId =
        b2CreateBody(
            worldId,
            &chassisBodyDef
        );


    b2ShapeDef chassisShapeDef =
        b2DefaultShapeDef();

    chassisShapeDef.density = 1.0f;


    // Full chassis = 2.4m x 0.5m.
    b2Polygon chassisShape =
        b2MakeBox(
            CHASSIS_WIDTH / 2.0f,
            CHASSIS_HEIGHT / 2.0f
        );


    b2CreatePolygonShape(
        chassisBodyId,
        &chassisShapeDef,
        &chassisShape
    );


    // =====================================================
    // REAR WHEEL
    // =====================================================

    b2BodyDef rearWheelBodyDef =
        b2DefaultBodyDef();

    rearWheelBodyDef.type =
        b2_dynamicBody;

    // Left and below chassis.
    rearWheelBodyDef.position =
        b2Vec2{ -0.8f, -7.65f };


    b2BodyId rearWheelBodyId =
        b2CreateBody(
            worldId,
            &rearWheelBodyDef
        );


    b2ShapeDef rearWheelShapeDef =
        b2DefaultShapeDef();

    rearWheelShapeDef.material.friction = 0.9f;

    rearWheelShapeDef.density = 1.0f;


    b2Circle rearWheelCircle;

    rearWheelCircle.center =
        b2Vec2{ 0.0f, 0.0f };

    rearWheelCircle.radius =
        WHEEL_RADIUS;


    b2ShapeId rearWheelShapeId =
        b2CreateCircleShape(
            rearWheelBodyId,
            &rearWheelShapeDef,
            &rearWheelCircle
        );

    b2Shape_SetFriction(
        rearWheelShapeId,
        1.0f
    );


    // =====================================================
    // FRONT WHEEL
    // =====================================================

    b2BodyDef frontWheelBodyDef =
        b2DefaultBodyDef();

    frontWheelBodyDef.type =
        b2_dynamicBody;

    // Right and below chassis.
    frontWheelBodyDef.position =
        b2Vec2{ 0.8f, -7.65f };


    b2BodyId frontWheelBodyId =
        b2CreateBody(
            worldId,
            &frontWheelBodyDef
        );


    b2ShapeDef frontWheelShapeDef =
        b2DefaultShapeDef();

    frontWheelShapeDef.material.friction = 0.9f;

    frontWheelShapeDef.density = 1.0f;


    b2Circle frontWheelCircle;

    frontWheelCircle.center =
        b2Vec2{ 0.0f, 0.0f };

    frontWheelCircle.radius =
        WHEEL_RADIUS;

    rearWheelShapeDef.material.friction = 0.9f;
    frontWheelShapeDef.material.friction = 0.9f;

    b2ShapeId frontWheelShapeId =
        b2CreateCircleShape(
            frontWheelBodyId,
            &frontWheelShapeDef,
            &frontWheelCircle
        );

    b2Shape_SetFriction(
        frontWheelShapeId,
        1.0f
    );


    // =====================================================
 // REAR WHEEL JOINT
 // =====================================================

    b2WheelJointDef rearWheelJointDef =
        b2DefaultWheelJointDef();

    rearWheelJointDef.bodyIdA =
        chassisBodyId;

    rearWheelJointDef.bodyIdB =
        rearWheelBodyId;

    rearWheelJointDef.localAnchorA =
        b2Vec2{ -0.8f, -0.65f };

    rearWheelJointDef.localAnchorB =
        b2Vec2{ 0.0f, 0.0f };

    rearWheelJointDef.localAxisA =
        b2Vec2{ 0.0f, 1.0f };


    // Suspension
    rearWheelJointDef.enableSpring = true;
    rearWheelJointDef.hertz = 4.0f;
    rearWheelJointDef.dampingRatio = 0.7f;


    // Suspension limits
    rearWheelJointDef.enableLimit = true;
    rearWheelJointDef.lowerTranslation = -0.15f;
    rearWheelJointDef.upperTranslation = 0.15f;


    // Motor
    rearWheelJointDef.enableMotor = true;

    // Starts stopped. W changes this later.
    rearWheelJointDef.motorSpeed = 0.0f;

    // Give it plenty of torque while testing.
    rearWheelJointDef.maxMotorTorque = 100.0f;


    // Chassis and wheel should not collide.
    rearWheelJointDef.collideConnected = false;


    b2JointId rearWheelJointId =
        b2CreateWheelJoint(
            worldId,
            &rearWheelJointDef
        );

    // =====================================================
    // FRONT WHEEL JOINT
    // =====================================================

    b2WheelJointDef frontWheelJointDef =
        b2DefaultWheelJointDef();


    frontWheelJointDef.bodyIdA =
        chassisBodyId;

    frontWheelJointDef.bodyIdB =
        frontWheelBodyId;


    frontWheelJointDef.localAnchorA =
        b2Vec2{ 0.8f, -0.65f };


    frontWheelJointDef.localAnchorB =
        b2Vec2{ 0.0f, 0.0f };


    frontWheelJointDef.localAxisA =
        b2Vec2{ 0.0f, 1.0f };


    frontWheelJointDef.enableSpring = true;

    frontWheelJointDef.hertz = 4.0f;

    frontWheelJointDef.dampingRatio = 0.7f;


    frontWheelJointDef.enableLimit = true;

    frontWheelJointDef.lowerTranslation = -0.15f;

    frontWheelJointDef.upperTranslation = 0.15f;


    frontWheelJointDef.collideConnected = false;


    b2JointId frontWheelJointId =
        b2CreateWheelJoint(
            worldId,
            &frontWheelJointDef
        );


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

    Uint64 previousTime =
        SDL_GetTicksNS();


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


        // -------------------------------------------------
        // INPUT
        // -------------------------------------------------

        ProcessInput(
            running,
            rearWheelJointId
        );

        const bool* keyboardState =
            SDL_GetKeyboardState(nullptr);


        // -------------------------------------------------
        // FIXED PHYSICS UPDATE
        // -------------------------------------------------

        physicsAccumulator += deltaTime;

        while (physicsAccumulator >= physicsTimeStep)
        {
            b2World_Step(
                worldId,
                physicsTimeStep,
                subStepCount
            );

            physicsAccumulator -=
                physicsTimeStep;
        }


        // =================================================
        // READ CURRENT BOX2D POSITIONS
        // IMPORTANT: these must update EVERY FRAME.
        // =================================================

        b2Vec2 chassisPosition =
            b2Body_GetPosition(
                chassisBodyId
            );


        b2Vec2 rearWheelPosition =
            b2Body_GetPosition(
                rearWheelBodyId
            );


        b2Vec2 frontWheelPosition =
            b2Body_GetPosition(
                frontWheelBodyId
            );


        b2Vec2 groundPosition =
            b2Body_GetPosition(
                groundBodyId
            );


        // =================================================
        // CHASSIS -> SDL
        // =================================================

        SDL_FRect chassisRect;

        chassisRect.w =
            CHASSIS_WIDTH *
            PIXELS_PER_METER;

        chassisRect.h =
            CHASSIS_HEIGHT *
            PIXELS_PER_METER;


        chassisRect.x =
            SCREEN_CENTER_X +
            chassisPosition.x *
            PIXELS_PER_METER -
            chassisRect.w / 2.0f;


        chassisRect.y =
            SCREEN_CENTER_Y -
            chassisPosition.y *
            PIXELS_PER_METER -
            chassisRect.h / 2.0f;


        // =================================================
        // REAR WHEEL -> SDL
        // =================================================

        SDL_FPoint rearWheelScreen;

        rearWheelScreen.x =
            SCREEN_CENTER_X +
            rearWheelPosition.x *
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
            SCREEN_CENTER_X +
            frontWheelPosition.x *
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
            40.0f * PIXELS_PER_METER;

        groundRect.h =
            2.0f * PIXELS_PER_METER;


        groundRect.x =
            SCREEN_CENTER_X +
            groundPosition.x *
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
            chassisRect,
            groundRect,
            rearWheelScreen,
            frontWheelScreen
        );
    }


    // =====================================================
    // CLEANUP
    // =====================================================

    b2DestroyWorld(worldId);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}