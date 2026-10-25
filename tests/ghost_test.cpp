/*
 * Tests of the ghosts of HaikuPacman: they never enter a wall or leave the maze except through the tunnels, they leave the
 * ghost house, they are never fast, and the higher levels chase better. The game's own code is used (HaikuPacman.cpp is
 * included), with stand-ins for the Haiku headers (tests/stubs), so this runs on any computer with SDL2 and libcurl:
 *
 *   make test
 */
#define SDL_MAIN_HANDLED
#define main game_main
#include "../HaikuPacman.cpp"
#undef main

#include <cstdlib>
#include <cstdio>
#include <vector>

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { if (failures < 20) { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } failures++; } } while (0)

static std::vector<std::pair<int, int> > freeTiles;

static void StartLevel(int forLevel) {
    ResetGame(nullptr);
    level = forLevel;
    Ghost* all[] = {&blinky, &pinky, &inky, &clyde};
    for (Ghost* g : all) { g->baseSpeed = GhostBaseSpeed(level); g->currentSpeed = g->baseSpeed; }
    freeTiles.clear();
    for (int r = 0; r < MAP_HEIGHT; r++)
        for (int c = 0; c < MAP_WIDTH; c++)
            if (maze[r][c] != 1 && maze[r][c] != 2 && !(r == HOUSE_ROW && c >= HOUSE_LEFT && c <= HOUSE_RIGHT)) freeTiles.push_back({c, r});
}

static void PlacePacman(int tx, int ty, int dx, int dy) {
    pacman.x = tx * TILE_SIZE; pacman.y = ty * TILE_SIZE; pacman.dx = dx; pacman.dy = dy;
}

static int GridOf(float v) { return (int)v / TILE_SIZE; }

// The ghost's square overlaps no wall, and is inside the maze (up and down always, left and right only in the tunnels).
static bool GhostPlaceOk(const Ghost& g, char* why) {
    if (g.y < 0 || g.y > (MAP_HEIGHT - 1) * TILE_SIZE) { sprintf(why, "y %.1f is outside the maze", g.y); return false; }
    if (g.x < -TILE_SIZE || g.x >= SCREEN_WIDTH) { sprintf(why, "x %.1f is outside the screen", g.x); return false; }
    const int x0 = (int)floorf(g.x / TILE_SIZE), x1 = (int)floorf((g.x + TILE_SIZE - 1) / TILE_SIZE);
    const int y0 = (int)floorf(g.y / TILE_SIZE), y1 = (int)floorf((g.y + TILE_SIZE - 1) / TILE_SIZE);
    for (int ty = y0; ty <= y1; ty++)
        for (int tx = x0; tx <= x1; tx++)
            if (tx >= 0 && tx < MAP_WIDTH && maze[ty][tx] == 1) { sprintf(why, "at %.1f,%.1f it is in the wall tile %d,%d", g.x, g.y, tx, ty); return false; }
    return true;
}

static void TestNeverInWallsAndNeverFast() {
    srand(12345);
    for (int lv = 1; lv <= 9; lv++) {
        StartLevel(lv);
        Ghost* all[] = {&blinky, &pinky, &inky, &clyde};
        const char* names[] = {"Blinky", "Pinky", "Inky", "Clyde"};
        float lastX[4], lastY[4];
        for (int g = 0; g < 4; g++) { lastX[g] = all[g]->x; lastY[g] = all[g]->y; }
        int frightenedUntil = 0, seenOutside[4] = {0, 0, 0, 0};
        for (int frame = 0; frame < 150000; frame++) {
            if (frame % 90 == 0) {
                const auto t = freeTiles[rand() % freeTiles.size()];
                static const int dxs[] = {0, 0, -1, 1, 0}, dys[] = {-1, 1, 0, 0, 0};
                const int d = rand() % 5;
                PlacePacman(t.first, t.second, dxs[d], dys[d]);
            }
            if (frame % 700 == 0 && rand() % 2) { frightenedMode = true; frightenedUntil = frame + 360; }
            if (frightenedMode && frame >= frightenedUntil) frightenedMode = false;
            totalPellets = (frame / 5000) % 2 ? initialPelletCount / 5 : initialPelletCount;
            isDying = false;
            const int pGridX = ((int)pacman.x + TILE_SIZE / 2) / TILE_SIZE, pGridY = ((int)pacman.y + TILE_SIZE / 2) / TILE_SIZE;
            UpdateGhosts(nullptr, pGridX, pGridY);
            for (int g = 0; g < 4; g++) {
                char why[128];
                CHECK(GhostPlaceOk(*all[g], why), "level %d, %s, frame %d: %s", lv, names[g], frame, why);
                const float stepX = std::abs(all[g]->x - lastX[g]), stepY = std::abs(all[g]->y - lastY[g]);
                // (out at one tunnel end and in at the other, or eaten and back in the ghost house, are not steps)
                const bool wrapped = stepX > 100 || (frightenedMode && all[g]->x == 13 * TILE_SIZE && all[g]->y == 10 * TILE_SIZE);
                CHECK(wrapped || stepX + stepY <= 2.0f, "level %d, %s, frame %d: moved %.1f pixels", lv, names[g], frame, stepX + stepY);
                lastX[g] = all[g]->x; lastY[g] = all[g]->y;
                if (!InGhostHouse(GridOf(all[g]->x), GridOf(all[g]->y))) seenOutside[g] = 1;
            }
        }
        for (int g = 0; g < 4; g++) CHECK(seenOutside[g], "level %d: %s never left the ghost house", lv, names[g]);
    }
}

static void TestSpeeds() {
    printf("ghost speed by level (pixels a frame, Pacman is 2.0; Blinky's last stretch in brackets):\n ");
    float previous = 0;
    for (int lv = 1; lv <= 12; lv++) {
        const float s = GhostBaseSpeed(lv);
        printf(" %d: %.2f [%.2f]", lv, s, s * 1.2f);
        CHECK(s >= previous, "the speed fell at level %d", lv);
        CHECK(s * 1.2f < 1.7f, "level %d: Blinky's top speed %.2f is too close to Pacman's", lv, s * 1.2f);
        previous = s;
    }
    printf("\n");
}

// How long Blinky takes to catch Pacman standing still, from the ghost house, by level.
static double CatchFrames(int forLevel, int tx, int ty) {
    StartLevel(forLevel);
    PlacePacman(tx, ty, 0, 0);
    frightenedMode = false; isDying = false;
    for (int frame = 1; frame <= 6000; frame++) {
        UpdateGhosts(nullptr, tx, ty);
        if (isDying) return frame;
    }
    return 6000;
}

static void TestChase() {
    const int spots[][2] = {{1, 1}, {26, 1}, {1, 20}, {26, 20}, {6, 6}, {21, 6}, {6, 16}, {21, 16}, {12, 14}, {1, 8}, {26, 12}, {9, 18}};
    const int count = sizeof(spots) / sizeof(spots[0]);
    printf("frames until a ghost catches Pacman standing still (12 places, from the ghost house):\n");
    double mean[5] = {0, 0, 0, 0, 0};
    for (int lv = 1; lv <= 4; lv++) {
        double total = 0; int never = 0;
        for (int i = 0; i < count; i++) {
            const double f = CatchFrames(lv, spots[i][0], spots[i][1]);
            total += f; if (f >= 6000) never++;
        }
        mean[lv] = total / count;
        printf("  level %d: %6.0f frames on average, %d of %d never", lv, mean[lv], never, count);
        printf("\n");
        CHECK(never == 0, "level %d: a ghost never caught Pacman in %d places", lv, never);
    }
    CHECK(mean[2] <= mean[1], "level 2 (%.0f) does not chase better than level 1 (%.0f)", mean[2], mean[1]);
    CHECK(mean[3] <= mean[2], "level 3 (%.0f) does not chase better than level 2 (%.0f)", mean[3], mean[2]);
}

int main() {
    TestSpeeds();
    TestNeverInWallsAndNeverFast();
    TestChase();
    if (failures) { printf("%d checks failed\n", failures); return 1; }
    printf("all ghost tests passed\n");
    return 0;
}
