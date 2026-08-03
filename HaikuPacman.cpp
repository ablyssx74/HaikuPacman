/*
 * Copyright 2026, Kris Beazley (ablyss) HaikuPacman@epluribusunix.net
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>
#include <string>

const int TILE_SIZE = 16;  
const int MAP_WIDTH = 28;  
const int MAP_HEIGHT = 22; 
const int SCREEN_WIDTH = MAP_WIDTH * TILE_SIZE;   
const int SCREEN_HEIGHT = MAP_HEIGHT * TILE_SIZE; 

// 1 = Wall, 0 = Pellet, 3 = Power Pellet, -1 = Empty, 2 = Ghost House Door
int initialMaze[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,3,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,3,1},
    {1,0,1,1,1,1,0,1,1,1,1,1,0,1,1,0,1,1,1,1,1,0,1,1,1,1,0,1},
    {1,0,1,1,1,1,0,1,1,1,1,1,0,1,1,0,1,1,1,1,1,0,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1}, 
    {1,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1},
    {1,1,1,0,1,1,0,1,1,1,1,1,0,1,1,0,1,1,1,1,1,0,1,1,0,1,1,1},
    {0,0,0,0,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,0,0,0,0},
    {1,1,1,1,1,1,0,1,1,0,1,1,1,2,2,1,1,1,0,1,1,0,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1},
    {0,0,0,0,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,0,0,0,0},
    {1,0,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,0,1,1,1,1,1,0,1,1,0,1,1,1,1,1,0,1,1,1,1,0,1},
    {1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1},
    {1,1,1,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,0,1,1,0,1,1,0,1,1,1},
    {1,0,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1}, 
    {1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int maze[MAP_HEIGHT][MAP_WIDTH];

int score = 0;
int lives = 3; 
int totalPellets = 0;
int initialPelletCount = 0;
Uint32 frameCounter = 0;
bool gameOver = false;

bool frightenedMode = false;
int frightenedTimer = 0;

bool cherrySpawned = false;
bool cherryEaten = false;
int cherryTileX = -1; 
int cherryTileY = -1; 

bool isDying = false;
int deathAnimFrame = 0;
const int MAX_DEATH_FRAMES = 32;

double audioPhase = 0.0;
double targetFrequency = 0.0;
double currentFrequency = 0.0;
int audioTimeRemaining = 0;

struct Player {
    float x; float y; float speed = 2.0f;       
    int dx, dy; int next_dx, next_dy;          
} pacman;

struct Ghost {
    float x; float y; float baseSpeed; float currentSpeed;      
    int dx, dy;
    int colorR, colorG, colorB;
} blinky, pinky, inky, clyde;

void AudioSynthCallback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    int16_t* buffer = (int16_t*)stream;
    int samples = len / 2;

    for (int i = 0; i < samples; i++) {
        if (audioTimeRemaining > 0) {
            currentFrequency += (targetFrequency - currentFrequency) * 0.05;
            audioPhase += (2.0 * M_PI * currentFrequency) / 44100.0;
            double rawWave = (sin(audioPhase) >= 0.0) ? 1.0 : -1.0;
            buffer[i] = (int16_t)(rawWave * 2000.0); 
            audioTimeRemaining--;
        } else {
            buffer[i] = 0; 
        }
    }
}

void PlaySynthTone(double freq, int durationSamples) {
    targetFrequency = freq;
    if (audioTimeRemaining <= 0) currentFrequency = freq;
    audioTimeRemaining = durationSamples;
}

void DrawFilledCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; 
            int dy = radius - h; 
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
}

void SoftResetPositions() {
    pacman.x = 1 * TILE_SIZE; pacman.y = 1 * TILE_SIZE;
    pacman.dx = 0; pacman.dy = 0; pacman.next_dx = 0; pacman.next_dy = 0;

    blinky.x = 13 * TILE_SIZE; blinky.y = 10 * TILE_SIZE; blinky.dx = 0; blinky.dy = -1;
    pinky.x = 14 * TILE_SIZE; pinky.y = 10 * TILE_SIZE; pinky.dx = 0; pinky.dy = -1;
    inky.x = 13 * TILE_SIZE; inky.y = 11 * TILE_SIZE; inky.dx = 0; inky.dy = -1;
    clyde.x = 14 * TILE_SIZE; clyde.y = 11 * TILE_SIZE; clyde.dx = 0; clyde.dy = -1;
    
    isDying = false; deathAnimFrame = 0;
}

void UpdateWindowTitle(SDL_Window* window) {
    std::string titleStr = "Score: " + std::to_string(score) + "  |  Lives: " + std::to_string(lives);
    if (frightenedMode) titleStr += "  [GHOSTS FRIGHTENED!]";
    if (cherrySpawned) titleStr += "  *CHERRY BONUS ACTIVE!*";
    if (gameOver) titleStr = "GAME OVER! Final Score: " + std::to_string(score) + " | Press SPACE to Restart";
    SDL_SetWindowTitle(window, titleStr.c_str());
}

void ResetGame(SDL_Window* window) {
    score = 0; lives = 3; totalPellets = 0; gameOver = false; frightenedMode = false; frightenedTimer = 0; frameCounter = 0;
    cherrySpawned = false; cherryEaten = false;
    
    for(int r=0; r<MAP_HEIGHT; ++r) {
        for(int c=0; c<MAP_WIDTH; ++c) {
            maze[r][c] = initialMaze[r][c];
            if (maze[r][c] == 0 || maze[r][c] == 3) totalPellets++;
        }
    }
    initialPelletCount = totalPellets;

    blinky.baseSpeed = 1.0f; blinky.currentSpeed = 1.0f; blinky.colorR = 255; blinky.colorG = 0; blinky.colorB = 0;
    pinky.baseSpeed = 1.0f; pinky.currentSpeed = 1.0f; pinky.colorR = 255; pinky.colorG = 182; pinky.colorB = 193;
    inky.baseSpeed = 1.0f; inky.currentSpeed = 1.0f; inky.colorR = 0; inky.colorG = 255; inky.colorB = 255;
    clyde.baseSpeed = 1.0f; clyde.currentSpeed = 1.0f; clyde.colorR = 255; clyde.colorG = 140; clyde.colorB = 0;

    SoftResetPositions();
    UpdateWindowTitle(window);
}

void NextLevel(SDL_Window* window) {
    frightenedMode = false; frightenedTimer = 0; frameCounter = 0; totalPellets = 0;
    cherrySpawned = false; cherryEaten = false;
    
    for(int r=0; r<MAP_HEIGHT; ++r) {
        for(int c=0; c<MAP_WIDTH; ++c) {
            maze[r][c] = initialMaze[r][c];
            if (maze[r][c] == 0 || maze[r][c] == 3) totalPellets++;
        }
    }
    initialPelletCount = totalPellets;

    blinky.baseSpeed += 0.25f; pinky.baseSpeed += 0.25f; inky.baseSpeed += 0.25f; clyde.baseSpeed += 0.25f;

    SoftResetPositions();
    PlaySynthTone(587.33, 15000); 
    UpdateWindowTitle(window);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return 1;

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = AudioSynthCallback;

    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audioDevice) {
        SDL_PauseAudioDevice(audioDevice, 0);
    }

    SDL_Window* window = SDL_CreateWindow("Haiku Pac-Man Clone", 
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    // Update Chcker
   	{
    const char* targetUrl = "https://raw.githubusercontent.com/ablyssx74/HaikuPacman/refs/heads/main/VERSION";
    const char* localVersion = "v1.0.1"; 
    char updateCmd[1024];
    snprintf(updateCmd, sizeof(updateCmd),
        #ifndef IS_HAIKU_32BIT
        "(REMOTE_V=$(curl -sL \"%s\" | tr -d '\\r\\n'); "
        #else
        "(REMOTE_V=$(curl-x86 -sL \"%s\" | tr -d '\\r\\n'); "
        #endif
        "if [ ! -z \"$REMOTE_V\" ] && [ \"$REMOTE_V\" != \"%s\" ]; then "
        "notify --title \"Update Available\" --group \"HaikuPacman\" "
        "\"A newer version of HaikuPacman is available! ($REMOTE_V)\"; fi) &",
        targetUrl, localVersion);	
    system(updateCmd);
   }
    
    
    ResetGame(window);
    bool quit = false;
    SDL_Event e;

    while (!quit) {
        frameCounter++;

        if (frightenedMode && !isDying) {
            frightenedTimer--;
            if (frightenedTimer <= 0) {
                frightenedMode = false;
                UpdateWindowTitle(window);
            }
        }

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) quit = true;
                if (gameOver) {
                    if (e.key.keysym.sym == SDLK_SPACE) ResetGame(window);
                } else if (!isDying) {
                    switch (e.key.keysym.sym) {
                        case SDLK_UP:    pacman.next_dx = 0;  pacman.next_dy = -1; break;
                        case SDLK_DOWN:  pacman.next_dx = 0;  pacman.next_dy = 1;  break;
                        case SDLK_LEFT:  pacman.next_dx = -1; pacman.next_dy = 0;  break;
                        case SDLK_RIGHT: pacman.next_dx = 1;  pacman.next_dy = 0;  break;
                    }
                }
            }
        }

        if (isDying) {
            deathAnimFrame++;
            if (deathAnimFrame % 4 == 0) {
                PlaySynthTone(400.0 - (deathAnimFrame * 8), 3000); 
            }
            if (deathAnimFrame >= MAX_DEATH_FRAMES) {
                lives--; 
                if (lives <= 0) {
                    gameOver = true;
                } else {
                    SoftResetPositions(); 
                }
                UpdateWindowTitle(window);
            }
        }

          // --- GLOBAL SIMULATION UPDATE LOOP ---
        if (!gameOver && !isDying) {
            // Pacman movement logic
            bool pAlignedX = ((int)pacman.x % TILE_SIZE == 0);
            bool pAlignedY = ((int)pacman.y % TILE_SIZE == 0);

            if (pAlignedX && pAlignedY) {
                int curTileX = (int)pacman.x / TILE_SIZE; int curTileY = (int)pacman.y / TILE_SIZE;
                int chkTileX = curTileX + pacman.next_dx;  int chkTileY = curTileY + pacman.next_dy;
                if (chkTileX >= 0 && chkTileX < MAP_WIDTH && chkTileY >= 0 && chkTileY < MAP_HEIGHT) {
                    if (maze[chkTileY][chkTileX] != 1) {
                        pacman.dx = pacman.next_dx; pacman.dy = pacman.next_dy;
                    }
                }
            }

            if (pacman.x < -TILE_SIZE) {
                pacman.x = SCREEN_WIDTH - TILE_SIZE;
                pacman.y = ((int)(pacman.y + TILE_SIZE / 2) / TILE_SIZE) * TILE_SIZE;
            } else if (pacman.x >= SCREEN_WIDTH) {
                pacman.x = 0;
                pacman.y = ((int)(pacman.y + TILE_SIZE / 2) / TILE_SIZE) * TILE_SIZE;
            }

            float pNextX = pacman.x + (pacman.dx * pacman.speed);
            float pNextY = pacman.y + (pacman.dy * pacman.speed);
            int pL = pNextX / TILE_SIZE; int pR = (pNextX + TILE_SIZE - 1) / TILE_SIZE;
            int pT = pNextY / TILE_SIZE; int pB = (pNextY + TILE_SIZE - 1) / TILE_SIZE;

            if (pL < 0 || pR >= MAP_WIDTH) {
                pacman.x = pNextX; pacman.y = pNextY;
            } else {
                if (maze[pT][pL] != 1 && maze[pT][pR] != 1 && maze[pB][pL] != 1 && maze[pB][pR] != 1) {
                    pacman.x = pNextX; pacman.y = pNextY;
                } else {
                    pacman.dx = 0; pacman.dy = 0;
                }
            }

            // Pellet Interaction Scoring
            int pGridX = ((int)pacman.x + TILE_SIZE / 2) / TILE_SIZE;
            int pGridY = ((int)pacman.y + TILE_SIZE / 2) / TILE_SIZE;
            if (pGridX >= 0 && pGridX < MAP_WIDTH && pGridY >= 0 && pGridY < MAP_HEIGHT) {
                if (maze[pGridY][pGridX] == 0) {
                    maze[pGridY][pGridX] = -1; totalPellets--; score += 10;
                    PlaySynthTone(240.0 + ((frameCounter % 2) * 60.0), 2000); 
                    UpdateWindowTitle(window);
                } 
                else if (maze[pGridY][pGridX] == 3) {
                    maze[pGridY][pGridX] = -1; totalPellets--; score += 50;
                    frightenedMode = true; frightenedTimer = 360;
                    PlaySynthTone(880.0, 6000); 
                    UpdateWindowTitle(window);
                }
                
                if (!cherrySpawned && !cherryEaten && ((float)totalPellets / (float)initialPelletCount <= 0.5f)) {
                    bool foundSafeSpot = false;
                    for (int r = 10; r < MAP_HEIGHT - 2 && !foundSafeSpot; r++) {
                        for (int c = 5; c < MAP_WIDTH - 5; c++) {
                            if (maze[r][c] == 0 || maze[r][c] == -1) {
                                cherryTileX = c; cherryTileY = r;
                                cherrySpawned = true; foundSafeSpot = true;
                                UpdateWindowTitle(window);
                                break;
                            }
                        }
                    }
                }

                if (cherrySpawned && pGridX == cherryTileX && pGridY == cherryTileY) {
                    cherrySpawned = false; cherryEaten = true; score += 500;
                    PlaySynthTone(1200.0, 10000); 
                    UpdateWindowTitle(window);
                }

                if (totalPellets <= 0) {
                    NextLevel(window);
                    continue; 
                }
            }

            // --- GHOST ARTIFICIAL INTELLIGENCE & TRACKING ---
            int bTargetX = pGridX; int bTargetY = pGridY;
            int pTargetX = pGridX + (pacman.dx * 4); int pTargetY = pGridY + (pacman.dy * 4);
            int iTargetX = pGridX - (pacman.dx * 2); int iTargetY = pGridY - (pacman.dy * 2);
            
            // Clyde Proximity AI Logic
            int gClydeTileX = (int)floorf(clyde.x / TILE_SIZE); 
            int gClydeTileY = (int)floorf(clyde.y / TILE_SIZE);
            float clydeDistance = std::hypot(gClydeTileX - pGridX, gClydeTileY - pGridY);
            int cTargetX = bTargetX; int cTargetY = bTargetY;
            if (clydeDistance < 8.0f) {
                cTargetX = 0; cTargetY = MAP_HEIGHT - 1; 
            }

            Ghost* ghostList[] = {&blinky, &pinky, &inky, &clyde};
            int targetXList[] = {bTargetX, pTargetX, iTargetX, cTargetX};
            int targetYList[] = {bTargetY, pTargetY, iTargetY, cTargetY};

            for(int g = 0; g < 4; g++) {
                Ghost* gh = ghostList[g];

                if (frightenedMode) {
                    gh->currentSpeed = gh->baseSpeed * 0.6f;
                } else {
                    if (g == 0) {
                        float pelletRatio = (float)totalPellets / (float)initialPelletCount;
                        if (pelletRatio < 0.3f) gh->currentSpeed = gh->baseSpeed * 1.6f;
                        else if (pelletRatio < 0.6f) gh->currentSpeed = gh->baseSpeed * 1.3f;
                        else gh->currentSpeed = gh->baseSpeed;
                    } else {
                        gh->currentSpeed = gh->baseSpeed;
                    }
                }

                // Math fix: Use floorf to handle negative coordinates at edges safely
                int gTileX = (int)floorf(gh->x / TILE_SIZE); 
                int gTileY = (int)floorf(gh->y / TILE_SIZE);
                bool gInLeftTunnel  = (gTileX < 1);
                bool gInRightTunnel = (gTileX >= MAP_WIDTH - 1);

                if (gInLeftTunnel || gInRightTunnel) {
                    if (gh->dx == 0) {
                        gh->dx = (gInLeftTunnel) ? -1 : 1; 
                        gh->dy = 0;
                    }
                } 
                else {
                    // Check if ghost is approaching an intersection tile closely
                    bool gAlignedX = ((int)gh->x % TILE_SIZE == 0);
                    bool gAlignedY = ((int)gh->y % TILE_SIZE == 0);

                    if (gAlignedX && gAlignedY) {
                        int dirsX[] = {0, 0, -1, 1}; int dirsY[] = {-1, 1, 0, 0};
                        int bestDirIdx = -1; float extremeDist = frightenedMode ? -1.0f : 999999.0f;

                        for (int i = 0; i < 4; i++) {
                            if (dirsX[i] == -gh->dx && dirsY[i] == -gh->dy) continue; 
                            int testTileX = gTileX + dirsX[i]; int testTileY = gTileY + dirsY[i];

                            if (testTileX >= 0 && testTileX < MAP_WIDTH && testTileY >= 0 && testTileY < MAP_HEIGHT) {
                                if (maze[testTileY][testTileX] != 1) { 
                                    float dist = std::hypot(testTileX - targetXList[g], testTileY - targetYList[g]);
                                    if (frightenedMode) {
                                        if (dist > extremeDist) { extremeDist = dist; bestDirIdx = i; }
                                    } else {
                                        if (dist < extremeDist) { extremeDist = dist; bestDirIdx = i; }
                                    }
                                }
                            }
                        }
                        if (bestDirIdx != -1) { 
                            gh->dx = dirsX[bestDirIdx]; 
                            gh->dy = dirsY[bestDirIdx]; 
                        }
                    }
                }

                // Apply safe physics steps
                gh->x += gh->dx * gh->currentSpeed;
                gh->y += gh->dy * gh->currentSpeed;

                // Anti-Clip Alignment Correction: Keep ghosts snapped perfectly inside hallways
                if (gh->dx != 0 && (int)gh->y % TILE_SIZE != 0) {
                    gh->y = roundf(gh->y / TILE_SIZE) * TILE_SIZE;
                }
                if (gh->dy != 0 && (int)gh->x % TILE_SIZE != 0) {
                    gh->x = roundf(gh->x / TILE_SIZE) * TILE_SIZE;
                }

                // Screen Teleport Wrap Triggers
                if (gh->x < -TILE_SIZE) {
                    gh->x = SCREEN_WIDTH - TILE_SIZE;
                    gh->y = ((int)(gh->y + TILE_SIZE / 2) / TILE_SIZE) * TILE_SIZE;
                } else if (gh->x >= SCREEN_WIDTH) {
                    gh->x = 0;
                    gh->y = ((int)(gh->y + TILE_SIZE / 2) / TILE_SIZE) * TILE_SIZE;
                }

                // Pacman and Ghost Collision check
                if (std::abs(pacman.x - gh->x) < 10 && std::abs(pacman.y - gh->y) < 10) {
                    if (frightenedMode) {
                        gh->x = 13 * TILE_SIZE; gh->y = 10 * TILE_SIZE; gh->dx = 0; gh->dy = -1;
                        score += 200;
                        PlaySynthTone(1000.0, 4000);
                        UpdateWindowTitle(window);
                    } else {
                        isDying = true; deathAnimFrame = 0;
                    }
                }
            }
        }

        // --- RENDER LAYERS ---
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (int row = 0; row < MAP_HEIGHT; ++row) {
            for (int col = 0; col < MAP_WIDTH; ++col) {
                SDL_Rect tileRect = { col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                if (maze[row][col] == 1) {
                    if (gameOver) SDL_SetRenderDrawColor(renderer, 80, 0, 0, 255);
                    else SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                    SDL_RenderFillRect(renderer, &tileRect);
                } else if (maze[row][col] == 0) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    DrawFilledCircle(renderer, col * TILE_SIZE + 8, row * TILE_SIZE + 8, 2);
                } else if (maze[row][col] == 3) {
                    if ((frameCounter / 10) % 2 == 0) {
                        SDL_SetRenderDrawColor(renderer, 255, 184, 151, 255);
                        DrawFilledCircle(renderer, col * TILE_SIZE + 8, row * TILE_SIZE + 8, 5);
                    }
                }
            }
        }

        if (cherrySpawned) {
            SDL_SetRenderDrawColor(renderer, 220, 20, 60, 255); 
            DrawFilledCircle(renderer, cherryTileX * TILE_SIZE + 5, cherryTileY * TILE_SIZE + 9, 3);
            DrawFilledCircle(renderer, cherryTileX * TILE_SIZE + 11, cherryTileY * TILE_SIZE + 11, 3);
            SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
            SDL_RenderDrawLine(renderer, cherryTileX * TILE_SIZE + 5, cherryTileY * TILE_SIZE + 9, cherryTileX * TILE_SIZE + 8, cherryTileY * TILE_SIZE + 3);
            SDL_RenderDrawLine(renderer, cherryTileX * TILE_SIZE + 11, cherryTileY * TILE_SIZE + 11, cherryTileX * TILE_SIZE + 8, cherryTileY * TILE_SIZE + 3);
        }

        if (!gameOver) {
            int pCenterX = (int)pacman.x + 8; int pCenterY = (int)pacman.y + 8;
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);

            if (isDying) {
                int radius = 8 - (deathAnimFrame / 4);
                if (radius < 1) radius = 1;
                DrawFilledCircle(renderer, pCenterX, pCenterY, radius);
            } else {
                bool mouthOpen = (frameCounter / 8) % 2 == 0;
                if (mouthOpen && (pacman.dx != 0 || pacman.dy != 0)) {
                    for (int w = 0; w < 16; w++) {
                        for (int h = 0; h < 16; h++) {
                            int dx = 8 - w; int dy = 8 - h;
                            if ((dx*dx + dy*dy) <= 64) {
                                bool skip = false;
                                if (pacman.dx > 0 && dx < 0 && std::abs(dy) < std::abs(dx)) skip = true;
                                else if (pacman.dx < 0 && dx > 0 && std::abs(dy) < std::abs(dx)) skip = true;
                                else if (pacman.dy > 0 && dy < 0 && std::abs(dx) < std::abs(dy)) skip = true;
                                else if (pacman.dy < 0 && dy > 0 && std::abs(dx) < std::abs(dy)) skip = true;
                                if (!skip) SDL_RenderDrawPoint(renderer, (int)pacman.x + w, (int)pacman.y + h);
                            }
                        }
                    }
                } else {
                    DrawFilledCircle(renderer, pCenterX, pCenterY, 8);
                }
            }
        }

        if (!isDying) {
            Ghost* renderGhosts[] = {&blinky, &pinky, &inky, &clyde};
            for (int g = 0; g < 4; g++) {
                int gx = (int)renderGhosts[g]->x; int gy = (int)renderGhosts[g]->y;

                if (frightenedMode) {
                    if ((frameCounter / 30) % 2 == 0) {
                        SDL_SetRenderDrawColor(renderer, 150, 0, 220, 255); 
                    } else {
                        SDL_SetRenderDrawColor(renderer, 230, 230, 255, 255); 
                    }
                } else {
                    SDL_SetRenderDrawColor(renderer, renderGhosts[g]->colorR, renderGhosts[g]->colorG, renderGhosts[g]->colorB, 255);
                }

                SDL_Rect bodyRect = { gx, gy + 8, 16, 8 };
                SDL_RenderFillRect(renderer, &bodyRect);
                DrawFilledCircle(renderer, gx + 8, gy + 8, 8);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); 
    }

    if (audioDevice) SDL_CloseAudioDevice(audioDevice);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
