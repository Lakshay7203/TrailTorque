#pragma once

#include <box2d/box2d.h>
#include "Input.h"

inline constexpr float BIKE_CHASSIS_WIDTH = 2.4f;
inline constexpr float BIKE_CHASSIS_HEIGHT = 0.5f;
inline constexpr float BIKE_WHEEL_RADIUS = 0.4f;

struct Bike
{
    b2BodyId chassisBodyId;

    b2BodyId rearWheelBodyId;
    b2BodyId frontWheelBodyId;

    b2ShapeId rearWheelShapeId;
    b2ShapeId frontWheelShapeId;

    b2JointId rearWheelJointId;
    b2JointId frontWheelJointId;
};

Bike CreateBike(b2WorldId worldId);

void ResetBike(
    Bike& bike,
    b2Vec2 respawnPosition
);

bool IsBikeGrounded(const Bike& bike);

void UpdateBikeControls(
    const Bike& bike,
    const InputState& input,
    bool bikeGrounded,
    bool levelComplete
);

void LimitBikeAngularSpeed(
    const Bike& bike,
    bool bikeGrounded
);