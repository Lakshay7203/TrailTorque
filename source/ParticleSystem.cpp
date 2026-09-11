#include "ParticleSystem.h"

#include <algorithm>


// =========================================================
// RENDER SETTINGS
// Keep these matched with main.cpp for now.
// =========================================================

namespace
{
    constexpr float PIXELS_PER_METER = 40.0f;
    constexpr float CAMERA_TARGET_X = 400.0f;
    constexpr float SCREEN_CENTER_Y = 300.0f;
}


// =========================================================
// WHEELIE DUST
// =========================================================

void ParticleSystem::SpawnWheelieDust(
    b2Vec2 rearWheelPosition)
{
    for (int i = 0; i < 8; ++i)
    {
        Particle particle;

        float horizontalOffset =
            static_cast<float>(i - 4) *
            0.04f;


        particle.position =
        {
            rearWheelPosition.x -
                0.15f +
                horizontalOffset,

            rearWheelPosition.y -
                0.25f
        };


        particle.velocity =
        {
            -0.8f -
                static_cast<float>(i) *
                0.10f,

            0.7f +
                static_cast<float>(i % 3) *
                0.18f
        };


        particle.lifetime =
            0.45f;

        particle.maxLifetime =
            particle.lifetime;


        particle.size =
            0.10f +
            static_cast<float>(i % 3) *
            0.025f;


        particle.type =
            ParticleType::Dust;


        particles.push_back(
            particle
        );
    }
}


// =========================================================
// BOOST BURST
// =========================================================

void ParticleSystem::SpawnBoostBurst(
    b2Vec2 rearWheelPosition)
{
    for (int i = 0; i < 10; ++i)
    {
        Particle particle;


        float verticalSpread =
            static_cast<float>(i - 5) *
            0.10f;


        particle.position =
        {
            rearWheelPosition.x -
                0.35f,

            rearWheelPosition.y
        };


        particle.velocity =
        {
            -2.5f -
                static_cast<float>(i) *
                0.25f,

            verticalSpread
        };


        particle.lifetime =
            0.35f;

        particle.maxLifetime =
            particle.lifetime;


        particle.size =
            0.08f;


        particle.type =
            ParticleType::Boost;


        particles.push_back(
            particle
        );
    }
}


// =========================================================
// BOOST TRAIL
// =========================================================

void ParticleSystem::SpawnBoostTrail(
    b2Vec2 rearWheelPosition)
{
    for (int i = 0; i < 2; ++i)
    {
        Particle particle;


        particle.position =
        {
            rearWheelPosition.x -
                0.45f,

            rearWheelPosition.y +
                static_cast<float>(i) *
                0.12f
        };


        particle.velocity =
        {
            -3.5f -
                static_cast<float>(i) *
                0.8f,

            0.0f
        };


        particle.lifetime =
            0.22f;

        particle.maxLifetime =
            particle.lifetime;


        particle.size =
            0.07f;


        particle.type =
            ParticleType::Boost;


        particles.push_back(
            particle
        );
    }
}


// =========================================================
// UPDATE
// =========================================================

void ParticleSystem::Update(
    float deltaTime,
    bool boostActive,
    b2Vec2 rearWheelPosition)
{
    // -----------------------------------------------------
    // CREATE CONTINUOUS BOOST TRAIL
    // -----------------------------------------------------

    if (boostActive)
    {
        boostTrailTimer -=
            deltaTime;


        if (boostTrailTimer <= 0.0f)
        {
            SpawnBoostTrail(
                rearWheelPosition
            );


            boostTrailTimer =
                0.04f;
        }
    }
    else
    {
        boostTrailTimer =
            0.0f;
    }


    // -----------------------------------------------------
    // UPDATE EXISTING PARTICLES
    // -----------------------------------------------------

    for (Particle& particle :
        particles)
    {
        particle.position.x +=
            particle.velocity.x *
            deltaTime;


        particle.position.y +=
            particle.velocity.y *
            deltaTime;


        // Dust gets simple gravity.
        if (particle.type ==
            ParticleType::Dust)
        {
            particle.velocity.y -=
                1.5f *
                deltaTime;
        }


        particle.lifetime -=
            deltaTime;
    }


    // -----------------------------------------------------
    // REMOVE DEAD PARTICLES
    // -----------------------------------------------------

    particles.erase(
        std::remove_if(
            particles.begin(),
            particles.end(),

            [](const Particle& particle)
            {
                return
                    particle.lifetime <=
                    0.0f;
            }
        ),

        particles.end()
    );
}


// =========================================================
// RENDER
// =========================================================

void ParticleSystem::Render(
    SDL_Renderer* renderer,
    float cameraX) const
{
    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND
    );


    for (const Particle& particle :
        particles)
    {
        // Convert world position -> screen position.

        float screenX =
            CAMERA_TARGET_X +
            (
                particle.position.x -
                cameraX
                ) *
            PIXELS_PER_METER;


        float screenY =
            SCREEN_CENTER_Y -
            particle.position.y *
            PIXELS_PER_METER;


        float lifePercent =
            particle.lifetime /
            particle.maxLifetime;


        // =================================================
        // DUST
        // =================================================

        if (particle.type ==
            ParticleType::Dust)
        {
            float sizePixels =
                particle.size *
                PIXELS_PER_METER;


            SDL_FRect dustRect =
            {
                screenX -
                    sizePixels / 2.0f,

                screenY -
                    sizePixels / 2.0f,

                sizePixels,
                sizePixels
            };


            Uint8 alpha =
                static_cast<Uint8>(
                    180.0f *
                    lifePercent
                    );


            SDL_SetRenderDrawColor(
                renderer,
                145,
                105,
                70,
                alpha
            );


            SDL_RenderFillRect(
                renderer,
                &dustRect
            );
        }


        // =================================================
        // BOOST STREAK
        // =================================================

        else
        {
            float height =
                particle.size *
                PIXELS_PER_METER;


            float width =
                height *
                5.0f;


            SDL_FRect boostRect =
            {
                screenX - width,
                screenY -
                    height / 2.0f,

                width,
                height
            };


            Uint8 alpha =
                static_cast<Uint8>(
                    230.0f *
                    lifePercent
                    );


            SDL_SetRenderDrawColor(
                renderer,
                255,
                185,
                40,
                alpha
            );


            SDL_RenderFillRect(
                renderer,
                &boostRect
            );
        }
    }
}


// =========================================================
// CLEAR
// =========================================================

void ParticleSystem::Clear()
{
    particles.clear();

    boostTrailTimer =
        0.0f;
}