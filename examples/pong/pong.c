#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include <P2.h>

#define FATAL(...) (fprintf(stderr, __VA_ARGS__), exit(-1)) 

struct Pong {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Rect player_rect;
  bool running;
}
typedef Pong;

void Pong_Run(Pong *game) {
  P2_Print("Game started.");
  game->running = true;

  bool keystates[4096];

  while (game->running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          game->running = false;
          break;
        case SDL_KEYDOWN:
          keystates[event.key.keysym.scancode] = true;
          break;
        case SDL_KEYUP:
          keystates[event.key.keysym.scancode] = false;
          break;
      }
    }

    if (keystates[SDL_SCANCODE_A]) {
      game->player_rect.x -= 2;
    }

    if (keystates[SDL_SCANCODE_D]) {
      game->player_rect.x += 2;
    }


    // Black.
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
    SDL_RenderClear(game->renderer);

    // White.
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(game->renderer, &game->player_rect);

    SDL_RenderPresent(game->renderer);

    P2_Print("Key A: %s.", keystates[SDL_SCANCODE_A] ? "Pressed" : "Released");
    P2_Print("Key D: %s.", keystates[SDL_SCANCODE_D] ? "Pressed" : "Released");
    P2_Print("Player Rect is {%d, %d, %d, %d}", game->player_rect.x, game->player_rect.y, game->player_rect.w, game->player_rect.h);
  }

  P2_Print("Game stopped.");
}

int main(int argc, char* argv[]) {
  P2_Init();
  Pong game = {0};

  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    FATAL("SDL_Init fail. %s", SDL_GetError());
  }

  game.window = SDL_CreateWindow("Pong.", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
  if (game.window == NULL) {
    FATAL("SDL_CreateWindow fail. %s", SDL_GetError());
  }

  game.renderer = SDL_CreateRenderer(game.window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (game.renderer == NULL) {
    FATAL("SDL_CreateRenderer fail. %s", SDL_GetError());
  }

  game.player_rect = (SDL_Rect){(640-100)/2, 480 - 40, 100, 20};

  Pong_Run(&game);
  P2_Deinit();
}

