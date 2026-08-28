#pragma once

#include <box2d/box2d.h>
#include <vector>

inline constexpr float TERRAIN_GROUND_HALF_WIDTH = 100.0f;
inline constexpr float TERRAIN_GROUND_HEIGHT = 2.0f;

struct TerrainSegment
{
    b2Vec2 start;
    b2Vec2 end;
};

struct Terrain
{
    b2BodyId groundBodyId;
    b2BodyId hillBodyId;

    std::vector<TerrainSegment> segments;
};

Terrain CreateTerrain(b2WorldId worldId);