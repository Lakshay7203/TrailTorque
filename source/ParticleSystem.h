#pragma once

#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <vector>


enum class ParticleType
{
    Dust,
    Boost
};


struct Particle
{
    b2Vec2 position;
    b2Vec2 velocity;

    float lifetime;
    float maxLifetime;

    float size;

    ParticleType type;
};


class ParticleSystem
{
public:

    void SpawnWheelieDust(
        b2Vec2 rearWheelPosition
    );


    void SpawnBoostBurst(
        b2Vec2 rearWheelPosition
    );


    void Update(
        float deltaTime,
        bool boostActive,
        b2Vec2 rearWheelPosition
    );


    void Render(
        SDL_Renderer* renderer,
        float cameraX
    ) const;


    void Clear();


private:

    void SpawnBoostTrail(
        b2Vec2 rearWheelPosition
    );


    std::vector<Particle> particles;


    float boostTrailTimer =
        0.0f;
};