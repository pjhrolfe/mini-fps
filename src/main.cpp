#define SDL_MAIN_HANDLED

#include <iostream>
#include <cmath>

#include <SDL.h>
#include <SDL_image.h>

#include "Camera.h"
#include "Color.h"
#include "Level.h"
#include "Renderer.h"
#include "Utilities.h"

// TODO: Multiple resolutions
const size_t SCREEN_WIDTH = 1600;
const size_t SCREEN_HEIGHT = 900;

const float RENDER_RAY_INCREMENT = 0.005f;
const size_t RENDER_DISTANCE = 128;

const float SPEED_MODIFIER = 5;
const float ROTATION_MODIFIER = 0.5;

void get_input_state(bool &gameIsRunning, bool &moveLeft, bool &moveRight, bool &moveForward,
                     bool &moveBack, int &mouseX, int &mouseY) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            gameIsRunning = false;
        }

        if (event.type == SDL_MOUSEMOTION) {
            mouseX = event.motion.xrel;
            mouseY = event.motion.yrel;
        }
    }

    const Uint8 *currentKeyStates = SDL_GetKeyboardState(nullptr);

    if (currentKeyStates[SDL_SCANCODE_ESCAPE]) {
        gameIsRunning = false;
    }

    if (currentKeyStates[SDL_SCANCODE_W]) {
        moveForward = true;
    } else {
        moveForward = false;
    }

    if (currentKeyStates[SDL_SCANCODE_S]) {
        moveBack = true;
    } else {
        moveBack = false;
    }

    if (currentKeyStates[SDL_SCANCODE_A]) {
        moveLeft = true;
    } else {
        moveLeft = false;
    }

    if (currentKeyStates[SDL_SCANCODE_D]) {
        moveRight = true;
    } else {
        moveRight = false;
    }
}

bool has_collided(Level& level, const float x, const float y) {
    bool collided = false;

    int roundedX = round(x);
    int roundedY = round(y);

    for (size_t cellX = roundedX - 1; cellX <= roundedX + 1; cellX++) {
        for (size_t cellY = roundedY - 1; cellY <= roundedY + 1; cellY++) {
            if (level.get(cellX, cellY) != RGBA_WHITE) {
                if (x >= cellX - 0.05 && x <= cellX + 1 + 0.05 &&
                    y >= cellY - 0.05 && y <= cellY + 1 + 0.05) {
                    collided = true;
                }
            }
        }
    }

    return collided;
}

int main() {
    if (!initialize_sdl()) {
        std::cerr << "SDL could not be initialized:" << SDL_GetError();
    } else {
        std::cout << "SDL initialized" << std::endl;
    }

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    const std::string assetsFilePath = assets_file_path();

    if (!initialize_window_and_renderer(&window, &renderer, SCREEN_WIDTH, SCREEN_HEIGHT)) {
        std::cerr << "Window and/or renderer could not be initialized" << std::endl;
    } else {
        std::cout << "Window and renderer initialized" << std::endl;
    }

    if (!initialize_sdl_image()) {
        std::cerr << "SDL_image could not be initialized" << std::endl;
    } else {
        std::cout << "SDL_image initialized" << std::endl;
    }

    std::string levelFilePath = assetsFilePath + "levels/testLevel10.png";

    Level level = Level(levelFilePath.c_str());
    level.print();

    bool gameIsRunning = true;
    int mouseX, mouseY;
    bool moveLeft, moveRight, moveForward, moveBack;

    Camera playerCamera(2, 2, 1.523, (70.0 / 360.0) * 2 * M_PI, SCREEN_WIDTH, SCREEN_HEIGHT, RENDER_RAY_INCREMENT,
                        RENDER_DISTANCE, 1);

    // TEMP
    std::string texFilePath = assetsFilePath + "sprites/testWall2.png";
    SDL_Surface *tmpTexSurface = IMG_Load(texFilePath.c_str());
    size_t texSize = tmpTexSurface->w; // Only use square textures
    Uint32 texBuffer[texSize][texSize];
    Uint32 **texBufferPtr = new Uint32 *[texSize];
    for (int i = 0; i < texSize; i++) {
        texBufferPtr[i] = texBuffer[i];
    }

    Uint32 *pixels = (Uint32 *) tmpTexSurface->pixels;

    for (int p = 0; p < texSize; p++) {
        for (int q = 0; q < texSize; q++) {
            texBuffer[p][q] = pixels[p * texSize + q];
        }
    }

    SDL_FreeSurface(tmpTexSurface);

    SDL_SetRelativeMouseMode(SDL_TRUE);

    double oldTime, curTime, frameDelta;
    curTime = 0;

    SDL_Texture *frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                                  playerCamera.viewportWidth, playerCamera.viewportHeight);

    while (gameIsRunning) {
        oldTime = curTime;
        curTime = SDL_GetTicks64();

        frameDelta = frame_time(oldTime, curTime);

        std::cout << frames_per_second(oldTime, curTime);

        mouseX = 0;
        mouseY = 0;
        get_input_state(gameIsRunning, moveLeft, moveRight, moveForward, moveBack, mouseX, mouseY);

        float prevPlayerCameraX = playerCamera.x;
        float prevPlayerCameraY = playerCamera.y;
        float prevPlayerCameraAngle = playerCamera.angle;

        playerCamera.angle += mouseX * frameDelta * ROTATION_MODIFIER;

        if (moveForward) {
            playerCamera.x += frameDelta * SPEED_MODIFIER * cos(playerCamera.angle);
            playerCamera.y += frameDelta * SPEED_MODIFIER *  sin(playerCamera.angle);
        }

        if (moveBack) {
            playerCamera.x -= frameDelta * SPEED_MODIFIER *  cos(playerCamera.angle);
            playerCamera.y -= frameDelta * SPEED_MODIFIER *  sin(playerCamera.angle);
        }

        if (moveLeft) {
            playerCamera.x += frameDelta * SPEED_MODIFIER *  cos(playerCamera.angle - M_PI / 2);
            playerCamera.y += frameDelta * SPEED_MODIFIER *  sin(playerCamera.angle - M_PI / 2);
        }

        if (moveRight) {
            playerCamera.x += frameDelta * SPEED_MODIFIER *  cos(playerCamera.angle + M_PI / 2);
            playerCamera.y += frameDelta * SPEED_MODIFIER *  sin(playerCamera.angle + M_PI / 2);
        }


        if (has_collided(level, playerCamera.x, playerCamera.y)) {
            playerCamera.x = prevPlayerCameraX;
            playerCamera.y = prevPlayerCameraY;
        }

        // Only rerender the screen if something's changed
        // TODO: Update this when animated sprites/enemies in game
        if (playerCamera.angle != prevPlayerCameraAngle || playerCamera.x != prevPlayerCameraX || playerCamera.y != prevPlayerCameraY) {
            draw(renderer, playerCamera, level, texBufferPtr, texSize, frameTexture);
        } else {
            SDL_Delay(1);
        }
    }

    quit(window);

    return 0;
}