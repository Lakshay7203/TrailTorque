#pragma once

#include <box2d/box2d.h>

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

void ResetBike(
    Bike& bike,
    b2Vec2 respawnPosition
);