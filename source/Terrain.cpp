#include "Terrain.h"

Terrain CreateTerrain(b2WorldId worldId)
{
    Terrain terrain;

    // =====================================================
    // FLAT GROUND
    // =====================================================

    b2BodyDef groundBodyDef =
        b2DefaultBodyDef();

    groundBodyDef.position =
        b2Vec2{ 0.0f, -10.0f };

    terrain.groundBodyId =
        b2CreateBody(
            worldId,
            &groundBodyDef
        );

    b2ShapeDef groundShapeDef =
        b2DefaultShapeDef();

    groundShapeDef.material.friction =
        0.9f;

    b2Polygon groundBox =
        b2MakeBox(
            TERRAIN_GROUND_HALF_WIDTH,
            TERRAIN_GROUND_HEIGHT / 2.0f
        );

    b2ShapeId groundShapeId =
        b2CreatePolygonShape(
            terrain.groundBodyId,
            &groundShapeDef,
            &groundBox
        );

    b2Shape_SetFriction(
        groundShapeId,
        1.0f
    );


    // =====================================================
    // HILL BODY
    // =====================================================

    b2BodyDef hillBodyDef =
        b2DefaultBodyDef();

    terrain.hillBodyId =
        b2CreateBody(
            worldId,
            &hillBodyDef
        );

    b2ShapeDef hillShapeDef =
        b2DefaultShapeDef();

    hillShapeDef.material.friction =
        1.0f;


    // =====================================================
    // COURSE
    // =====================================================

    terrain.segments =
    {
        // Small warm-up hill.
        {
            b2Vec2{ 6.0f, -9.0f },
            b2Vec2{ 10.0f, -7.8f }
        },

        {
            b2Vec2{ 10.0f, -7.8f },
            b2Vec2{ 14.0f, -9.0f }
        },


        // First jump ramp.
        {
            b2Vec2{ 18.0f, -9.0f },
            b2Vec2{ 22.0f, -6.8f }
        },


        // Landing hill.
        {
            b2Vec2{ 27.0f, -9.0f },
            b2Vec2{ 31.0f, -7.4f }
        },

        {
            b2Vec2{ 31.0f, -7.4f },
            b2Vec2{ 35.0f, -9.0f }
        },


        // Bigger hill.
        {
            b2Vec2{ 39.0f, -9.0f },
            b2Vec2{ 44.0f, -6.5f }
        },

        {
            b2Vec2{ 44.0f, -6.5f },
            b2Vec2{ 49.0f, -9.0f }
        },


        // Final jump.
        {
            b2Vec2{ 54.0f, -9.0f },
            b2Vec2{ 59.0f, -6.2f }
        },


        // Final landing hill.
        {
            b2Vec2{ 65.0f, -9.0f },
            b2Vec2{ 69.0f, -7.5f }
        },

        {
            b2Vec2{ 69.0f, -7.5f },
            b2Vec2{ 74.0f, -9.0f }
        }
    };


    // =====================================================
    // CREATE BOX2D SEGMENTS
    // =====================================================

    for (const TerrainSegment& terrainSegment : terrain.segments)
    {
        b2Segment box2dSegment;

        box2dSegment.point1 =
            terrainSegment.start;

        box2dSegment.point2 =
            terrainSegment.end;

        b2CreateSegmentShape(
            terrain.hillBodyId,
            &hillShapeDef,
            &box2dSegment
        );
    }

    return terrain;
}