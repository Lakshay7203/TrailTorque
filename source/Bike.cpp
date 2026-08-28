#include "Bike.h"

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
