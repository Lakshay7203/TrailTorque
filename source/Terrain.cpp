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
        b2Vec2{ 0.0f, -16.0f };

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
        // START PLATFORM
        { { -20.0f, -9.0f }, { 8.0f, -9.0f } },

        // Small rolling hill
        { { 8.0f, -9.0f }, { 15.0f, -6.8f } },
        { { 15.0f, -6.8f }, { 23.0f, -8.7f } },

        // Valley into medium hill
        { { 23.0f, -8.7f }, { 31.0f, -9.5f } },
        { { 31.0f, -9.5f }, { 40.0f, -6.5f } },
        { { 40.0f, -6.5f }, { 49.0f, -8.8f } },

        // Uneven off-road section
        { { 49.0f, -8.8f }, { 55.0f, -7.6f } },
        { { 55.0f, -7.6f }, { 61.0f, -9.2f } },
        { { 61.0f, -9.2f }, { 68.0f, -7.2f } },

        // BIG climb
        { { 68.0f, -7.2f }, { 78.0f, -4.5f } },
        { { 78.0f, -4.5f }, { 88.0f, -2.5f } },

        // Long downhill
        { { 88.0f, -2.5f }, { 98.0f, -5.0f } },
        { { 98.0f, -5.0f }, { 110.0f, -9.0f } },

        // Rolling bumps
        { { 110.0f, -9.0f }, { 116.0f, -7.7f } },
        { { 116.0f, -7.7f }, { 122.0f, -9.2f } },
        { { 122.0f, -9.2f }, { 129.0f, -7.0f } },
        { { 129.0f, -7.0f }, { 136.0f, -8.8f } },

        // Second big hill
        { { 136.0f, -8.8f }, { 146.0f, -5.5f } },
        { { 146.0f, -5.5f }, { 154.0f, -3.7f } },
        { { 154.0f, -3.7f }, { 163.0f, -5.5f } },
        { { 163.0f, -5.5f }, { 173.0f, -9.0f } },

        // Final rough section
        { { 173.0f, -9.0f }, { 179.0f, -7.5f } },
        { { 179.0f, -7.5f }, { 185.0f, -8.8f } },
        { { 185.0f, -8.8f }, { 192.0f, -7.0f } },
        { { 192.0f, -7.0f }, { 200.0f, -9.0f } },

        // =====================================================
        // EXTENDED OFFROAD SECTION
        // =====================================================

        // Long rolling climb
        { { 200.0f, -9.0f }, { 208.0f, -7.8f } },
        { { 208.0f, -7.8f }, { 216.0f, -5.8f } },
        { { 216.0f, -5.8f }, { 225.0f, -7.2f } },
        { { 225.0f, -7.2f }, { 233.0f, -9.2f } },

        // Rough valley
        { { 233.0f, -9.2f }, { 239.0f, -7.4f } },
        { { 239.0f, -7.4f }, { 245.0f, -8.8f } },
        { { 245.0f, -8.8f }, { 252.0f, -6.8f } },

        // Large mountain
        { { 252.0f, -6.8f }, { 262.0f, -4.2f } },
        { { 262.0f, -4.2f }, { 272.0f, -2.2f } },
        { { 272.0f, -2.2f }, { 282.0f, -5.0f } },
        { { 282.0f, -5.0f }, { 291.0f, -8.8f } },

        // Final rollers
        { { 291.0f, -8.8f }, { 297.0f, -7.2f } },
        { { 297.0f, -7.2f }, { 303.0f, -8.6f } },
        { { 303.0f, -8.6f }, { 309.0f, -7.5f } },
        { { 309.0f, -7.5f }, { 390.0f, -9.0f } }
           
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