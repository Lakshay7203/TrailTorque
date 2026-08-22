#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <box2d/box2d.h>
#include <vector>

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

constexpr float CAMERA_TARGET_X = 400.0f;

constexpr float GROUND_HALF_WIDTH = 100.0f;
constexpr float GROUND_HEIGHT = 2.0f;

// ---------------------------------------------------------
// INPUT
// ---------------------------------------------------------

struct InputState
{
    bool driveForward = false;
    bool driveBackward = false;

    bool leanBackward = false;
    bool leanForward = false;

    bool resetPressed = false;
};


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
        { -CHASSIS_WIDTH / 2.0f, -CHASSIS_HEIGHT / 2.0f },
        {  CHASSIS_WIDTH / 2.0f, -CHASSIS_HEIGHT / 2.0f },
        {  CHASSIS_WIDTH / 2.0f,  CHASSIS_HEIGHT / 2.0f },
        { -CHASSIS_WIDTH / 2.0f,  CHASSIS_HEIGHT / 2.0f }
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

// ---------------------------------------------------------
// RENDER
// ---------------------------------------------------------

void Render(
    SDL_Renderer* renderer,
    b2BodyId chassisBodyId,
    float cameraX,
    const SDL_FRect& groundRect,
    const SDL_FPoint& rearWheelScreen,
    const SDL_FPoint& frontWheelScreen,
    const SDL_FPoint& hillStart,
    const SDL_FPoint& hillPeak,
    const SDL_FPoint& hillEnd,
    const SDL_FPoint& jumpRampStart,
    const SDL_FPoint& jumpRampEnd)
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

    // Draw the hill in black.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    for (int thickness = -3; thickness <= 3; ++thickness)
    {
        SDL_RenderLine(
            renderer,
            hillStart.x,
            hillStart.y + thickness,
            hillPeak.x,
            hillPeak.y + thickness
        );

        SDL_RenderLine(
            renderer,
            hillPeak.x,
            hillPeak.y + thickness,
            hillEnd.x,
            hillEnd.y + thickness
        );
    }

    // Draw jump ramp.
    for (int thickness = -3; thickness <= 3; ++thickness)
    {
        SDL_RenderLine(
            renderer,
            jumpRampStart.x,
            jumpRampStart.y + thickness,
            jumpRampEnd.x,
            jumpRampEnd.y + thickness
        );
    }

    // -----------------------------------------------------
    // CHASSIS
    // -----------------------------------------------------

    // Green bike chassis.
    SDL_SetRenderDrawColor(renderer, 0, 180, 0, 255);

    DrawRotatedChassis(
        renderer,
        chassisBodyId,
        cameraX
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

    b2Polygon groundBox =
        b2MakeBox(
            GROUND_HALF_WIDTH,
            GROUND_HEIGHT / 2.0f
        );

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
// FIRST HILL
// =====================================================

// Static body that holds our hill segments.
    b2BodyDef hillBodyDef = b2DefaultBodyDef();

    b2BodyId hillBodyId =
        b2CreateBody(worldId, &hillBodyDef);


    // Collision settings for the hill.
    b2ShapeDef hillShapeDef = b2DefaultShapeDef();

    hillShapeDef.material.friction = 1.0f;


    // Uphill section.
    b2Segment hillUp;

    hillUp.point1 = b2Vec2{ 8.0f, -9.0f };
    hillUp.point2 = b2Vec2{ 12.0f, -7.5f };

    b2CreateSegmentShape(
        hillBodyId,
        &hillShapeDef,
        &hillUp
    );


    // Downhill section.
    b2Segment hillDown;

    hillDown.point1 = b2Vec2{ 12.0f, -7.5f };
    hillDown.point2 = b2Vec2{ 16.0f, -9.0f };

    b2CreateSegmentShape(
        hillBodyId,
        &hillShapeDef,
        &hillDown
    );

    // =====================================================
// FIRST JUMP RAMP
// =====================================================

    b2Segment jumpRamp;

    jumpRamp.point1 = b2Vec2{ 20.0f, -9.0f };
    jumpRamp.point2 = b2Vec2{ 24.0f, -6.5f };

    b2CreateSegmentShape(
        hillBodyId,
        &hillShapeDef,
        &jumpRamp
    );

    // =====================================================
    // BIKE CHASSIS
    // =====================================================

    b2BodyDef chassisBodyDef =
        b2DefaultBodyDef();

    chassisBodyDef.type =
        b2_dynamicBody;

    chassisBodyDef.angularDamping = 0.5f;

    chassisBodyDef.position =
        b2Vec2{ 0.0f, -7.0f };


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


    b2ShapeId chassisShapeId =
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

    float cameraX = 0.0f;

    Uint64 previousTime =
        SDL_GetTicksNS();

    InputState input;

    bool bikeGrounded = true;



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
            input
        );

        if (input.resetPressed)
        {
            // Reset chassis.
            b2Body_SetTransform(
                chassisBodyId,
                b2Vec2{ 0.0f, -7.0f },
                b2MakeRot(0.0f)
            );

            // Reset rear wheel.
            b2Body_SetTransform(
                rearWheelBodyId,
                b2Vec2{ -0.8f, -7.65f },
                b2MakeRot(0.0f)
            );

            // Reset front wheel.
            b2Body_SetTransform(
                frontWheelBodyId,
                b2Vec2{ 0.8f, -7.65f },
                b2MakeRot(0.0f)
            );


            // Remove all movement.
            b2Body_SetLinearVelocity(
                chassisBodyId,
                b2Vec2{ 0.0f, 0.0f }
            );

            b2Body_SetLinearVelocity(
                rearWheelBodyId,
                b2Vec2{ 0.0f, 0.0f }
            );

            b2Body_SetLinearVelocity(
                frontWheelBodyId,
                b2Vec2{ 0.0f, 0.0f }
            );


            // Remove all spinning.
            b2Body_SetAngularVelocity(
                chassisBodyId,
                0.0f
            );

            b2Body_SetAngularVelocity(
                rearWheelBodyId,
                0.0f
            );

            b2Body_SetAngularVelocity(
                frontWheelBodyId,
                0.0f
            );


            // Put camera back at the starting area.
            cameraX = 0.0f;

            bikeGrounded = true;
        }


        // -------------------------------------------------
        // FIXED PHYSICS UPDATE
        // -------------------------------------------------

        physicsAccumulator += deltaTime;

        while (physicsAccumulator >= physicsTimeStep)
        {
            // ---------------------------------------------
            // DRIVE
            // ---------------------------------------------

            if (input.driveForward)
            {
                b2WheelJoint_SetMotorSpeed(
                    rearWheelJointId,
                    -20.0f
                );
            }
            else if (input.driveBackward)
            {
                b2WheelJoint_SetMotorSpeed(
                    rearWheelJointId,
                    20.0f
                );
            }
            else
            {
                b2WheelJoint_SetMotorSpeed(
                    rearWheelJointId,
                    0.0f
                );
            }


            // ---------------------------------------------
            // CURRENT ROTATION
            // ---------------------------------------------

            float angularVelocity =
                b2Body_GetAngularVelocity(chassisBodyId);

            b2Rot chassisRotation =
                b2Body_GetRotation(chassisBodyId);

            float chassisAngle =
                b2Rot_GetAngle(chassisRotation);


            // ---------------------------------------------
         // GROUND / AIR BIKE CONTROL
         // ---------------------------------------------

            if (bikeGrounded)
            {
                // Controlled ground leaning.
                float targetAngle = 0.0f;

                if (input.leanBackward && !input.leanForward)
                {
                    targetAngle = 0.35f;
                }
                else if (input.leanForward && !input.leanBackward)
                {
                    targetAngle = -0.35f;
                }

                const float leanStrength = 8.0f;
                const float leanDamping = 2.0f;

                float angleError =
                    targetAngle - chassisAngle;

                float correctionTorque =
                    (angleError * leanStrength)
                    -
                    (angularVelocity * leanDamping);

                b2Body_ApplyTorque(
                    chassisBodyId,
                    correctionTorque,
                    true
                );
            }
            else
            {
                // Free rotation while airborne.
                const float airTorque = 20.0f;

                if (input.leanBackward && !input.leanForward)
                {
                    b2Body_ApplyTorque(
                        chassisBodyId,
                        airTorque,
                        true
                    );
                }
                else if (input.leanForward && !input.leanBackward)
                {
                    b2Body_ApplyTorque(
                        chassisBodyId,
                        -airTorque,
                        true
                    );
                }
            }


            // ---------------------------------------------
            // RUN BOX2D
            // ---------------------------------------------

            b2World_Step(
                worldId,
                physicsTimeStep,
                subStepCount
            );

            b2ContactData rearContacts[4];
            b2ContactData frontContacts[4];

            int rearContactCount =
                b2Shape_GetContactData(
                    rearWheelShapeId,
                    rearContacts,
                    4
                );

            int frontContactCount =
                b2Shape_GetContactData(
                    frontWheelShapeId,
                    frontContacts,
                    4
                );

            bool rearWheelGrounded =
                rearContactCount > 0;

            bool frontWheelGrounded =
                frontContactCount > 0;

            bikeGrounded =
                rearWheelGrounded ||
                frontWheelGrounded;

            if (bikeGrounded)
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

            // ---------------------------------------------
            // LIMIT ROTATION SPEED
            // ---------------------------------------------

            angularVelocity =
                b2Body_GetAngularVelocity(chassisBodyId);

            const float maxAngularSpeed =
                bikeGrounded ? 1.5f : 8.0f;

            if (angularVelocity > maxAngularSpeed)
            {
                b2Body_SetAngularVelocity(
                    chassisBodyId,
                    maxAngularSpeed
                );
            }

            if (angularVelocity < -maxAngularSpeed)
            {
                b2Body_SetAngularVelocity(
                    chassisBodyId,
                    -maxAngularSpeed
                );
            }


            physicsAccumulator -= physicsTimeStep;
        }


        // =================================================
        // READ CURRENT BOX2D POSITIONS
        // IMPORTANT: these must update EVERY FRAME.
        // =================================================

        b2Vec2 chassisPosition =
            b2Body_GetPosition(
                chassisBodyId
            );

        // Camera gradually catches up to the bike.
        const float cameraFollowSpeed = 3.0f;

        cameraX +=
            (chassisPosition.x - cameraX) *
            cameraFollowSpeed *
            deltaTime;
        SDL_FPoint hillStart;
        SDL_FPoint hillPeak;
        SDL_FPoint hillEnd;

        hillStart.x =
            CAMERA_TARGET_X +
            (8.0f - cameraX) * PIXELS_PER_METER;

        hillStart.y =
            SCREEN_CENTER_Y -
            (-9.0f * PIXELS_PER_METER);


        hillPeak.x =
            CAMERA_TARGET_X +
            (12.0f - cameraX) * PIXELS_PER_METER;

        hillPeak.y =
            SCREEN_CENTER_Y -
            (-7.5f * PIXELS_PER_METER);


        hillEnd.x =
            CAMERA_TARGET_X +
            (16.0f - cameraX) * PIXELS_PER_METER;

        hillEnd.y =
            SCREEN_CENTER_Y -
            (-9.0f * PIXELS_PER_METER);

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

        SDL_FPoint jumpRampStart;
        SDL_FPoint jumpRampEnd;


        jumpRampStart.x =
            CAMERA_TARGET_X +
            (20.0f - cameraX) * PIXELS_PER_METER;

        jumpRampStart.y =
            SCREEN_CENTER_Y -
            (-9.0f * PIXELS_PER_METER);


        jumpRampEnd.x =
            CAMERA_TARGET_X +
            (24.0f - cameraX) * PIXELS_PER_METER;

        jumpRampEnd.y =
            SCREEN_CENTER_Y -
            (-6.5f * PIXELS_PER_METER);


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
            CAMERA_TARGET_X +
            (chassisPosition.x - cameraX) *
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
            chassisBodyId,
            cameraX,
            groundRect,
            rearWheelScreen,
            frontWheelScreen,
            hillStart,
            hillPeak,
            hillEnd,
            jumpRampStart,
            jumpRampEnd
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