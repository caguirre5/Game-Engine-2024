#include <memory>
#include <SDL.h>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int MAX_FPS = 60;

const int BALL_SPEED = 300;
const int BALL_SIZE = 20;

const int PADEL_SPEED = 400;
const int PADEL_H = 10;
const int PADEL_W = 150;

const int BRICK_H = 20;
const int BRICK_W = 62;

const float SPEED_INCREMENT = 1.1f;

Uint32 lastUpdateTime = 0;

struct Rect {
    SDL_Rect rect = {0,0,100,100};
    int vx = 0;
    int vy = 0;
    SDL_Color color = {0xFF, 0x00, 0xFF, 0xFF};
};

SDL_Color red = {0xFF, 0x00, 0x00, 0xFF};
SDL_Color blue = {0x00, 0x00, 0xFF, 0xFF};
SDL_Color white = {0xFF, 0xFF, 0xFF, 0xFF};

SDL_Color interpolateColor(SDL_Color startColor, SDL_Color endColor, float factor) {
    SDL_Color resultColor;
    resultColor.r = static_cast<Uint8>(startColor.r + factor * (endColor.r - startColor.r));
    resultColor.g = static_cast<Uint8>(startColor.g + factor * (endColor.g - startColor.g));
    resultColor.b = static_cast<Uint8>(startColor.b + factor * (endColor.b - startColor.b));
    resultColor.a = 0xFF; // Alpha is fully opaque
    return resultColor;
}

Rect padel = {{(SCREEN_WIDTH /2) - PADEL_W, SCREEN_HEIGHT-30, PADEL_W, PADEL_H}, PADEL_SPEED, 0, white};
Rect ball = {{(SCREEN_WIDTH /2) - BALL_SIZE, SCREEN_WIDTH/2, BALL_SIZE, BALL_SIZE}, 0, 100, blue};

std::vector<std::unique_ptr<Rect>> bricks;

void renderRect (SDL_Renderer* renderer, Rect& rect) {
    SDL_SetRenderDrawColor(renderer, rect.color.r, rect.color.g, rect.color.b, rect.color.a);
    SDL_RenderFillRect(renderer, &rect.rect);
}

void createBricks() {
    int totalBricks = 9 * 6;
    int currentBrick = 0;

    // Define start and end colors for the gradient
    SDL_Color startColor = {0xFF, 0x00, 0x00, 0xFF};
    SDL_Color endColor = {0x00, 0x00, 0xFF, 0xFF};

    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 6; ++j) {
            float factor = static_cast<float>(currentBrick) / (totalBricks - 1);
            SDL_Color brickColor = interpolateColor(startColor, endColor, factor);
            bricks.push_back(std::make_unique<Rect>(Rect{{i * (BRICK_W + 10), j * (BRICK_H + 10), BRICK_W, BRICK_H}, 0, 0, brickColor}));
            currentBrick++;
        }
    }
}

enum CollisionType {
    NONE,
    HORIZONTAL,
    VERTICAL
};

CollisionType checkCollision(const SDL_Rect& a, const SDL_Rect& b) {
    int leftA = a.x;
    int rightA = a.x + a.w;
    int topA = a.y;
    int bottomA = a.y + a.h;

    int leftB = b.x;
    int rightB = b.x + b.w;
    int topB = b.y;
    int bottomB = b.y + b.h;

    if (rightA <= leftB || leftA >= rightB || bottomA <= topB || topA >= bottomB) {
        return NONE;
    }

    int overlapLeft = rightA - leftB;
    int overlapRight = rightB - leftA;
    int overlapTop = bottomA - topB;
    int overlapBottom = bottomB - topA;

    if (overlapLeft < overlapRight && overlapLeft < overlapTop && overlapLeft < overlapBottom) {
        return HORIZONTAL;
    } else if (overlapRight < overlapLeft && overlapRight < overlapTop && overlapRight < overlapBottom) {
        return HORIZONTAL;
    } else {
        return VERTICAL;
    }
}

void handleInput(SDL_Event& e) {
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);

    padel.vx = 0;
    padel.vy = 0;

    if (keyboardState[SDL_SCANCODE_A]) {
        padel.vx = -PADEL_SPEED;
    }
    if (keyboardState[SDL_SCANCODE_D]) {
        padel.vx = PADEL_SPEED;
    }
}

void update(float dT, bool& quit) {
    if (padel.rect.x < 0) {
        padel.rect.x = 0; // Asegura que el padel no salga de la ventana
        padel.vx = 0; // Detiene el movimiento hacia la izquierda
    }
    if (padel.rect.x + padel.rect.w > SCREEN_WIDTH) {
        padel.rect.x = SCREEN_WIDTH - padel.rect.w; // Asegura que el padel no salga de la ventana
        padel.vx = 0; // Detiene el movimiento hacia la derecha
    }

    if (ball.rect.x < 0) {
        ball.vx *= -1;
    }

    if (ball.rect.y < 0) {
        ball.vy *= -1;
    }

    if (ball.rect.x + ball.rect.w > SCREEN_WIDTH) {
        ball.vx *= -1;
    }

    if (ball.rect.y + ball.rect.h > SCREEN_HEIGHT) {

        std::cout << "Game Over" << std::endl;
        quit = true;
    }


    CollisionType collision = checkCollision(padel.rect, ball.rect);
    if (collision != NONE) {
        // Calculate the relative hit position
        float hitPos = (ball.rect.x + (ball.rect.w / 2.0f)) - (padel.rect.x + (padel.rect.w / 2.0f));
        float normalizedHitPos = hitPos / (padel.rect.w / 2.0f);

        ball.vx = normalizedHitPos * BALL_SPEED; // Adjust vx based on hit position
        ball.vy *= -SPEED_INCREMENT; // Invert vy and increase speed
    }

    for (auto it = bricks.begin(); it != bricks.end(); ) {
        collision = checkCollision(ball.rect, (*it)->rect);
        if (collision != NONE) {
            if (collision == HORIZONTAL) {
                ball.vx *= -1;
            } else if (collision == VERTICAL) {
                ball.vy *= -1;
            }
            it = bricks.erase(it); // Eliminate the brick and free memory
        } else {
            ++it;
        }
    }

    if (bricks.empty()) {
        quit = true;
    }

    padel.rect.x += padel.vx * dT;
    padel.rect.y += padel.vy * dT;
    ball.rect.x += ball.vx * dT;
    ball.rect.y += ball.vy * dT;
}

int main(int argc, char* args[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Hello SDL World", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    createBricks();

    bool quit = false;
    SDL_Event e;

    Uint32 frameCount = 1;
    Uint32 frameStartTimestamp;
    Uint32 frameEndTimestamp;
    Uint32 lastFrameTime = 0;
    float frameDuration = (1.0/MAX_FPS) * 1000.0;
    float actualFrameDuration;
    int FPS = MAX_FPS;

    while (!quit) {
        frameStartTimestamp = SDL_GetTicks();

        Uint32 currentFrameTime = SDL_GetTicks();
        float dT = (currentFrameTime - lastFrameTime) / 1000.0;
        lastFrameTime = currentFrameTime;

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            handleInput(e);
        }

        // update
        update(dT, quit);

        // render
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);

        renderRect(renderer, padel);
        renderRect(renderer, ball);
        for (const auto& brick : bricks) {
            renderRect(renderer, *brick);
        }

        SDL_RenderPresent(renderer);

        frameEndTimestamp = SDL_GetTicks();
        actualFrameDuration = frameEndTimestamp - frameStartTimestamp;

        if (actualFrameDuration < frameDuration) {
            SDL_Delay(frameDuration - actualFrameDuration);
        }

        // fps calculation
        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        Uint32 elapsedTime = currentTime - lastUpdateTime;
        if (elapsedTime > 1000) {
            FPS = (float)frameCount / (elapsedTime / 1000.0);
            lastUpdateTime = currentTime;
            frameCount = 0;
        }
        SDL_SetWindowTitle(window, ("FPS: " + std::to_string(FPS)).c_str());
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
