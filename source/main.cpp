#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

int main(int argc, char* argv[])
{
    // Initialize SDL's video system.
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }

    // Create the main game window.
    // Title: TrailTorque
    // Resolution: 1280 x 720
    SDL_Window* window = SDL_CreateWindow(
        "TrailTorque",
        1280,
        720,
        0
    );

    // Create a renderer attached to our window.
    // The renderer is responsible for drawing things to the screen.
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        nullptr
    );

    // Check if the window failed to create.
    if (!window)
    {
        SDL_Log("Window creation failed: %s", SDL_GetError());

        SDL_Quit();
        return 1;
    }

    // Check if renderer creation failed.
    if (!renderer)
    {
        SDL_Log("renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    float playerSpeed = 100.0f;

    // Temporary player rectangle.
    // x = 300
    // y = 300
    // width = 150
    // height = 50
    SDL_FRect player = { 300, 300, 150, 50 };

    Uint64 previousTime = SDL_GetTicksNS();

    // Main game loop.
   // This keeps repeating until running becomes false.
    while (running)
    {
        Uint64 currentTime = SDL_GetTicksNS();
        float deltaTime = (currentTime - previousTime) / 1000000000.0f;
		previousTime = currentTime;

        const float amount = playerSpeed * deltaTime;

        if(player.x < 0)
		{
			player.x = 0;
		}

        const float xMax = 1280 - player.w;
		if(player.x > xMax)
		{
			player.x = xMax;
		}

		if(player.y < 0)
		{
			player.y = 0;
		}

        const float yMax = 720 - player.h;
		if(player.y > yMax)
		{
			player.y = yMax;
		}

        SDL_Event event;
     
        // Process all events currently waiting in SDL's event queue.
        while (SDL_PollEvent(&event))
        {
            // If the player closes the window,
            // stop the main game loop.
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
        }

        const bool* keyboardState = SDL_GetKeyboardState(nullptr);

        // Number 1 means pixel per frame
        if (keyboardState[SDL_SCANCODE_W]) {

            player.y -= amount;
        }

		if (keyboardState[SDL_SCANCODE_S]) {

			player.y += amount;
		}

        if (keyboardState[SDL_SCANCODE_A]) {
        
            player.x -= amount;
        }

		if (keyboardState[SDL_SCANCODE_D]) {

			player.x += amount;
		}

        if(keyboardState[SDL_SCANCODE_R])
		{
            player.x = 300;
			player.y = 300;
		}
        // -------------------------
        // RENDERING
        // -------------------------

        // Set background drawing color.
        // RGBA = Red, Green, Blue, Alpha.
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        // Clear the previous frame using the current drawing color.
        SDL_RenderClear(renderer);

        // Change the drawing color to black.
       // This color will be used to draw the player rectangle.
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        // Draw the player rectangle.
        SDL_RenderFillRect(renderer, &player);

        // Display everything we drew during this frame.
        SDL_RenderPresent(renderer);
    }

    // -------------------------
   // CLEANUP
   // -------------------------

   // Destroy resources in reverse order of creation.
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}