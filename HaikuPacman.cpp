/*
 * Copyright 2026, Kris Beazley (ablyss) HaikuPacman@epluribusunix.net
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>
#include <string>
#include <curl/curl.h>
#include <Notification.h>
#include <String.h>
#include <OS.h>
#include <stdio.h>

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
int level = 1;
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
    float moveBudget = 0.0f;                    // the part of a pixel that is owed to the ghost (see UpdateGhosts)
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

    // (all four start in the ghost house, a row of free tiles: the row under it is wall)
    blinky.x = 13 * TILE_SIZE; blinky.y = 10 * TILE_SIZE; blinky.dx = 0; blinky.dy = -1;
    pinky.x = 14 * TILE_SIZE; pinky.y = 10 * TILE_SIZE; pinky.dx = 0; pinky.dy = -1;
    inky.x = 12 * TILE_SIZE; inky.y = 10 * TILE_SIZE; inky.dx = 0; inky.dy = -1;
    clyde.x = 15 * TILE_SIZE; clyde.y = 10 * TILE_SIZE; clyde.dx = 0; clyde.dy = -1;
    blinky.moveBudget = pinky.moveBudget = inky.moveBudget = clyde.moveBudget = 0.0f;
    
    isDying = false; deathAnimFrame = 0;
}

void UpdateWindowTitle(SDL_Window* window) {
    std::string titleStr = "Score: " + std::to_string(score) + "  |  Lives: " + std::to_string(lives) + "  |  Level: " + std::to_string(level);
    if (frightenedMode) titleStr += "  [GHOSTS FRIGHTENED!]";
    if (cherrySpawned) titleStr += "  *CHERRY BONUS ACTIVE!*";
    if (gameOver) titleStr = "GAME OVER! Final Score: " + std::to_string(score) + " | Press SPACE to Restart";
    SDL_SetWindowTitle(window, titleStr.c_str());
}

// How fast the ghosts are: a little faster each level, but never near Pacman's 2 pixels a frame. What gets harder is how well they
// chase (see GhostTier), not how fast.
static float GhostBaseSpeed(int forLevel) {
    const float speed = 1.0f + 0.05f * (forLevel - 1);
    return speed > 1.3f ? 1.3f : speed;
}

void ResetGame(SDL_Window* window) {
    score = 0; lives = 3; level = 1; totalPellets = 0; gameOver = false; frightenedMode = false; frightenedTimer = 0; frameCounter = 0;
    cherrySpawned = false; cherryEaten = false;
    
    for(int r=0; r<MAP_HEIGHT; ++r) {
        for(int c=0; c<MAP_WIDTH; ++c) {
            maze[r][c] = initialMaze[r][c];
            if (maze[r][c] == 0 || maze[r][c] == 3) totalPellets++;
        }
    }
    initialPelletCount = totalPellets;

    blinky.baseSpeed = GhostBaseSpeed(level); blinky.currentSpeed = blinky.baseSpeed; blinky.colorR = 255; blinky.colorG = 0; blinky.colorB = 0;
    pinky.baseSpeed = GhostBaseSpeed(level); pinky.currentSpeed = pinky.baseSpeed; pinky.colorR = 255; pinky.colorG = 182; pinky.colorB = 193;
    inky.baseSpeed = GhostBaseSpeed(level); inky.currentSpeed = inky.baseSpeed; inky.colorR = 0; inky.colorG = 255; inky.colorB = 255;
    clyde.baseSpeed = GhostBaseSpeed(level); clyde.currentSpeed = clyde.baseSpeed; clyde.colorR = 255; clyde.colorG = 140; clyde.colorB = 0;

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

    level++;
    blinky.baseSpeed = pinky.baseSpeed = inky.baseSpeed = clyde.baseSpeed = GhostBaseSpeed(level);

    SoftResetPositions();
    PlaySynthTone(587.33, 15000); 
    UpdateWindowTitle(window);
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static int32 BackgroundUpdateChecker(void* data) {
    (void)data;
    const char* targetUrl = "https://raw.githubusercontent.com/ablyssx74/HaikuPacman/refs/heads/main/VERSION";
    // (the version comes from the VERSION file: the Makefile passes it in)
    const char* localVersion = "v" APP_LOCAL_VERSION;

    CURL* curl = curl_easy_init();
    if (!curl) return 0;

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, targetUrl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "HaikuPacman-UpdateChecker/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    BString remoteVersion(response.c_str());
    remoteVersion.Trim();

    // newer, not just different: "v1.0.4" is compared as the numbers 1, 0, 4
    int32 curMajor = 0, curMinor = 0, curRevision = 0, remMajor = 0, remMinor = 0, remRevision = 0;
    sscanf(localVersion, "%*[^0-9]%d.%d.%d", &curMajor, &curMinor, &curRevision);
    sscanf(remoteVersion.String(), "%*[^0-9]%d.%d.%d", &remMajor, &remMinor, &remRevision);
    const int32 currentFlattened = (curMajor * 10000) + (curMinor * 100) + curRevision;
    const int32 remoteFlattened = (remMajor * 10000) + (remMinor * 100) + remRevision;

    if (remoteVersion.Length() > 0 && remoteFlattened > currentFlattened) {
        BNotification notification(B_INFORMATION_NOTIFICATION);
        notification.SetGroup("HaikuPacman");
        notification.SetTitle("Update Available");
        BString content;
        content << "A newer version of HaikuPacman is available! (" << remoteVersion << ")";
        notification.SetContent(content);
        notification.Send();
    }

    return 0;
}

// ---- The ghosts ----
//
// A ghost moves in whole pixels: its speed (pixels per frame, with a fraction) is paid out one pixel at a time, so it is exactly
// on a tile now and then, and only there does it choose where to go next, among the tiles that are not walls. It cannot skip a
// tile, so it cannot go through a wall, and it cannot leave the maze except through the tunnels at the sides.
//
// The level makes them smarter, not faster (a ghost stays much slower than Pacman). They always take the shortest way round the
// walls to where they are going; what changes is how often they take a wrong turn and how well they work together:
//   level 1   a wrong turn at one corner in five
//   level 2   one in twelve
//   level 3   none, and they work together: Inky aims past Pacman from Blinky's side, and Clyde keeps away only when Pacman is
//             very close
//   level 4+  Clyde never backs off either
static const int HOUSE_ROW = 10, HOUSE_LEFT = 11, HOUSE_RIGHT = 16, DOOR_ROW = 9;

static bool InGhostHouse(int tileX, int tileY) {
    if (tileX < 0 || tileX >= MAP_WIDTH || tileY < 0 || tileY >= MAP_HEIGHT) return false;
    return (tileY == HOUSE_ROW && tileX >= HOUSE_LEFT && tileX <= HOUSE_RIGHT) || (tileY == DOOR_ROW && maze[tileY][tileX] == 2);
}

static int GhostTier() { return level < 4 ? level : 4; }
static int WrongTurnPercent(int tier) { return tier == 1 ? 20 : tier == 2 ? 8 : 0; }

// How many steps it takes to get from every tile to the target, round the walls (through the tunnels too); -1: cannot be reached.
static void BuildDistanceMap(int targetX, int targetY, int dist[MAP_HEIGHT][MAP_WIDTH]) {
    static int queueX[MAP_WIDTH * MAP_HEIGHT], queueY[MAP_WIDTH * MAP_HEIGHT];
    const int dirsX[] = {0, 0, -1, 1}, dirsY[] = {-1, 1, 0, 0};
    for (int r = 0; r < MAP_HEIGHT; r++)
        for (int c = 0; c < MAP_WIDTH; c++) dist[r][c] = -1;
    int head = 0, tail = 0;
    dist[targetY][targetX] = 0; queueX[tail] = targetX; queueY[tail] = targetY; tail++;
    while (head < tail) {
        const int x = queueX[head], y = queueY[head]; head++;
        for (int i = 0; i < 4; i++) {
            int nx = x + dirsX[i], ny = y + dirsY[i];
            if (nx < 0) nx = MAP_WIDTH - 1; else if (nx >= MAP_WIDTH) nx = 0;
            if (ny < 0 || ny >= MAP_HEIGHT) continue;
            if (maze[ny][nx] == 1 || dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[y][x] + 1; queueX[tail] = nx; queueY[tail] = ny; tail++;
        }
    }
}

// A target in the maze: one that is outside it, or in a wall (four tiles ahead of Pacman is often), becomes the nearest free tile.
static void ResolveTarget(int& targetX, int& targetY, int fallbackX, int fallbackY) {
    if (fallbackX < 0) fallbackX = 0; else if (fallbackX >= MAP_WIDTH) fallbackX = MAP_WIDTH - 1;
    if (fallbackY < 0) fallbackY = 0; else if (fallbackY >= MAP_HEIGHT) fallbackY = MAP_HEIGHT - 1;
    if (targetX < 0) targetX = 0; else if (targetX >= MAP_WIDTH) targetX = MAP_WIDTH - 1;
    if (targetY < 0) targetY = 0; else if (targetY >= MAP_HEIGHT) targetY = MAP_HEIGHT - 1;
    if (maze[targetY][targetX] != 1) return;
    float best = 1e9f; int bestX = fallbackX, bestY = fallbackY;
    for (int dy = -6; dy <= 6; dy++) {
        for (int dx = -6; dx <= 6; dx++) {
            const int nx = targetX + dx, ny = targetY + dy;
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT || maze[ny][nx] == 1) continue;
            const float d = std::hypot((float)dx, (float)dy);
            if (d < best) { best = d; bestX = nx; bestY = ny; }
        }
    }
    targetX = bestX; targetY = bestY;
}

// A ghost that is on a tile chooses its way: never into a wall, never back where it came from (unless it is in a dead end), and
// never into the ghost house from outside. Frightened, it goes as far from Pacman as it can.
static void ChooseGhostDirection(Ghost* gh, int tileX, int tileY, int targetX, int targetY, int pacTileX, int pacTileY) {
    const int dirsX[] = {0, 0, -1, 1}, dirsY[] = {-1, 1, 0, 0};
    const int tier = GhostTier();
    const bool inHouse = InGhostHouse(tileX, tileY);
    static int dist[MAP_HEIGHT][MAP_WIDTH];

    if (inHouse) {
        // out through the door, whatever else it has in mind
        targetX = (tileX <= 13) ? 13 : 14; targetY = 8;
    }
    const bool flee = frightenedMode && !inHouse;
    const bool useMap = !flee;
    if (useMap) BuildDistanceMap(targetX, targetY, dist);
    const bool wrongTurn = !flee && !inHouse && (rand() % 100) < WrongTurnPercent(tier);

    int bestDir = -1, choices = 0;
    for (int pass = 0; pass < 2 && bestDir < 0; pass++) {
        float bestScore = flee ? -1.0f : 1e9f;
        for (int i = 0; i < 4; i++) {
            if (pass == 0 && dirsX[i] == -gh->dx && dirsY[i] == -gh->dy) continue;
            int nx = tileX + dirsX[i]; const int ny = tileY + dirsY[i];
            if (nx < 0) nx = MAP_WIDTH - 1; else if (nx >= MAP_WIDTH) nx = 0;
            if (ny < 0 || ny >= MAP_HEIGHT || maze[ny][nx] == 1) continue;
            if (maze[ny][nx] == 2 && !inHouse) continue;
            float score;
            if (flee) score = std::hypot((float)(nx - pacTileX), (float)(ny - pacTileY));
            else { if (dist[ny][nx] < 0) continue; score = (float)dist[ny][nx]; }
            if (wrongTurn) {
                // a wrong turn: any way that is open, at random
                if (rand() % ++choices == 0) bestDir = i;
                continue;
            }
            if (flee ? (score > bestScore) : (score < bestScore)) { bestScore = score; bestDir = i; }
        }
    }
    if (bestDir >= 0) { gh->dx = dirsX[bestDir]; gh->dy = dirsY[bestDir]; }
    else { gh->dx = 0; gh->dy = 0; }
}

static void UpdateGhosts(SDL_Window* window, int pGridX, int pGridY) {
    const int tier = GhostTier();
    Ghost* ghostList[] = {&blinky, &pinky, &inky, &clyde};

    // where each one is going
    int targetXList[4], targetYList[4];
    targetXList[0] = pGridX; targetYList[0] = pGridY;                                         // Blinky: Pacman
    targetXList[1] = pGridX + pacman.dx * 4; targetYList[1] = pGridY + pacman.dy * 4;         // Pinky: ahead of Pacman
    if (tier >= 3) {
        // Inky: twice as far from Blinky as the tile two ahead of Pacman
        const int aheadX = pGridX + pacman.dx * 2, aheadY = pGridY + pacman.dy * 2;
        const int blinkyX = (int)floorf(blinky.x / TILE_SIZE), blinkyY = (int)floorf(blinky.y / TILE_SIZE);
        targetXList[2] = aheadX + (aheadX - blinkyX); targetYList[2] = aheadY + (aheadY - blinkyY);
    } else {
        targetXList[2] = pGridX - pacman.dx * 2; targetYList[2] = pGridY - pacman.dy * 2;     // Inky: behind Pacman
    }
    const float clydeDistance = std::hypot(floorf(clyde.x / TILE_SIZE) - pGridX, floorf(clyde.y / TILE_SIZE) - pGridY);
    const float clydeRetreat = (tier >= 4) ? 0.0f : (tier >= 3) ? 4.0f : 8.0f;
    if (clydeDistance < clydeRetreat) { targetXList[3] = 0; targetYList[3] = MAP_HEIGHT - 1; }   // Clyde: the corner
    else { targetXList[3] = pGridX; targetYList[3] = pGridY; }
    for (int g = 0; g < 4; g++) ResolveTarget(targetXList[g], targetYList[g], pGridX, pGridY);

    for (int g = 0; g < 4; g++) {
        Ghost* gh = ghostList[g];

        if (frightenedMode) {
            gh->currentSpeed = gh->baseSpeed * 0.6f;
        } else if (g == 0) {
            // Blinky gets a little faster as the pellets run out
            const float pelletRatio = (float)totalPellets / (float)initialPelletCount;
            if (pelletRatio < 0.3f) gh->currentSpeed = gh->baseSpeed * 1.2f;
            else if (pelletRatio < 0.6f) gh->currentSpeed = gh->baseSpeed * 1.1f;
            else gh->currentSpeed = gh->baseSpeed;
        } else {
            gh->currentSpeed = gh->baseSpeed;
        }

        gh->moveBudget += gh->currentSpeed;
        while (gh->moveBudget >= 1.0f) {
            gh->moveBudget -= 1.0f;

            // on a tile: choose the way (in the tunnels at the sides it just goes on)
            if (((int)gh->x % TILE_SIZE == 0) && ((int)gh->y % TILE_SIZE == 0)) {
                const int tileX = (int)gh->x / TILE_SIZE, tileY = (int)gh->y / TILE_SIZE;
                if (tileX >= 1 && tileX < MAP_WIDTH - 1)
                    ChooseGhostDirection(gh, tileX, tileY, targetXList[g], targetYList[g], pGridX, pGridY);
                else if (gh->dx == 0)
                    gh->dx = (tileX < 1) ? -1 : 1;
            }

            gh->x += gh->dx; gh->y += gh->dy;

            // the tunnels: out at one side, in at the other
            if (gh->x < -TILE_SIZE) gh->x = SCREEN_WIDTH - TILE_SIZE;
            else if (gh->x >= SCREEN_WIDTH) gh->x = 0;
        }

        // Pacman and Ghost Collision check
        if (std::abs(pacman.x - gh->x) < 10 && std::abs(pacman.y - gh->y) < 10) {
            if (frightenedMode) {
                gh->x = 13 * TILE_SIZE; gh->y = 10 * TILE_SIZE; gh->dx = 0; gh->dy = -1; gh->moveBudget = 0.0f;
                score += 200;
                PlaySynthTone(1000.0, 4000);
                UpdateWindowTitle(window);
            } else {
                isDying = true; deathAnimFrame = 0;
            }
        }
    }
}

int main() {
    // libcurl's global init is not thread-safe against other concurrently running
    // threads, so do it explicitly up front to avoid an implicit lazy global init
    // racing with the background update-checker thread started below.
    curl_global_init(CURL_GLOBAL_DEFAULT);

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
    
    // Update Checker
    {
        thread_id updateThread = spawn_thread(BackgroundUpdateChecker, "haikupacman_update_checker", B_LOW_PRIORITY, nullptr);
        if (updateThread >= 0) resume_thread(updateThread);
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

            UpdateGhosts(window, pGridX, pGridY);
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
    curl_global_cleanup();
    return 0;
}
