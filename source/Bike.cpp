#include "Bike.h"

Bike CreateBike(b2WorldId worldId)
{
    Bike bike;

    // =====================================================
    // CHASSIS
    // =====================================================

    b2BodyDef chassisBodyDef =
        b2DefaultBodyDef();

    chassisBodyDef.type =
        b2_dynamicBody;

    chassisBodyDef.angularDamping =
        0.5f;

    chassisBodyDef.position =
        b2Vec2{ 0.0f, -7.0f };

    bike.chassisBodyId =
        b2CreateBody(
            worldId,
            &chassisBodyDef
        );

    b2ShapeDef chassisShapeDef =
        b2DefaultShapeDef();

    chassisShapeDef.density =
        1.0f;

    b2Polygon chassisShape =
        b2MakeBox(
            BIKE_CHASSIS_WIDTH / 2.0f,
            BIKE_CHASSIS_HEIGHT / 2.0f
        );

    b2CreatePolygonShape(
        bike.chassisBodyId,
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

    rearWheelBodyDef.position =
        b2Vec2{ -0.8f, -7.65f };

    bike.rearWheelBodyId =
        b2CreateBody(
            worldId,
            &rearWheelBodyDef
        );

    b2ShapeDef rearWheelShapeDef =
        b2DefaultShapeDef();

    rearWheelShapeDef.material.friction =
        0.9f;

    rearWheelShapeDef.density =
        1.0f;

    b2Circle rearWheelCircle;

    rearWheelCircle.center =
        b2Vec2{ 0.0f, 0.0f };

    rearWheelCircle.radius =
        BIKE_WHEEL_RADIUS;

    bike.rearWheelShapeId =
        b2CreateCircleShape(
            bike.rearWheelBodyId,
            &rearWheelShapeDef,
            &rearWheelCircle
        );

    b2Shape_SetFriction(
        bike.rearWheelShapeId,
        1.0f
    );


    // =====================================================
    // FRONT WHEEL
    // =====================================================

    b2BodyDef frontWheelBodyDef =
        b2DefaultBodyDef();

    frontWheelBodyDef.type =
        b2_dynamicBody;

    frontWheelBodyDef.position =
        b2Vec2{ 0.8f, -7.65f };

    bike.frontWheelBodyId =
        b2CreateBody(
            worldId,
            &frontWheelBodyDef
        );

    b2ShapeDef frontWheelShapeDef =
        b2DefaultShapeDef();

    frontWheelShapeDef.material.friction =
        0.9f;

    frontWheelShapeDef.density =
        1.0f;

    b2Circle frontWheelCircle;

    frontWheelCircle.center =
        b2Vec2{ 0.0f, 0.0f };

    frontWheelCircle.radius =
        BIKE_WHEEL_RADIUS;

    bike.frontWheelShapeId =
        b2CreateCircleShape(
            bike.frontWheelBodyId,
            &frontWheelShapeDef,
            &frontWheelCircle
        );

    b2Shape_SetFriction(
        bike.frontWheelShapeId,
        1.0f
    );


    // =====================================================
    // REAR WHEEL JOINT
    // =====================================================

    b2WheelJointDef rearWheelJointDef =
        b2DefaultWheelJointDef();

    rearWheelJointDef.bodyIdA =
        bike.chassisBodyId;

    rearWheelJointDef.bodyIdB =
        bike.rearWheelBodyId;

    rearWheelJointDef.localAnchorA =
        b2Vec2{ -0.8f, -0.65f };

    rearWheelJointDef.localAnchorB =
        b2Vec2{ 0.0f, 0.0f };

    rearWheelJointDef.localAxisA =
        b2Vec2{ 0.0f, 1.0f };

    rearWheelJointDef.enableSpring =
        true;

    rearWheelJointDef.hertz =
        4.0f;

    rearWheelJointDef.dampingRatio =
        0.7f;

    rearWheelJointDef.enableLimit =
        true;

    rearWheelJointDef.lowerTranslation =
        -0.15f;

    rearWheelJointDef.upperTranslation =
        0.15f;

    rearWheelJointDef.enableMotor =
        true;

    rearWheelJointDef.motorSpeed =
        0.0f;

    rearWheelJointDef.maxMotorTorque =
        100.0f;

    rearWheelJointDef.collideConnected =
        false;

    bike.rearWheelJointId =
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
        bike.chassisBodyId;

    frontWheelJointDef.bodyIdB =
        bike.frontWheelBodyId;

    frontWheelJointDef.localAnchorA =
        b2Vec2{ 0.8f, -0.65f };

    frontWheelJointDef.localAnchorB =
        b2Vec2{ 0.0f, 0.0f };

    frontWheelJointDef.localAxisA =
        b2Vec2{ 0.0f, 1.0f };

    frontWheelJointDef.enableSpring =
        true;

    frontWheelJointDef.hertz =
        4.0f;

    frontWheelJointDef.dampingRatio =
        0.7f;

    frontWheelJointDef.enableLimit =
        true;

    frontWheelJointDef.lowerTranslation =
        -0.15f;

    frontWheelJointDef.upperTranslation =
        0.15f;

    frontWheelJointDef.collideConnected =
        false;

    bike.frontWheelJointId =
        b2CreateWheelJoint(
            worldId,
            &frontWheelJointDef
        );

    return bike;
}

void ResetBike(
    Bike& bike,
    b2Vec2 respawnPosition)
{
    // Chassis
    b2Body_SetTransform(
        bike.chassisBodyId,
        respawnPosition,
        b2MakeRot(0.0f)
    );

    // Rear wheel
    b2Body_SetTransform(
        bike.rearWheelBodyId,
        b2Vec2{
            respawnPosition.x - 0.8f,
            respawnPosition.y - 0.65f
        },
        b2MakeRot(0.0f)
    );

    // Front wheel
    b2Body_SetTransform(
        bike.frontWheelBodyId,
        b2Vec2{
            respawnPosition.x + 0.8f,
            respawnPosition.y - 0.65f
        },
        b2MakeRot(0.0f)
    );

    // Stop movement
    b2Body_SetLinearVelocity(
        bike.chassisBodyId,
        b2Vec2{ 0.0f, 0.0f }
    );

    b2Body_SetLinearVelocity(
        bike.rearWheelBodyId,
        b2Vec2{ 0.0f, 0.0f }
    );

    b2Body_SetLinearVelocity(
        bike.frontWheelBodyId,
        b2Vec2{ 0.0f, 0.0f }
    );

    // Stop rotation
    b2Body_SetAngularVelocity(
        bike.chassisBodyId,
        0.0f
    );

    b2Body_SetAngularVelocity(
        bike.rearWheelBodyId,
        0.0f
    );

    b2Body_SetAngularVelocity(
        bike.frontWheelBodyId,
        0.0f
    );
}

bool IsBikeGrounded(const Bike& bike)
{
    b2ContactData rearContacts[4];
    b2ContactData frontContacts[4];

    int rearContactCount =
        b2Shape_GetContactData(
            bike.rearWheelShapeId,
            rearContacts,
            4
        );

    int frontContactCount =
        b2Shape_GetContactData(
            bike.frontWheelShapeId,
            frontContacts,
            4
        );

    bool rearWheelGrounded =
        rearContactCount > 0;

    bool frontWheelGrounded =
        frontContactCount > 0;

    return rearWheelGrounded ||
        frontWheelGrounded;
}

void UpdateBikeControls(
    const Bike& bike,
    const InputState& input,
    bool bikeGrounded,
    bool levelComplete)
{
    // ---------------------------------------------
    // DRIVE
    // ---------------------------------------------

    if (!levelComplete && input.driveForward)
    {
        b2WheelJoint_SetMotorSpeed(
            bike.rearWheelJointId,
            -20.0f
        );
    }
    else if (!levelComplete && input.driveBackward)
    {
        b2WheelJoint_SetMotorSpeed(
            bike.rearWheelJointId,
            20.0f
        );
    }
    else
    {
        b2WheelJoint_SetMotorSpeed(
            bike.rearWheelJointId,
            0.0f
        );
    }

    // ---------------------------------------------
    // CURRENT ROTATION
    // ---------------------------------------------

    float angularVelocity =
        b2Body_GetAngularVelocity(
            bike.chassisBodyId
        );

    b2Rot chassisRotation =
        b2Body_GetRotation(
            bike.chassisBodyId
        );

    float chassisAngle =
        b2Rot_GetAngle(
            chassisRotation
        );

    // ---------------------------------------------
    // GROUND / AIR CONTROL
    // ---------------------------------------------

    if (bikeGrounded)
    {
        float targetAngle = 0.0f;

        if (input.leanBackward &&
            !input.leanForward)
        {
            targetAngle = 0.35f;
        }
        else if (input.leanForward &&
            !input.leanBackward)
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
            bike.chassisBodyId,
            correctionTorque,
            true
        );
    }
    else
    {
        const float airTorque = 20.0f;

        if (input.leanBackward &&
            !input.leanForward)
        {
            b2Body_ApplyTorque(
                bike.chassisBodyId,
                airTorque,
                true
            );
        }
        else if (input.leanForward &&
            !input.leanBackward)
        {
            b2Body_ApplyTorque(
                bike.chassisBodyId,
                -airTorque,
                true
            );
        }
    }
}

void LimitBikeAngularSpeed(
    const Bike& bike,
    bool bikeGrounded)
{
    float angularVelocity =
        b2Body_GetAngularVelocity(
            bike.chassisBodyId
        );

    const float maxAngularSpeed =
        bikeGrounded ? 1.5f : 8.0f;

    if (angularVelocity > maxAngularSpeed)
    {
        b2Body_SetAngularVelocity(
            bike.chassisBodyId,
            maxAngularSpeed
        );
    }

    if (angularVelocity < -maxAngularSpeed)
    {
        b2Body_SetAngularVelocity(
            bike.chassisBodyId,
            -maxAngularSpeed
        );
    }
}
