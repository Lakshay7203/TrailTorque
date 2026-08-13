#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

// Window settings
constexpr float SCREEN_WIDTH = 1280.0f;
constexpr float SCREEN_HEIGHT = 720.0f;

// Starting player position
constexpr float PLAYER_START_X = 300.0f;
constexpr float PLAYER_START_Y = 300.0f;


// ---------------------------------------------------------
// INPUT
// Handles window events and keyboard controls.
// ---------------------------------------------------------
void ProcessInput(bool& running, SDL_FRect& player, float amount)
{
    SDL_Event event;

    // Process SDL events.
    while (SDL_PollEvent(&event))
    {
        // Close the game when the user clicks X.
        if (event.type == SDL_EVENT_QUIT)
        {
            running = false;
        }
    }

    // Get the current keyboard state.
    const bool* keyboardState = SDL_GetKeyboardState(nullptr);

    // Move up.
    if (keyboardState[SDL_SCANCODE_W])
    {
        player.y -= amount;
    }

    // Move down.
    if (keyboardState[SDL_SCANCODE_S])
    {
        player.y += amount;
    }

    // Move left.
    if (keyboardState[SDL_SCANCODE_A])
    {
        player.x -= amount;
    }

    // Move right.
    if (keyboardState[SDL_SCANCODE_D])
    {
        player.x += amount;
    }

    // Reset the player.
    if (keyboardState[SDL_SCANCODE_R])
    {
        player.x = PLAYER_START_X;
        player.y = PLAYER_START_Y;
    }
}


// ---------------------------------------------------------
// UPDATE
// Updates game logic after input.
// ---------------------------------------------------------
void Update(SDL_FRect& player)
{
    // Prevent player from leaving the left side.
    if (player.x < 0.0f)
    {
        player.x = 0.0f;
    }

    // Calculate maximum allowed horizontal position.
    const float xMax = SCREEN_WIDTH - player.w;

    // Prevent player from leaving the right side.
    if (player.x > xMax)
    {
        player.x = xMax;
    }

    // Prevent player from leaving the top.
    if (player.y < 0.0f)
    {
        player.y = 0.0f;
    }

    // Calculate maximum allowed vertical position.
    const float yMax = SCREEN_HEIGHT - player.h;

    // Prevent player from leaving the bottom.
    if (player.y > yMax)
    {
        player.y = yMax;
    }
}


// ---------------------------------------------------------
// RENDER
// Draws the current game state.
// ---------------------------------------------------------
void Render(SDL_Renderer* renderer, const SDL_FRect& player)
{
    // Set background color to red.
    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);

    // Clear the previous frame.
    SDL_RenderClear(renderer);

    // Change drawing color to black.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // Draw the player rectangle.
    SDL_RenderFillRect(renderer, &player);

    // Show the completed frame.
    SDL_RenderPresent(renderer);
}


// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main(int argc, char* argv[])
{
    // Initialize SDL video systems.
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

    // Always check the window BEFORE creating the renderer.
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

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        nullptr
    );

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


    // -----------------------------------------------------
    // GAME VARIABLES
    // -----------------------------------------------------

    bool running = true;

    // Speed measured in pixels per second.
    const float playerSpeed = 100.0f;

    // Temporary player rectangle.
    SDL_FRect player =
    {
        PLAYER_START_X,
        PLAYER_START_Y,
        150.0f,
        50.0f
    };

    // Store the starting time for delta-time calculation.
    Uint64 previousTime = SDL_GetTicksNS();


    // -----------------------------------------------------
    // MAIN GAME LOOP
    // -----------------------------------------------------

    while (running)
    {
        // Get current frame time.
        const Uint64 currentTime = SDL_GetTicksNS();

        // Calculate how many seconds passed since last frame.
        const float deltaTime =
            static_cast<float>(currentTime - previousTime)
            / 1000000000.0f;

        // Current frame becomes the previous frame
        // for the next loop iteration.
        previousTime = currentTime;

        // Calculate how far the player should move this frame.
        const float amount = playerSpeed * deltaTime;


        // 1. Read player input.
        ProcessInput(running, player, amount);

        // 2. Update game logic.
        Update(player);

        // 3. Draw the game.
        Render(renderer, player);
    }


    // -----------------------------------------------------
    // CLEANUP
    // -----------------------------------------------------

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}