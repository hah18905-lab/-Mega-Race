// =====================================================================
//  MEGA RACE 3D : TURBO EDITION
//  Street racing + kart racing with transforming vehicles.
//  Car / tank / robot / kart -> boat on water -> plane in the air.
//  Items, coins, drift mini-turbo, traffic, police chases, career.
//  C++17 + raylib 5.x, one source file.
//
//  Build (Windows, w64devkit / MinGW):
//     g++ main.cpp -o MegaRace.exe -O2 -std=c++17 -lraylib -lopengl32 -lgdi32 -lwinmm
//  Visual Studio: open the folder with CMakeLists.txt (raylib downloads itself).
//  Linux:  g++ main.cpp -o race -O2 -std=c++17 -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
// =====================================================================
#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#include "rlgl.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

static int SW = 1280, SH = 720;
static int vpW = 1280, vpH = 720;      // pixel size of the viewport being rendered (split screen / render scale)
static float hudScale = 1.0f;          // HUD virtual-unit scale inside a viewport
static const float PI_F = 3.14159265358979f;

typedef Vector3 V3;
static inline V3 vec(float x, float y, float z) { V3 r; r.x = x; r.y = y; r.z = z; return r; }
static inline Vector2 v2(float x, float y) { Vector2 r; r.x = x; r.y = y; return r; }
static inline V3 add(V3 a, V3 b) { return vec(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline V3 sub(V3 a, V3 b) { return vec(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline V3 mul(V3 a, float s) { return vec(a.x * s, a.y * s, a.z * s); }
static inline float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float len(V3 a) { return sqrtf(dot(a, a)); }
static inline V3 norm(V3 a) { float l = len(a); return l > 1e-6f ? mul(a, 1.0f / l) : vec(0, 0, 1); }
static inline V3 cross(V3 a, V3 b) { return vec(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static inline float sgn(float v) { return v < 0 ? -1.0f : 1.0f; }
static inline float dist2D(V3 a, V3 b) { float dx = a.x - b.x, dz = a.z - b.z; return sqrtf(dx * dx + dz * dz); }
static float wrapA(float a) { while (a > PI_F) a -= 2 * PI_F; while (a < -PI_F) a += 2 * PI_F; return a; }
static float frand(float a, float b) { return a + (b - a) * (float)GetRandomValue(0, 10000) / 10000.0f; }
static float smooth01(float t) { t = clampf(t, 0, 1); return t * t * (3 - 2 * t); }
static Color C(int r, int g, int b, int a = 255) {
    Color c; c.r = (unsigned char)clampf((float)r, 0, 255); c.g = (unsigned char)clampf((float)g, 0, 255);
    c.b = (unsigned char)clampf((float)b, 0, 255); c.a = (unsigned char)clampf((float)a, 0, 255); return c;
}
static Color mixC(Color a, Color b, float t) {
    t = clampf(t, 0, 1);
    return C((int)lerpf(a.r, b.r, t), (int)lerpf(a.g, b.g, t), (int)lerpf(a.b, b.b, t), (int)lerpf(a.a, b.a, t));
}
static Color shade(Color c, float k) { return C((int)(c.r * k), (int)(c.g * k), (int)(c.b * k), c.a); }
static Color alpha(Color c, float a) { return C(c.r, c.g, c.b, (int)(255.0f * clampf(a, 0, 1))); }
static Color mulC(Color a, Color b) { return C(a.r * b.r / 255, a.g * b.g / 255, a.b * b.b / 255, a.a * b.a / 255); }
static Color balloonCol(int k) { static const Color bc[3] = {{255, 60, 70, 255}, {255, 210, 40, 255}, {60, 140, 255, 255}}; return bc[((k % 3) + 3) % 3]; }
static float hash1(int x) { unsigned h = (unsigned)x * 374761393u; h = (h ^ (h >> 13)) * 1274126177u; return (float)(h & 0xFFFF) / 65535.0f; }

// ------------------------------ language -----------------------------
static bool cyrOK = false;
static int langSel = 0;          // 0 = Russian, 1 = English
static const char* T(const char* ru, const char* en) { return (cyrOK && langSel == 0) ? ru : en; }

// ------------------------------ enums --------------------------------
enum Base { B_CAR = 0, B_TANK, B_ROBOT, B_KART, B_COUNT };
enum Mode { M_LAND = 0, M_WATER, M_AIR };
enum Theme { TH_CITY = 0, TH_HIGHWAY, TH_COUNTRY, TH_HARBOR, TH_RIVER, TH_CANYON, TH_BAY, TH_TUNNEL, TH_BEACH, TH_LAGOON, TH_BACKROOMS, TH_POOL,
             TH_SNOW, TH_ICE, TH_PEAKS, TH_FJORD,            // frost pass
             TH_DESERT, TH_OASIS, TH_VOLCANO, TH_CRATER,     // volcano
             TH_STATION, TH_COOLANT, TH_VOID };              // orbital station
enum Weather { W_CLEAR = 0, W_RAIN, W_FOG, W_SNOW, W_STORM, W_COUNT };
enum TimeOfDay { TD_NIGHT = 0, TD_SUNSET, TD_DAY };
enum GameMode { GM_RACE = 0, GM_TIMETRIAL, GM_ELIM, GM_PURSUIT, GM_BATTLE, GM_COUNT };
enum Item { IT_NONE = 0, IT_TURBO, IT_ROCKET, IT_OIL, IT_SHIELD, IT_BOMB, IT_LIGHTNING, IT_COUNT };
static const int NUM_TRACKS = 6;

static const char* baseName(Base b) {
    switch (b) {
        case B_CAR: return T("\320\241\320\237\320\236\320\240\320\242\320\232\320\220\320\240", "SPORTS CAR");
        case B_TANK: return T("\320\242\320\220\320\235\320\232", "TANK");
        case B_ROBOT: return T("\320\240\320\236\320\221\320\236\320\242", "ROBOT");
        default: return T("\320\232\320\220\320\240\320\242", "KART");
    }
}
static bool spaceNames = false;      // set while the orbit track is loaded
static const char* modeName(Base b, Mode m) {
    if (spaceNames) {
        if (m == M_AIR) return T("\320\227\320\222\320\225\320\227\320\224\320\236\320\233\320\201\320\242", "STARSHIP");
        if (m == M_WATER) return T("\320\237\320\233\320\220\320\227\320\234\320\236-\320\232\320\220\320\242\320\225\320\240", "PLASMA SKIFF");
        return b == B_KART ? T("\320\245\320\236\320\222\320\225\320\240-\320\232\320\220\320\240\320\242", "HOVER KART") : T("\320\245\320\236\320\222\320\225\320\240\320\232\320\220\320\240", "HOVERCAR");
    }
    if (m == M_AIR) return T("\320\241\320\220\320\234\320\236\320\233\320\201\320\242", "PLANE");
    if (m == M_WATER) return b == B_TANK ? T("\320\232\320\220\320\235\320\236\320\235\320\225\320\240\320\232\320\220", "GUNBOAT") : b == B_ROBOT ? T("\320\223\320\230\320\224\320\240\320\236\320\221\320\236\320\242", "HYDROBOT") : b == B_KART ? T("\320\223\320\230\320\224\320\240\320\236\320\246\320\230\320\232\320\233", "JET SKI") : T("\320\232\320\220\320\242\320\225\320\240", "SPEEDBOAT");
    return baseName(b);
}
static const char* weatherName(int w) {
    switch (w) {
        case W_RAIN: return T("\320\224\320\236\320\226\320\224\320\254", "RAIN");
        case W_FOG: return T("\320\242\320\243\320\234\320\220\320\235", "FOG");
        case W_SNOW: return T("\320\241\320\235\320\225\320\223", "SNOW");
        case W_STORM: return T("\320\223\320\240\320\236\320\227\320\220", "STORM");
        default: return T("\320\257\320\241\320\235\320\236", "CLEAR");
    }
}
static const char* gameModeName(int m) {
    switch (m) {
        case GM_TIMETRIAL: return T("\320\235\320\220 \320\222\320\240\320\225\320\234\320\257", "TIME TRIAL");
        case GM_ELIM: return T("\320\235\320\220 \320\222\320\253\320\221\320\253\320\222\320\220\320\235\320\230\320\225", "ELIMINATION");
        case GM_PURSUIT: return T("\320\237\320\236\320\223\320\236\320\235\320\257", "PURSUIT");
        case GM_BATTLE: return T("\320\221\320\230\320\242\320\222\320\220", "BATTLE");
        default: return T("\320\223\320\236\320\235\320\232\320\220", "RACE");
    }
}
static const char* trackName(int t) {
    switch (t) {
        case 1: return T("\320\241\320\236\320\233\320\235\320\225\320\247\320\235\320\253\320\231 \320\237\320\233\320\257\320\226", "SUNNY BEACH");
        case 2: return T("\320\227\320\220\320\232\320\243\320\233\320\230\320\241\320\254\320\225", "THE BACKROOMS");
        case 3: return T("\320\233\320\225\320\224\320\257\320\235\320\236\320\231 \320\237\320\225\320\240\320\225\320\222\320\220\320\233", "FROST PASS");
        case 4: return T("\320\222\320\243\320\233\320\232\320\220\320\235", "VOLCANO");
        case 5: return T("\320\236\320\240\320\221\320\230\320\242\320\220", "ORBIT STATION");
        default: return T("\320\235\320\225\320\236\320\235\320\236\320\222\320\253\320\231 \320\223\320\236\320\240\320\236\320\224", "NEON CITY");
    }
}
static const char* itemName(int it) {
    switch (it) {
        case IT_TURBO: return T("\320\242\320\243\320\240\320\221\320\236", "TURBO");
        case IT_ROCKET: return T("\320\240\320\220\320\232\320\225\320\242\320\220", "ROCKET");
        case IT_OIL: return T("\320\234\320\220\320\241\320\233\320\236", "OIL");
        case IT_SHIELD: return T("\320\251\320\230\320\242", "SHIELD");
        case IT_BOMB: return T("\320\234\320\230\320\235\320\220", "MINE");
        case IT_LIGHTNING: return T("\320\234\320\236\320\233\320\235\320\230\320\257", "LIGHTNING");
        default: return "";
    }
}

struct Stats { float vmax, acc, turn, lat, mass, rad; };
//                    LAND                         WATER                          AIR
static const Stats ST[4][3] = {
    {{60, 14, 1.9f, 8.0f, 1.2f, 2.4f}, {48, 13, 1.45f, 1.7f, 1.2f, 2.7f}, {80, 30, 1.35f, 0, 1.2f, 3.2f}},   // car
    {{53, 13, 1.45f, 11, 4.0f, 3.4f},  {45, 12, 1.0f, 2.6f, 4.0f, 3.6f},  {74, 26, 1.05f, 0, 4.0f, 4.2f}},  // tank
    {{54, 20, 2.5f, 9, 1.6f, 2.0f},    {46, 18, 2.1f, 3.2f, 1.6f, 2.3f},  {76, 28, 1.5f, 0, 1.6f, 3.0f}},   // robot
    {{57, 16, 2.25f, 9.5f, 0.9f, 1.9f}, {47, 14, 1.8f, 2.0f, 0.9f, 2.2f}, {76, 30, 1.6f, 0, 0.9f, 2.8f}}};  // kart
// garage stat bars: speed, acceleration, handling, armor
static const float GSTAT[4][4] = {{0.95f, 0.7f, 0.75f, 0.4f}, {0.75f, 0.55f, 0.55f, 1.0f}, {0.8f, 0.95f, 0.9f, 0.6f}, {0.85f, 0.8f, 1.0f, 0.3f}};
static const int VPRICE[4] = {0, 6000, 3500, 0};
static const int UPCOST[5] = {600, 1300, 2400, 3800, 5500};
static const int NPAINT = 14, NPAINT_FREE = 12;
static const Color PAINT[NPAINT] = {{220, 30, 40, 255}, {30, 110, 240, 255}, {250, 200, 20, 255}, {40, 200, 90, 255},
                               {255, 120, 10, 255}, {150, 60, 230, 255}, {235, 235, 240, 255}, {40, 42, 48, 255},
                               {255, 90, 170, 255}, {0, 200, 210, 255}, {140, 230, 40, 255}, {120, 70, 40, 255},
                               {255, 196, 40, 255}, {200, 225, 245, 255}};           // 12 gold, 13 ice chrome (boss rewards)
static const int NDECAL = 8, NDRIVER = 6;    // decals: 0 none .. 6 magma, 7 crown (boss rewards)
static const char* decalName(int d) {
    switch (d) {
        case 1: return T("\320\237\320\236\320\233\320\236\320\241\320\253", "STRIPES");
        case 2: return T("\320\237\320\233\320\220\320\234\320\257", "FLAMES");
        case 3: return T("\320\235\320\236\320\234\320\225\320\240", "NUMBER");
        case 4: return T("\320\250\320\220\320\245\320\234\320\220\320\242\320\232\320\220", "CHECKER");
        case 5: return T("\320\234\320\236\320\233\320\235\320\230\320\257", "BOLT");
        case 6: return T("\320\234\320\220\320\223\320\234\320\220", "MAGMA");
        case 7: return T("\320\232\320\236\320\240\320\236\320\235\320\220", "CROWN");
        default: return T("\320\235\320\225\320\242", "NONE");
    }
}
static const char* paintName(int p) {
    if (p == 12) return T("\320\227\320\236\320\233\320\236\320\242\320\236", "GOLD");
    if (p == 13) return T("\320\233\320\225\320\224\320\257\320\235\320\236\320\231 \320\245\320\240\320\236\320\234", "ICE CHROME");
    return "";
}
struct DriverDef { const char* ru; const char* en; Color helmet; int style; };
static const DriverDef DRIVERS[NDRIVER] = {{"\320\234\320\220\320\232\320\241", "MAX", {220, 40, 40, 255}, 0}, {"\320\233\320\230\320\241\320\220", "FOXY", {255, 130, 30, 255}, 1},
                                           {"\320\221\320\236\320\233\320\242", "BOLT", {170, 175, 185, 255}, 2}, {"\320\227\320\236\320\230", "ZOE", {255, 110, 190, 255}, 3},
                                           {"\320\242\320\236\320\240\320\235", "THORN", {70, 160, 70, 255}, 4}, {"\320\227\320\225\320\242", "ZED", {150, 90, 240, 255}, 5}};
static const char* driverName(int d) { d = ((d % NDRIVER) + NDRIVER) % NDRIVER; return T(DRIVERS[d].ru, DRIVERS[d].en); }

struct Input { float throttle = 0, steer = 0, strafe = 0; bool boost = false, special = false, special2 = false, item = false; };

struct Racer {
    int id = 0; bool isPlayer = false, isCop = false, parked = false, out = false;
    Base base = B_CAR; Mode mode = M_LAND;
    V3 pos{0, 0, 0}, vel{0, 0, 0};
    float yaw = 0, pitch = 0, roll = 0, steerV = 0, airSpeed = 40;
    float hp = 100, energy = 100; bool boostLock = false, boosting = false;
    float specialCd = 0, special2Cd = 0, switchCd = 0, hitCd = 0, invuln = 0, deadTimer = 0, padCd = 0;
    float morphAir = 0, morphWater = 0, wheelRot = 0, walk = 0, accelL = 0, slip = 0;
    float rpm = 0, gearShiftT = 0; int gear = 1;
    bool dead = false, grounded = true;
    int lastIdx = 0, lap = 0, checkMask = 7; bool finished = false; int finishPlace = 0;
    float finishTime = 0, lapStart = 0, bestLap = 0, lastLap = 0, wrongT = 0, scrapeT = 0, hurtT = 0;
    Color color = WHITE, helmet = WHITE, color2 = WHITE; std::string name;
    int pid = -1;                 // 0 / 1 for human players, -1 for AI and police
    int decal = 0, driverStyle = 0, number = 0;
    float lane = 0, aiTimer = 20, aiLaneT = 0, stuckTimer = 0, aggro = 0.5f, skill = 1.0f, rubber = 1.0f;
    Input in; bool prevSpecial = false, prevItem = false;
    float distCenter = 0, lateral = 0; int rings = 0;
    std::vector<char> ringHit;
    // kart-racing layer
    int item = IT_NONE, itemCount = 0; float rouletteT = 0, itemHoldT = 0;
    float shieldT = 0, spinT = 0, spinAng = 0, shrinkT = 0, boostT = 0, regenT = 0;
    float driftCharge = 0; int driftDir = 0;
    int coins = 0, coinsTotal = 0, takedowns = 0;
    float trickT = 0, airTime = 0, squashT = 0, lastGround = 0; bool trickDone = false;
    int lastHitBy = -1; float lastHitT = -10;
    // slipstream, battle, bosses, shortcuts, per-race stats
    float draftT = 0, draftK = 0; int balloons = 0, points = 0;
    int boss = 0; float bossT = 8;
    int scIdx = -1;
    int deaths = 0, ultraTurbos = 0, tricks = 0, drafts = 0;
    float startBoostT = 0, popCd = 0;
    // upgrades (player only)
    float upV = 1, upA = 1, upN = 1, upArmor = 1, upGrip = 1;
};
struct Shell { V3 p, v; float life; int owner; };
struct Particle { V3 p, v; float life, maxLife, size; Color c; bool grav, add; };
struct Pad { V3 p; float yaw; };
struct Ring { V3 p; V3 tang; int idx; };
struct Tree { V3 p; float s; int kind; bool solid; };      // kind: 0 round, 1 pine, 2 palm, 3 backrooms pillar, 4 cactus, 5 snowy pine
struct Rock { V3 p; float w, h, d, yaw; int seed; Color c; };
struct Bld { V3 p; float yaw, w, d, h; int tex; Color tint; int neon; Color neonC; bool antenna; int side; };
struct Lamp { V3 base, head; Color c; int type; float armYaw; int id; };
struct Box { V3 p; float yaw, w, h, d; Color c; int tex; };
struct Crane { V3 p; float yaw, h; };
struct Board { V3 p; float yaw; Color c; int seed; };
struct Deco { V3 p; float yaw, s; int type; Color c; };        // 0 umbrella, 1 hut, 2 surfboard, 3 tower, 4 ball, 5 asteroid, 6 ice floe, 7 snowman, 8 station module, 9 lava vent, 10 skull rock

struct Settings {
    int laps = 2, opponents = 5, difficulty = 1, tod = TD_NIGHT, weather = 0, sound = 1, music = 6, post = 1, paint = 0;
    Base base = B_CAR; bool fullscreen = false;
    int mode = GM_RACE, track = 0, items = 1, traffic = 1;
    // v4
    int players = 1; Base base2 = B_KART;
    int paint2 = 6, decal = 1, driver = 0, driver2 = 1;
    int quality = 2, vibration = 1, musicSrc = 0, showFps = 0;
};
static Settings S;
static const int CAREER_MAX = 32;
struct Profile {
    int money = 1500; int upg[4] = {0, 0, 0, 0}; int owned[4] = {1, 0, 0, 1};
    int career[CAREER_MAX] = {0}; int backrooms = 0;
    float bestLap[NUM_TRACKS] = {0}, bestRace[NUM_TRACKS] = {0};
    int totalTakedowns = 0, totalCoins = 0;
    // v4: achievements, unlocks, lifetime stats, daily challenge
    unsigned long long ach = 0;
    int bossMask = 0, wins = 0, tricks = 0, rings = 0, ultras = 0, drafts = 0, walls = 0, dailyDone = 0, dailyDay = -1, earned = 0;
    int battles = 0, escapes = 0, races2p = 0;
};
static Profile P;

// ------------------------------ global state -------------------------
static int N = 0, curTrack = 0;
static std::vector<V3> track, tanv, norv;
static std::vector<float> curv, hw, lim, airAlt, sArc, th, outerW;
static std::vector<Mode> zmode;
static std::vector<Theme> theme;
static std::vector<int> gateIdx;
static float trackLen = 0;
static std::vector<Tree> trees;
static std::vector<Rock> rocks;
static std::vector<Bld> blds;
static std::vector<Lamp> lamps;
static std::vector<Box> boxes;
static std::vector<Crane> cranes;
static std::vector<Board> boards;
static std::vector<Deco> decos;
static std::vector<Pad> pads;
static std::vector<Ring> rings;
static float mapMinX, mapMaxX, mapMinZ, mapMaxZ;

static std::vector<Racer> racers;
static std::vector<Shell> shells;
static std::vector<Particle> parts;
static std::vector<int> order;

static Weather curWeather = W_CLEAR;
static float wI = 1.0f, weatherTimer = 40, flashT = 0, thunderT = -1;
static float gTime = 0, raceTime = 0, countdown = 4.0f, shakeAmt = 0;
static float shakeP[2] = {0, 0};          // per human player (split screen)
static int numHumans = 1;                 // 1 or 2 human players in this race
static int vpPlayer = 0;                  // racer id whose viewport is being drawn
static int finishCount = 0; static bool raceOver = false; static float finishBannerT = 0;
static int numRacers = 6;
static int careerEvent = -1;
static float slowmoT = 0, takedownBannerT = 0; static int takedownVictim = -1;
static std::string takedownName;
// mode state
static float elimT = 30, pursuitT = 0, pursuitGoal = 150, heat = 1, bustedT = 0, copSpawnT = 0;
static int resultState = 0;          // 0 running, 1 won, 2 lost (busted / eliminated / time)
static float ttTarget = 0;

// lighting / atmosphere
static Color skyTop, skyHor, fogCol, ambCol, sunCol;
static float fogS = 200, fogE = 900, waveAmp = 0.6f, nightK = 1.0f, wetK = 0.0f, snowK = 0.0f;
static V3 lightDir = {-0.4f, 0.8f, -0.3f};
static V3 camPos = {0, 0, 0}, camFwd = {0, 0, 1};

// ------------------------------ notifications ------------------------
struct Note { std::string text; float t; Color c; int owner; };
static std::vector<Note> notes;
static void notify(const std::string& s, Color c = C(255, 255, 255), float t = 2.6f, int owner = -1) {
    for (auto& n : notes) if (n.text == s && n.owner == owner) { n.t = t; return; }
    notes.push_back({s, t, c, owner});
    if (notes.size() > 4) notes.erase(notes.begin());
}
static float hintT = 0;

// ------------------------------ particles ----------------------------
static int partCap = 1800;
static void emit(V3 p, V3 v, float life, float size, Color c, bool grav = false, bool additive = false) {
    if ((int)parts.size() > partCap) return;
    Particle q; q.p = p; q.v = v; q.life = life; q.maxLife = life; q.size = size; q.c = c; q.grav = grav; q.add = additive;
    parts.push_back(q);
}
static void burst(V3 p, int n, float spd, Color c, float life, float size, bool grav = true, bool additive = false) {
    for (int i = 0; i < n; i++) {
        V3 d = norm(vec(frand(-1, 1), frand(-0.2f, 1), frand(-1, 1)));
        emit(p, mul(d, frand(0.3f, 1.0f) * spd), frand(0.5f, 1.0f) * life, size * frand(0.5f, 1.0f), c, grav, additive);
    }
}

// ------------------------------ fonts & text -------------------------
static Font fBold, fItal;
static bool fontsLoaded = false;
static Font tryFont(const char* const* names, int count, int size, int* cps, int ncp, bool& ok) {
    const char* dirs[2] = {"", GetApplicationDirectory()};
    for (int d = 0; d < 2; d++)
        for (int i = 0; i < count; i++) {
            std::string p = std::string(dirs[d]) + names[i];
            if (FileExists(p.c_str())) {
                Font f = LoadFontEx(p.c_str(), size, cps, ncp);
                if (f.texture.id != 0 && f.glyphCount > 100) {
                    GenTextureMipmaps(&f.texture);
                    SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);
                    ok = true; return f;
                }
            }
        }
    ok = false; return GetFontDefault();
}
static void loadFonts() {
    std::vector<int> cps;
    for (int c = 32; c < 127; c++) cps.push_back(c);
    int extra[] = {0x401, 0x451, 0xB0, 0x2014, 0xAB, 0xBB, 0x2116};
    for (int e : extra) cps.push_back(e);
    for (int c = 0x410; c < 0x450; c++) cps.push_back(c);
    const char* italNames[] = {"assets/font_italic.ttf", "C:/Windows/Fonts/arialbi.ttf", "C:/Windows/Fonts/segoeuiz.ttf",
                               "/usr/share/fonts/truetype/dejavu/DejaVuSans-BoldOblique.ttf", "/System/Library/Fonts/Supplemental/Arial Bold Italic.ttf"};
    const char* boldNames[] = {"assets/font_bold.ttf", "C:/Windows/Fonts/arialbd.ttf", "C:/Windows/Fonts/segoeuib.ttf",
                               "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
                               "/System/Library/Fonts/Supplemental/Arial Bold.ttf"};
    bool ok1 = false, ok2 = false;
    fItal = tryFont(italNames, 5, 96, cps.data(), (int)cps.size(), ok1);
    fBold = tryFont(boldNames, 6, 40, cps.data(), (int)cps.size(), ok2);
    if (!ok1 && ok2) fItal = fBold;
    if (ok1 && !ok2) fBold = fItal;
    fontsLoaded = ok1 || ok2;
    cyrOK = fontsLoaded;
    langSel = cyrOK ? 0 : 1;
}
static Font& fnt(int style) { return style ? fItal : fBold; }
static float spacingFor(int style, float sz) { return fontsLoaded ? sz * (style ? 0.0f : 0.02f) : sz / 10.0f; }
static float txtW(const char* s, float sz, int style = 0) { return MeasureTextEx(fnt(style), s, sz, spacingFor(style, sz)).x; }
static void txt(const char* s, float x, float y, float sz, Color c, int style = 0) { DrawTextEx(fnt(style), s, v2(x, y), sz, spacingFor(style, sz), c); }
static void txtS(const char* s, float x, float y, float sz, Color c, int style = 0) {
    DrawTextEx(fnt(style), s, v2(x + sz * 0.05f + 1, y + sz * 0.05f + 1), sz, spacingFor(style, sz), C(0, 0, 0, (int)(c.a * 0.6f)));
    DrawTextEx(fnt(style), s, v2(x, y), sz, spacingFor(style, sz), c);
}
static void txtC(const char* s, float cx, float y, float sz, Color c, int style = 0, bool shadow = true) {
    float w = txtW(s, sz, style);
    if (shadow) txtS(s, cx - w / 2, y, sz, c, style); else txt(s, cx - w / 2, y, sz, c, style);
}

// =====================================================================
//                     PROCEDURAL TEXTURES
// =====================================================================
static Texture2D texWhite, texBeam, texAsphalt, texConcrete, texGrass, texSand, texWater, texGlow, texContainer, texDirt;
static Texture2D texSandRoad, texWallpaper, texCarpet, texCeiling, texTiles, texWood, texStripes;
static Texture2D texSnow, texIce, texBasalt, texLava, texPlate, texTankSign, texDeckRoad;
static Texture2D texFacade[3], texBoard[3];
static int facadeTod = -1;

static Texture2D finishTex(Image& img, bool repeat = true) {
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(t, repeat ? TEXTURE_WRAP_REPEAT : TEXTURE_WRAP_CLAMP);
    return t;
}
static Image noiseImage(int w, int h, Color base, int amp, int seed) {
    Image img = GenImageColor(w, h, base);
    Color* px = (Color*)img.data;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float n = hash1(x * 7919 + y * 104729 + seed) * 0.6f + hash1((x / 4) * 31 + (y / 4) * 1031 + seed * 3) * 0.4f;
            int d = (int)((n - 0.5f) * 2 * amp);
            px[y * w + x] = C(base.r + d, base.g + d, base.b + d, 255);
        }
    return img;
}
static void genStaticTextures() {
    texWhite.id = rlGetTextureIdDefault(); texWhite.width = 1; texWhite.height = 1; texWhite.mipmaps = 1; texWhite.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    {   // asphalt: u across the road, v along (24 m)
        Image img = noiseImage(256, 512, C(78, 80, 86), 14, 11);
        Color* px = (Color*)img.data;
        auto paint = [&](int x0, int x1, int y0, int y1, Color c) {
            for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) px[y * 256 + x] = mixC(px[y * 256 + x], c, 0.85f + 0.15f * hash1(x * 13 + y * 7));
        };
        paint(10, 16, 0, 512, C(235, 235, 230)); paint(240, 246, 0, 512, C(235, 235, 230));
        for (int k = 1; k < 4; k++) paint(k * 64 - 2, k * 64 + 2, 0, 230, C(235, 235, 230));
        for (int y = 0; y < 512; y++) for (int x = 0; x < 256; x++)
            if ((x > 40 && x < 56) || (x > 104 && x < 120) || (x > 136 && x < 152) || (x > 200 && x < 216)) px[y * 256 + x] = shade(px[y * 256 + x], 0.88f);
        texAsphalt = finishTex(img);
    }
    {   // packed-sand beach road with tyre ruts
        Image img = noiseImage(256, 512, C(214, 184, 132), 16, 13);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 512; y++) for (int x = 0; x < 256; x++) {
            float rut = 0;
            int ruts[4] = {48, 88, 168, 208};
            for (int r : ruts) rut = std::max(rut, 1.0f - fabsf((float)(x - r)) / 12.0f);
            px[y * 256 + x] = shade(px[y * 256 + x], 1.0f - 0.18f * clampf(rut, 0, 1));
            if (hash1(x * 31 + y * 17) < 0.004f) px[y * 256 + x] = C(150, 140, 120);
            if (x < 8 || x > 247) px[y * 256 + x] = shade(px[y * 256 + x], 0.85f);
        }
        texSandRoad = finishTex(img);
    }
    {
        Image img = noiseImage(128, 128, C(150, 150, 148), 12, 21);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) if (x % 32 < 2 || y % 32 < 2) px[y * 128 + x] = shade(px[y * 128 + x], 0.7f);
        texConcrete = finishTex(img);
    }
    { Image img = noiseImage(128, 128, C(120, 160, 90), 22, 31); texGrass = finishTex(img); }
    { Image img = noiseImage(128, 128, C(225, 200, 150), 14, 41); texSand = finishTex(img); }
    { Image img = noiseImage(128, 128, C(120, 95, 70), 20, 51); texDirt = finishTex(img); }
    {
        Image img = GenImageColor(128, 128, C(40, 110, 150));
        Color* px = (Color*)img.data;
        for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) {
            float a = sinf((x + sinf(y * 0.19f) * 6) * 0.2f) * 0.5f + sinf((y + sinf(x * 0.13f) * 8) * 0.1f) * 0.5f;
            float n = hash1(x * 31 + y * 977);
            float hl = a > 0.75f ? (a - 0.75f) * 4.0f : 0.0f;
            px[y * 128 + x] = C(40 + (int)(hl * 150 + n * 10), 110 + (int)(hl * 120 + n * 10), 150 + (int)(hl * 90 + n * 10));
        }
        texWater = finishTex(img);
    }
    {
        Image img = GenImageColor(64, 64, BLANK);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 64; y++) for (int x = 0; x < 64; x++) {
            float dx = (x - 31.5f) / 31.5f, dy = (y - 31.5f) / 31.5f, r = sqrtf(dx * dx + dy * dy);
            float a = clampf(1.0f - r, 0, 1); a = a * a * (0.6f + 0.4f * a);
            px[y * 64 + x] = C(255, 255, 255, (int)(a * 255));
        }
        texGlow = finishTex(img, false);
    }
    {
        Image img = GenImageColor(64, 64, BLANK);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 64; y++) for (int x = 0; x < 64; x++) {
            float u = (x - 31.5f) / 31.5f, v = y / 63.0f;
            float a = powf(1.0f - v, 1.6f) * expf(-u * u * 4.0f) * clampf(v * 8.0f, 0, 1);
            px[y * 64 + x] = C(255, 255, 255, (int)(a * 255));
        }
        texBeam = finishTex(img, false);
    }
    {
        Image img = GenImageColor(64, 64, WHITE);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 64; y++) for (int x = 0; x < 64; x++) {
            float k = 0.78f + 0.22f * (((x / 3) % 2) ? 1.0f : 0.0f);
            if (y < 3 || y > 60) k = 0.6f;
            px[y * 64 + x] = C((int)(255 * k), (int)(255 * k), (int)(255 * k));
        }
        texContainer = finishTex(img);
    }
    {   // backrooms wallpaper: mono-yellow with faint chevron stripes and damp stains
        Image img = noiseImage(128, 128, C(200, 182, 98), 6, 71);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) {
            float st = ((x + (y / 8 % 2) * 4) % 16 < 2) ? 0.9f : 1.0f;
            float stain = hash1((x / 16) * 13 + (y / 16) * 7 + 3) < 0.12f ? 0.88f : 1.0f;
            float bottom = y > 116 ? 0.75f : 1.0f;
            px[y * 128 + x] = shade(px[y * 128 + x], st * stain * bottom);
        }
        texWallpaper = finishTex(img);
    }
    { Image img = noiseImage(128, 128, C(150, 128, 74), 18, 81); texCarpet = finishTex(img); }
    {
        Image img = noiseImage(128, 128, C(205, 200, 180), 5, 91);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) if (x % 32 < 2 || y % 32 < 2) px[y * 128 + x] = C(120, 116, 100);
        texCeiling = finishTex(img);
    }
    {
        Image img = noiseImage(128, 128, C(225, 238, 244), 4, 101);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) if (x % 16 < 1 || y % 16 < 1) px[y * 128 + x] = C(150, 175, 185);
        texTiles = finishTex(img);
    }
    {
        Image img = noiseImage(128, 128, C(165, 120, 75), 14, 111);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) if (y % 16 < 2) px[y * 128 + x] = C(100, 70, 45);
        texWood = finishTex(img);
    }
    {   // yellow / black hazard stripes for ramps
        Image img = GenImageColor(64, 64, C(255, 200, 0));
        Color* px = (Color*)img.data;
        for (int y = 0; y < 64; y++) for (int x = 0; x < 64; x++) if (((x + y) / 16) % 2) px[y * 64 + x] = C(25, 25, 25);
        texStripes = finishTex(img);
    }
    { Image img = noiseImage(128, 128, C(236, 242, 250), 7, 121); texSnow = finishTex(img); }
    {   // frozen lake: pale blue with white cracks
        Image img = noiseImage(256, 256, C(170, 210, 235), 10, 131);
        Color* px = (Color*)img.data;
        for (int k = 0; k < 26; k++) {
            float x = hash1(k * 17) * 256, y = hash1(k * 29 + 1) * 256, a = hash1(k * 31 + 2) * 6.28f;
            for (int st = 0; st < 90; st++) {
                a += (hash1(k * 1000 + st) - 0.5f) * 0.6f; x += cosf(a); y += sinf(a);
                int ix = ((int)x % 256 + 256) % 256, iy = ((int)y % 256 + 256) % 256;
                px[iy * 256 + ix] = C(240, 248, 255);
            }
        }
        texIce = finishTex(img);
    }
    {   // basalt road: dark stone with faint glowing cracks
        Image img = noiseImage(256, 256, C(58, 52, 50), 14, 141);
        Color* px = (Color*)img.data;
        for (int k = 0; k < 14; k++) {
            float x = hash1(k * 7 + 5) * 256, y = hash1(k * 11 + 3) * 256, a = hash1(k * 13 + 9) * 6.28f;
            for (int st = 0; st < 70; st++) {
                a += (hash1(k * 500 + st) - 0.5f) * 0.8f; x += cosf(a); y += sinf(a);
                int ix = ((int)x % 256 + 256) % 256, iy = ((int)y % 256 + 256) % 256;
                px[iy * 256 + ix] = C(200, 80, 20);
            }
        }
        for (int y = 0; y < 256; y++) for (int x = 0; x < 256; x++) if (x < 6 || x > 249) px[y * 256 + x] = C(180, 150, 40);
        texBasalt = finishTex(img);
    }
    {   // lava: bright noise blobs
        Image img = GenImageColor(128, 128, C(255, 120, 20));
        Color* px = (Color*)img.data;
        for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) {
            float n = sinf(x * 0.15f + sinf(y * 0.11f) * 2.0f) * 0.5f + sinf(y * 0.09f + sinf(x * 0.07f) * 3.0f) * 0.5f;
            float k = clampf(n * 0.5f + 0.5f, 0, 1);
            Color dark = C(190, 50, 10), hot = C(255, 225, 110);
            px[y * 128 + x] = k < 0.3f ? mixC(C(90, 30, 15), dark, k / 0.3f) : mixC(dark, hot, (k - 0.3f) / 0.7f);
        }
        texLava = finishTex(img);
    }
    {   // station metal plates
        Image img = noiseImage(128, 128, C(150, 158, 170), 6, 151);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 128; y++) for (int x = 0; x < 128; x++) {
            if (x % 64 < 2 || y % 64 < 2) px[y * 128 + x] = C(70, 76, 88);
            else if ((x % 64 == 6 || x % 64 == 57) && (y % 64 == 6 || y % 64 == 57)) px[y * 128 + x] = C(210, 215, 225);
        }
        texPlate = finishTex(img);
    }
    {   // station deck road: dark panels with lane markers
        Image img = noiseImage(256, 512, C(52, 58, 72), 6, 161);
        Color* px = (Color*)img.data;
        for (int y = 0; y < 512; y++) for (int x = 0; x < 256; x++) {
            if (y % 128 < 2) px[y * 256 + x] = C(30, 34, 44);
            if ((x > 10 && x < 16) || (x > 240 && x < 246)) px[y * 256 + x] = C(0, 220, 255);
            if (x > 125 && x < 131 && (y % 64) < 36) px[y * 256 + x] = C(230, 235, 245);
        }
        texDeckRoad = finishTex(img);
    }
    {   // shortcut sign: tank silhouette on hazard yellow
        Image img = GenImageColor(128, 128, C(255, 200, 0));
        ImageDrawRectangle(&img, 0, 0, 128, 10, C(25, 25, 25)); ImageDrawRectangle(&img, 0, 118, 128, 10, C(25, 25, 25));
        ImageDrawRectangle(&img, 18, 70, 92, 26, C(25, 25, 25));      // hull
        ImageDrawRectangle(&img, 42, 50, 40, 22, C(25, 25, 25));      // turret
        ImageDrawRectangle(&img, 80, 56, 36, 7, C(25, 25, 25));       // gun
        for (int k = 0; k < 5; k++) ImageDrawCircle(&img, 28 + k * 18, 100, 7, C(25, 25, 25));
        texTankSign = finishTex(img, false);
    }
    {
        const char* words[3] = {"NITRO", "TURBO", "MEGA"};
        Color bg[3] = {C(20, 20, 40), C(240, 240, 240), C(255, 140, 0)};
        Color fg[3] = {C(0, 220, 255), C(220, 20, 40), C(20, 20, 20)};
        for (int k = 0; k < 3; k++) {
            Image img = GenImageColor(256, 128, bg[k]);
            ImageDrawRectangle(&img, 0, 96, 256, 32, fg[k]);
            ImageDrawRectangle(&img, 8, 8, 40, 80, fg[k]);
            ImageDrawText(&img, words[k], 62, 20, 60, fg[k]);
            ImageDrawText(&img, "RACING OIL", 62, 100, 20, bg[k]);
            texBoard[k] = finishTex(img, false);
        }
    }
}
static void genFacades(int tod) {
    if (facadeTod == tod || !IsWindowReady()) return;
    if (facadeTod >= 0) for (int k = 0; k < 3; k++) UnloadTexture(texFacade[k]);
    facadeTod = tod;
    for (int k = 0; k < 3; k++) {
        Color wall = k == 0 ? C(70, 85, 105) : k == 1 ? C(150, 85, 65) : C(165, 160, 150);
        if (tod == TD_NIGHT) wall = shade(wall, 0.22f); else if (tod == TD_SUNSET) wall = shade(wall, 0.75f);
        Image img = noiseImage(128, 256, wall, 6, 61 + k);
        Color* px = (Color*)img.data;
        for (int r = 0; r < 16; r++)
            for (int c = 0; c < 8; c++) {
                int seed = k * 1000 + r * 37 + c * 101;
                float h = hash1(seed);
                Color win;
                if (tod == TD_NIGHT) {
                    if (h < 0.42f) win = hash1(seed + 5) < 0.7f ? C(255, 214, 150) : C(170, 205, 255);
                    else win = C(18, 22, 32);
                } else {
                    float sk = 0.5f + 0.5f * (1.0f - r / 16.0f);
                    win = mixC(C(40, 60, 85), C(150, 180, 210), sk * 0.6f + 0.2f * hash1(seed + 9));
                    if (tod == TD_SUNSET && h < 0.25f) win = C(255, 190, 120);
                }
                int x0 = c * 16 + 3, x1 = c * 16 + 13, y0 = r * 16 + 3, y1 = r * 16 + 12;
                if (k == 0) { x0 = c * 16 + 1; x1 = c * 16 + 15; y0 = r * 16 + 2; y1 = r * 16 + 14; }
                if (k == 2) { x0 = c * 16; x1 = c * 16 + 16; y0 = r * 16 + 5; y1 = r * 16 + 12; }
                for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) px[y * 128 + x] = shade(win, 0.85f + 0.15f * (float)(y - y0) / (float)(y1 - y0));
            }
        texFacade[k] = finishTex(img);
    }
}

// =====================================================================
//                     TRACKS + WORLD LAYOUT
// =====================================================================
struct ItemBox { V3 p; float t; };
struct Coin { V3 p; float t; };
struct Ramp { V3 p; float yaw, w, len, h; };
struct Dyn { V3 home, p, v; float yaw, rot, spin, t; int type; bool hit; Color c; };   // 0 cone, 1 barrel, 2 crate, 3 beach ball, 4 box
static std::vector<ItemBox> itemBoxes;
static std::vector<Coin> coinsW;
static std::vector<Ramp> ramps;
static std::vector<Dyn> dyns;

struct ZoneDef { float a, b; Mode m; Theme t; };
struct TrackDef { float R, sx; float amp[4], ph[4]; float hills; int nz; ZoneDef z[12]; };
static const TrackDef TDEF[NUM_TRACKS] = {
    {1000, 1.5f, {0.22f, 0.15f, 0.10f, 0.07f}, {0.5f, 1.3f, 2.1f, 0.7f}, 7.0f, 12,
     {{0.00f, 0.10f, M_LAND, TH_CITY}, {0.10f, 0.20f, M_LAND, TH_HIGHWAY}, {0.20f, 0.31f, M_WATER, TH_HARBOR}, {0.31f, 0.41f, M_LAND, TH_COUNTRY},
      {0.41f, 0.53f, M_AIR, TH_CANYON}, {0.53f, 0.58f, M_LAND, TH_HIGHWAY}, {0.58f, 0.63f, M_LAND, TH_TUNNEL}, {0.63f, 0.67f, M_LAND, TH_HIGHWAY},
      {0.67f, 0.76f, M_WATER, TH_RIVER}, {0.76f, 0.84f, M_LAND, TH_CITY}, {0.84f, 0.92f, M_AIR, TH_BAY}, {0.92f, 1.01f, M_LAND, TH_CITY}}},
    {800, 1.4f, {0.22f, 0.14f, 0.09f, 0.06f}, {1.1f, 0.4f, 2.0f, 1.0f}, 6.0f, 7,
     {{0.00f, 0.17f, M_LAND, TH_BEACH}, {0.17f, 0.29f, M_WATER, TH_LAGOON}, {0.29f, 0.44f, M_LAND, TH_COUNTRY}, {0.44f, 0.57f, M_AIR, TH_BAY},
      {0.57f, 0.75f, M_LAND, TH_BEACH}, {0.75f, 0.85f, M_WATER, TH_LAGOON}, {0.85f, 1.01f, M_LAND, TH_BEACH}}},
    {520, 1.3f, {0.20f, 0.16f, 0.10f, 0.08f}, {0.3f, 1.0f, 2.5f, 0.4f}, 0.0f, 3,
     {{0.00f, 0.42f, M_LAND, TH_BACKROOMS}, {0.42f, 0.58f, M_WATER, TH_POOL}, {0.58f, 1.01f, M_LAND, TH_BACKROOMS}}},
    // 3: frost pass
    {850, 1.35f, {0.20f, 0.15f, 0.10f, 0.07f}, {0.8f, 2.0f, 1.2f, 0.3f}, 8.0f, 9,
     {{0.00f, 0.16f, M_LAND, TH_SNOW}, {0.16f, 0.27f, M_LAND, TH_ICE}, {0.27f, 0.38f, M_LAND, TH_SNOW}, {0.38f, 0.51f, M_AIR, TH_PEAKS},
      {0.51f, 0.60f, M_LAND, TH_SNOW}, {0.60f, 0.65f, M_LAND, TH_TUNNEL}, {0.65f, 0.73f, M_LAND, TH_SNOW}, {0.73f, 0.85f, M_WATER, TH_FJORD},
      {0.85f, 1.01f, M_LAND, TH_SNOW}}},
    // 4: volcano
    {900, 1.45f, {0.21f, 0.15f, 0.11f, 0.06f}, {2.2f, 0.6f, 1.7f, 2.8f}, 7.0f, 7,
     {{0.00f, 0.18f, M_LAND, TH_DESERT}, {0.18f, 0.28f, M_WATER, TH_OASIS}, {0.28f, 0.40f, M_LAND, TH_DESERT}, {0.40f, 0.56f, M_LAND, TH_VOLCANO},
      {0.56f, 0.69f, M_AIR, TH_CRATER}, {0.69f, 0.80f, M_LAND, TH_VOLCANO}, {0.80f, 1.01f, M_LAND, TH_DESERT}}},
    // 5: orbit station (space)
    {760, 1.3f, {0.19f, 0.15f, 0.12f, 0.08f}, {1.5f, 2.4f, 0.2f, 1.1f}, 5.0f, 7,
     {{0.00f, 0.22f, M_LAND, TH_STATION}, {0.22f, 0.36f, M_AIR, TH_VOID}, {0.36f, 0.52f, M_LAND, TH_STATION}, {0.52f, 0.64f, M_WATER, TH_COOLANT},
      {0.64f, 0.78f, M_LAND, TH_STATION}, {0.78f, 0.88f, M_AIR, TH_VOID}, {0.88f, 1.01f, M_LAND, TH_STATION}}}};
static bool spaceTrack() { return curTrack == 5; }
// detour loops: the road swings out around a hill / block, a tank shortcut cuts straight across
struct Bump { float u, half, h; };
static const Bump BUMPS[NUM_TRACKS][2] = {{{0.36f, 0.03f, 210}, {0.80f, 0.026f, 170}}, {{0.085f, 0.032f, 200}, {0.66f, 0.032f, 200}}, {{0, 0, 0}, {0, 0, 0}},
                                          {{0.08f, 0.03f, 200}, {0.925f, 0.028f, 180}}, {{0.09f, 0.032f, 210}, {0.34f, 0.028f, 190}}, {{0, 0, 0}, {0, 0, 0}}};
static std::vector<int> bumpA, bumpB;
static bool baseAllowed(Base b) { return !spaceTrack() || b == B_CAR || b == B_KART; }

static float airProfile(float u) {
    return 4.0f + 55.0f * powf(sinf(PI_F * clampf(u, 0, 1)), 1.3f) * (0.8f + 0.2f * sinf(4 * PI_F * u + 1.0f));
}
static float hAtLat(int i, float s) {
    float a = fabsf(s), L = lim[i] + 1.0f;
    if (a <= L) return th[i];
    return th[i] * clampf(1.0f - (a - L) / 40.0f, 0, 1);
}
static bool indoorTrack() { return curTrack == 2; }
static bool clearOfTrack(V3 p, float r, float extra = 0) {
    for (int j = 0; j < N; j += 2) {
        float dx = track[j].x - p.x, dz = track[j].z - p.z, need = lim[j] + r + extra;
        if (dx * dx + dz * dz < need * need) return false;
    }
    return true;
}
static void buildTrack(int tid) {
    curTrack = tid; spaceNames = tid == 5;
    const TrackDef& D = TDEF[tid];
    const int CP = 48, SEG = 50;
    std::vector<V3> cp(CP);
    const int H[4] = {2, 3, 5, 7};
    for (int i = 0; i < CP; i++) {
        float a = 2 * PI_F * i / CP, r = 1.0f;
        for (int k = 0; k < 4; k++) r += D.amp[k] * sinf(H[k] * a + D.ph[k]);
        r *= D.R;
        cp[i] = vec(cosf(a) * r * D.sx, 0, sinf(a) * r);
    }
    auto cr = [](float a, float b, float c, float d, float t) {
        float t2 = t * t, t3 = t2 * t;
        return 0.5f * ((2 * b) + (-a + c) * t + (2 * a - 5 * b + 4 * c - d) * t2 + (-a + 3 * b - 3 * c + d) * t3);
    };
    track.clear();
    for (int i = 0; i < CP; i++)
        for (int s = 0; s < SEG; s++) {
            V3 p0 = cp[(i + CP - 1) % CP], p1 = cp[i], p2 = cp[(i + 1) % CP], p3 = cp[(i + 2) % CP];
            float t = (float)s / SEG;
            track.push_back(vec(cr(p0.x, p1.x, p2.x, p3.x, t), 0, cr(p0.z, p1.z, p2.z, p3.z, t)));
        }
    N = (int)track.size();
    // push detour loops outwards (away from the loop centre)
    bumpA.clear(); bumpB.clear();
    {
        V3 ctr = vec(0, 0, 0); for (const V3& p : track) ctr = add(ctr, p); ctr = mul(ctr, 1.0f / N);
        std::vector<V3> base = track;
        for (int b = 0; b < 2; b++) {
            const Bump& bp = BUMPS[tid][b];
            if (bp.h <= 0) continue;
            int c = (int)(bp.u * N), hwI = (int)(bp.half * N);
            V3 tn = norm(sub(base[(c + 1) % N], base[(c + N - 1) % N])), nv = vec(tn.z, 0, -tn.x);
            if (dot(nv, sub(base[c], ctr)) < 0) nv = mul(nv, -1.0f);
            for (int k = -hwI; k <= hwI; k++) {
                int i = (c + k + N) % N;
                V3 ti = norm(sub(base[(i + 1) % N], base[(i + N - 1) % N])), ni = vec(ti.z, 0, -ti.x);
                if (dot(ni, nv) < 0) ni = mul(ni, -1.0f);
                float t = (float)(k + hwI) / (2.0f * hwI), w = sinf(PI_F * t); w *= w;
                track[i] = add(track[i], mul(ni, bp.h * w));
            }
            bumpA.push_back((c - hwI + N) % N); bumpB.push_back((c + hwI) % N);
        }
        // re-space points evenly along the new curve so speeds and indices stay uniform
        std::vector<float> acc(N + 1, 0);
        for (int i = 0; i < N; i++) acc[i + 1] = acc[i] + dist2D(track[i], track[(i + 1) % N]);
        float total = acc[N];
        std::vector<V3> res(N);
        int seg = 0;
        for (int i = 0; i < N; i++) {
            float want = total * i / N;
            while (seg < N - 1 && acc[seg + 1] < want) seg++;
            float t = (want - acc[seg]) / std::max(1e-4f, acc[seg + 1] - acc[seg]);
            res[i] = add(track[seg], mul(sub(track[(seg + 1) % N], track[seg]), t));
        }
        // map bump indices to the re-spaced indices
        for (size_t b = 0; b < bumpA.size(); b++) {
            bumpA[b] = (int)(acc[bumpA[b]] / total * N) % N; bumpB[b] = (int)(acc[bumpB[b]] / total * N) % N;
        }
        track = res;
    }
    tanv.assign(N, vec(0, 0, 1)); norv.assign(N, vec(1, 0, 0)); curv.assign(N, 0); airAlt.assign(N, 0); sArc.assign(N, 0); th.assign(N, 0);
    zmode.assign(N, M_LAND); theme.assign(N, TH_CITY);
    mapMinX = mapMinZ = 1e9f; mapMaxX = mapMaxZ = -1e9f;
    for (int i = 0; i < N; i++) {
        V3 t = norm(sub(track[(i + 1) % N], track[(i + N - 1) % N]));
        tanv[i] = t; norv[i] = vec(t.z, 0, -t.x);
        mapMinX = std::min(mapMinX, track[i].x); mapMaxX = std::max(mapMaxX, track[i].x);
        mapMinZ = std::min(mapMinZ, track[i].z); mapMaxZ = std::max(mapMaxZ, track[i].z);
        if (i > 0) sArc[i] = sArc[i - 1] + dist2D(track[i], track[i - 1]);
    }
    trackLen = sArc[N - 1] + dist2D(track[0], track[N - 1]);
    for (int i = 0; i < N; i++) {
        V3 a = tanv[i], b = tanv[(i + 6) % N];
        float ang = fabsf(atan2f(a.x * b.z - a.z * b.x, a.x * b.x + a.z * b.z));
        curv[i] = ang / std::max(dist2D(track[(i + 6) % N], track[i]), 1.0f);
    }
    gateIdx.clear(); rings.clear();
    for (int zi = 0; zi < D.nz; zi++) {
        const ZoneDef& z = D.z[zi];
        int a = (int)(z.a * N), b = std::min(N, (int)(z.b * N));
        for (int i = a; i < b; i++) {
            zmode[i] = z.m; theme[i] = z.t;
            if (z.m == M_AIR) airAlt[i] = airProfile((float)(i - a) / (float)(b - a));
        }
        if (z.m != M_LAND) {
            gateIdx.push_back(a); gateIdx.push_back(b % N);
            if (z.m == M_AIR)
                for (int i = a + 22; i < b - 10; i += 24) {
                    Ring r; r.idx = i; r.p = vec(track[i].x, airAlt[i] + 1.0f, track[i].z); r.tang = tanv[i];
                    rings.push_back(r);
                }
        }
    }
    hw.assign(N, 14.0f); lim.assign(N, 0); outerW.assign(N, 0);
    std::vector<float> lraw(N), oraw(N), hraw(N);
    for (int i = 0; i < N; i++) {
        float h = 14, L = 20, O = 0;
        switch (theme[i]) {
            case TH_CITY: L = 20; O = 95; break;
            case TH_HIGHWAY: L = 17; break;
            case TH_COUNTRY: L = 34; O = curTrack == 1 ? 150.0f : 0.0f; break;
            case TH_HARBOR: case TH_RIVER: h = 22; L = 22; O = 90; break;
            case TH_TUNNEL: L = 15.2f; break;
            case TH_CANYON: L = 70; O = 260; break;
            case TH_BAY: L = 70; O = 320; break;
            case TH_BEACH: L = 24; O = 130; break;
            case TH_LAGOON: h = 24; L = 24; O = 130; break;
            case TH_BACKROOMS: h = 16; L = 18; break;
            case TH_POOL: h = 17; L = 18; break;
            case TH_SNOW: h = 14; L = 20; break;
            case TH_ICE: h = 22; L = 26; break;
            case TH_PEAKS: L = 70; break;
            case TH_FJORD: h = 22; L = 22; break;
            case TH_DESERT: h = 14; L = 19; break;
            case TH_OASIS: h = 24; L = 24; break;
            case TH_VOLCANO: h = 15; L = 17; break;
            case TH_CRATER: L = 70; break;
            case TH_STATION: h = 15; L = 17; break;
            case TH_COOLANT: h = 18; L = 18; break;
            case TH_VOID: L = 70; break;
            default: break;
        }
        hraw[i] = h; lraw[i] = L; oraw[i] = O;
    }
    hw = hraw;
    for (int it = 0; it < 12; it++) {
        std::vector<float> t = hw;
        for (int i = 0; i < N; i++) { float s = 0; for (int k = -3; k <= 3; k++) s += t[(i + k + N) % N]; hw[i] = s / 7.0f; }
    }
    for (int i = 0; i < N; i++) {
        float s = 0, o = 0; for (int k = -5; k <= 5; k++) { s += lraw[(i + k + N) % N]; o += oraw[(i + k + N) % N]; }
        lim[i] = std::max(s / 11.0f, hw[i] + 1.0f); outerW[i] = o / 11.0f;
    }
    // hills: only in open-road themes, eased to zero towards other zones
    std::vector<float> m(N);
    for (int i = 0; i < N; i++) { Theme t = theme[i]; m[i] = (t == TH_COUNTRY || t == TH_HIGHWAY || t == TH_BEACH || t == TH_SNOW || t == TH_DESERT || t == TH_STATION) ? 1.0f : 0.0f; }
    for (int pass = 0; pass < 2; pass++) {
        std::vector<float> t = m;
        for (int i = 0; i < N; i++) { float s = 0; for (int k = -25; k <= 25; k++) s += t[(i + k + N) % N]; m[i] = s / 51.0f; }
    }
    for (int i = 0; i < N; i++) {
        float s = sArc[i];
        float base = D.hills * (0.55f + 0.45f * sinf(s / 170.0f + 1.3f)) * (0.6f + 0.4f * sinf(s / 83.0f + 0.4f));
        th[i] = base * clampf(2.0f * m[i] - 1.0f, 0, 1);
    }
    // keep the start straight flat
    for (int k = -60; k <= 20; k++) th[(k + N) % N] = 0;
}

static void layoutItemBoxes(int step) {
    itemBoxes.clear();
    for (int i = 40; i < N; i += step) {
        if (zmode[i] == M_AIR) {
            for (int k = -1; k <= 1; k++) { ItemBox b; b.p = add(track[i], mul(norv[i], k * 9.0f)); b.p.y = airAlt[i] + 1.0f; b.t = 0; itemBoxes.push_back(b); }
            continue;
        }
        if (theme[i] == TH_TUNNEL) continue;
        float w = std::min(hw[i] - 3.0f, 12.0f);
        for (int k = 0; k < 4; k++) {
            ItemBox b; float l = -w + 2 * w * k / 3.0f;
            b.p = add(track[i], mul(norv[i], l)); b.p.y = (zmode[i] == M_WATER ? 0.0f : th[i]) + 1.4f; b.t = 0;
            itemBoxes.push_back(b);
        }
    }
}
// ------------------------------ tank shortcuts ------------------------
// a straight dirt path cutting a bend, closed by a crate barricade that only a tank can smash
struct Shortcut { int a, b; V3 pa, pb, dir, nrm, wallP, wdir, wn; float len, w, wallAlong, whl; bool open; float openT; };
static std::vector<Shortcut> shortcuts;
static bool shortcutThemeOK(Theme t) { return t == TH_CITY || t == TH_HIGHWAY || t == TH_COUNTRY || t == TH_BEACH || t == TH_SNOW || t == TH_DESERT; }
// returns the shortcut whose corridor contains p (with margin), -1 if none
static int shortcutAt(V3 p, float margin = 0) {
    for (size_t k = 0; k < shortcuts.size(); k++) {
        const Shortcut& s = shortcuts[k];
        V3 d = sub(p, s.pa); d.y = 0;
        float u = dot(d, s.dir), v = dot(d, s.nrm);
        if (u > -2 - margin && u < s.len + 2 + margin && fabsf(v) < s.w / 2 + margin) return (int)k;
    }
    return -1;
}
static void buildShortcuts() {
    shortcuts.clear();
    if (curTrack == 2 || curTrack == 5) return;
    struct Cand { int i, j; float score; };
    std::vector<Cand> cands;
    for (size_t b = 0; b < bumpA.size(); b++) {
        // the path starts/ends a little inside the loop's flanks
        int span = (bumpB[b] - bumpA[b] + N) % N, i = (bumpA[b] + span / 14) % N, j = (bumpB[b] - span / 14 + N) % N;
        bool ok = true;
        for (int k = 0; k <= span && ok; k++) { int q = (bumpA[b] + k) % N; if (zmode[q] != M_LAND || !shortcutThemeOK(theme[q])) ok = false; }
        if (ok) cands.push_back({i, j, 1});
    }
    for (const Cand& c : cands) {
        Shortcut s; s.a = c.i; s.b = c.j; s.pa = track[c.i]; s.pb = track[c.j]; s.pa.y = s.pb.y = 0;
        s.dir = norm(sub(s.pb, s.pa)); s.nrm = vec(s.dir.z, 0, -s.dir.x); s.len = dist2D(s.pa, s.pb); s.w = 10.0f;
        s.open = false; s.openT = 0;
        // barricade where the path crosses the road edge
        // the barricade runs along the road edge where the path leaves the road
        s.wallAlong = 6; s.wallP = s.pa; s.wdir = tanv[c.i]; s.wn = norv[c.i]; s.whl = 6;
        for (float u = 1; u < s.len * 0.5f; u += 0.25f) {
            V3 p = add(s.pa, mul(s.dir, u));
            int best = c.i; float bd = 1e9f;
            for (int k = c.i; k <= c.i + 20; k++) { float d = dist2D(p, track[k % N]); if (d < bd) { bd = d; best = k % N; } }
            float lat = dot(sub(p, track[best]), norv[best]);
            if (fabsf(lat) > lim[best] + 0.8f) {
                s.wallAlong = u; s.wallP = p; s.wdir = tanv[best]; s.wn = mul(norv[best], sgn(lat));
                float sn = fabsf(dot(s.dir, s.wn));
                s.whl = std::min(14.0f, (s.w / 2) / std::max(0.3f, sn) + 1.0f);
                break;
            }
        }
        shortcuts.push_back(s);
    }
    // clear props out of the shortcut corridors
    auto blocked = [](V3 p, float r) { return shortcutAt(p, r + 4.0f) >= 0; };
    trees.erase(std::remove_if(trees.begin(), trees.end(), [&](const Tree& t) { return blocked(t.p, 2); }), trees.end());
    rocks.erase(std::remove_if(rocks.begin(), rocks.end(), [&](const Rock& r) { return blocked(r.p, std::max(r.w, r.d) * 0.6f); }), rocks.end());
    blds.erase(std::remove_if(blds.begin(), blds.end(), [&](const Bld& b) { return blocked(b.p, std::max(b.w, b.d) * 0.7f); }), blds.end());
    decos.erase(std::remove_if(decos.begin(), decos.end(), [&](const Deco& d) { return blocked(d.p, 4); }), decos.end());
    boxes.erase(std::remove_if(boxes.begin(), boxes.end(), [&](const Box& b) { return blocked(b.p, 6); }), boxes.end());
    cranes.erase(std::remove_if(cranes.begin(), cranes.end(), [&](const Crane& c) { return blocked(c.p, 8); }), cranes.end());
    boards.erase(std::remove_if(boards.begin(), boards.end(), [&](const Board& b) { return blocked(b.p, 10); }), boards.end());
    lamps.erase(std::remove_if(lamps.begin(), lamps.end(), [&](const Lamp& l) { return blocked(l.base, 1); }), lamps.end());
}

static void placeProps() {
    SetRandomSeed(424242 + curTrack * 77);
    trees.clear(); rocks.clear(); blds.clear(); lamps.clear(); boxes.clear(); cranes.clear(); boards.clear(); pads.clear(); decos.clear();
    itemBoxes.clear(); coinsW.clear(); ramps.clear(); dyns.clear();
    const Color neonPal[5] = {C(255, 40, 160), C(0, 230, 255), C(170, 70, 255), C(255, 140, 0), C(60, 255, 120)};
    int lampId = 0;
    for (int side = -1; side <= 1; side += 2) {
        float nextB = frand(0, 20), nextL = side > 0 ? 0.0f : 15.0f, nextC = 0, nextBoard = 200, nextD = frand(0, 30), nextP = 20;
        for (int i = 0; i < N; i++) {
            Theme thm = theme[i]; float s = sArc[i];
            V3 P = track[i], nv = mul(norv[i], (float)side);
            float yaw = atan2f(tanv[i].x, tanv[i].z);
            if ((thm == TH_CITY || thm == TH_RIVER) && s >= nextB) {
                Bld b; b.w = frand(16, 34); b.d = frand(16, 30);
                float hr = frand(0, 1);
                b.h = hr < 0.6f ? frand(14, 40) : hr < 0.92f ? frand(40, 85) : frand(85, 150);
                float off = lim[i] + (thm == TH_RIVER ? 10.0f : 3.0f) + b.d / 2;
                b.p = add(P, mul(nv, off)); b.yaw = yaw; b.tex = GetRandomValue(0, 2);
                int g = GetRandomValue(190, 255); b.tint = C(g, g, g);
                b.neon = frand(0, 1) < 0.4f ? GetRandomValue(1, 2) : 0; b.neonC = neonPal[GetRandomValue(0, 4)];
                b.antenna = b.h > 50 && frand(0, 1) < 0.5f; b.side = side;
                if (clearOfTrack(b.p, sqrtf(b.w * b.w + b.d * b.d) * 0.43f)) blds.push_back(b);
                nextB = s + b.w + frand(3, 9);
            }
            if (thm == TH_HARBOR && s >= nextB) {
                float off = lim[i] + frand(8, 40);
                if (frand(0, 1) < 0.35f) {
                    Bld b; b.w = frand(30, 50); b.d = frand(20, 28); b.h = frand(10, 16);
                    b.p = add(P, mul(nv, lim[i] + 45 + b.d / 2)); b.yaw = yaw; b.tex = 2; b.tint = C(170, 180, 190);
                    b.neon = 0; b.neonC = WHITE; b.antenna = false; b.side = side;
                    if (clearOfTrack(b.p, sqrtf(b.w * b.w + b.d * b.d) * 0.45f)) blds.push_back(b);
                }
                int stacks = GetRandomValue(1, 3);
                Color cc[6] = {C(200, 50, 40), C(30, 90, 170), C(230, 160, 30), C(40, 140, 80), C(120, 120, 130), C(200, 90, 30)};
                for (int k = 0; k < stacks; k++) {
                    Box bx; bx.w = 2.6f; bx.d = 12.0f; bx.h = 2.6f; bx.yaw = yaw + PI_F / 2; bx.tex = 1;
                    V3 base = add(P, mul(nv, off + k * 3.0f));
                    int hgt = GetRandomValue(1, 3);
                    for (int l = 0; l < hgt; l++) {
                        bx.p = vec(base.x, 1.3f + l * 2.65f, base.z); bx.c = cc[GetRandomValue(0, 5)];
                        if (clearOfTrack(bx.p, 6.5f)) boxes.push_back(bx);
                    }
                }
                nextB = s + frand(14, 30);
            }
            if (thm == TH_HARBOR && s >= nextC) {
                Crane c; c.p = add(P, mul(nv, lim[i] + 12)); c.yaw = yaw; c.h = frand(38, 52);
                if (clearOfTrack(c.p, 8)) cranes.push_back(c);
                nextC = s + frand(180, 320);
            }
            // beach decorations
            if ((thm == TH_BEACH || thm == TH_LAGOON) && s >= nextD) {
                int n = GetRandomValue(1, 3);
                for (int k = 0; k < n; k++) {
                    Deco d; float off = lim[i] + frand(6, 70);
                    float r = frand(0, 1);
                    d.type = r < 0.45f ? 0 : r < 0.62f ? 1 : r < 0.8f ? 2 : r < 0.88f ? 3 : 4;
                    d.p = add(add(P, mul(nv, off)), mul(tanv[i], frand(-8, 8)));
                    d.p.y = hAtLat(i, off); d.yaw = yaw + frand(-0.5f, 0.5f); d.s = frand(0.9f, 1.2f);
                    Color pal[6] = {C(255, 70, 70), C(255, 200, 40), C(40, 160, 255), C(60, 220, 120), C(255, 120, 200), C(255, 255, 255)};
                    d.c = pal[GetRandomValue(0, 5)];
                    if (clearOfTrack(d.p, d.type == 1 ? 6.0f : 2.5f)) decos.push_back(d);
                }
                nextD = s + frand(18, 40);
            }
            // backrooms pillars
            if (thm == TH_BACKROOMS && side > 0 && s >= nextP) {
                int n = GetRandomValue(1, 2);
                float off0 = frand(-hw[i] + 3, hw[i] - 3);
                for (int k = 0; k < n; k++) {
                    // second pillar keeps a wide gap so even a tank fits between them
                    float off = k == 0 ? off0 : (off0 > 0 ? off0 - frand(15, 20) : off0 + frand(15, 20));
                    if (fabsf(off) > hw[i] - 3) break;
                    Tree t; t.p = add(P, mul(norv[i], off)); t.p.y = 0; t.s = 1.0f; t.kind = 3; t.solid = true;
                    trees.push_back(t);
                }
                nextP = s + frand(45, 90);
            }
            // frost / volcano / space decorations
            if (s >= nextD && (thm == TH_SNOW || thm == TH_FJORD || thm == TH_DESERT || thm == TH_VOLCANO || thm == TH_VOID || thm == TH_STATION || thm == TH_CRATER)) {
                Deco d; d.yaw = yaw + frand(-0.6f, 0.6f); d.s = frand(0.9f, 1.3f); d.c = WHITE;
                bool ok = true; float gap = frand(60, 140);
                if (thm == TH_SNOW) { d.type = 7; float off = lim[i] + frand(5, 30); d.p = add(P, mul(nv, off)); d.p.y = hAtLat(i, off); gap = frand(150, 320); }
                else if (thm == TH_FJORD) { d.type = 6; float off = lim[i] + frand(6, 40); d.p = add(P, mul(nv, off)); d.p.y = 0.1f; d.s = frand(1.5f, 4.0f); gap = frand(25, 60); }
                else if (thm == TH_DESERT) { d.type = 10; float off = lim[i] + frand(12, 60); d.p = add(P, mul(nv, off)); d.p.y = hAtLat(i, off); gap = frand(200, 400); }
                else if (thm == TH_VOLCANO || thm == TH_CRATER) { d.type = 9; float off = lim[i] + frand(6, thm == TH_CRATER ? 120.0f : 26.0f); d.p = add(P, mul(nv, off)); d.p.y = 0; gap = frand(50, 110); }
                else if (thm == TH_VOID) { d.type = 5; float off = lim[i] + frand(-40, 220); d.p = add(P, mul(nv, off)); d.p.y = frand(-50, 110); d.s = frand(3, 16); gap = frand(18, 40); }
                else { d.type = 8; float off = lim[i] + frand(55, 170); d.p = add(P, mul(nv, off)); d.p.y = frand(-25, 45); d.s = frand(1.0f, 2.2f); gap = frand(140, 300); }
                if (d.type != 5 && d.type != 8 && !clearOfTrack(d.p, d.type == 6 ? d.s * 2 : 3.0f)) ok = false;
                if (d.type == 5 && fabsf(dot(sub(d.p, P), norv[i])) < lim[i] + 8 && d.p.y < airAlt[i] + 30 && d.p.y > airAlt[i] - 30) ok = false;
                if (ok) decos.push_back(d);
                nextD = s + gap;
            }
            // lamps
            if (s >= nextL) {
                Lamp L; L.type = -1;
                if (thm == TH_CITY) { L.type = 0; L.c = C(215, 230, 255); L.base = add(P, mul(nv, hw[i] + 2.6f)); L.head = add(add(P, mul(nv, hw[i] - 2.0f)), vec(0, 9.0f, 0)); nextL = s + 32; }
                else if (thm == TH_HIGHWAY) { L.type = 1; L.c = C(255, 168, 70); L.base = add(P, mul(nv, lim[i] + 1.2f)); L.head = add(add(P, mul(nv, lim[i] - 4.0f)), vec(0, 11.0f, 0)); nextL = s + 48; }
                else if (thm == TH_HARBOR || thm == TH_RIVER) { L.type = 2; L.c = C(255, 190, 110); L.base = add(P, mul(nv, lim[i] + 2.0f)); L.head = add(add(P, mul(nv, lim[i] + 0.5f)), vec(0, 8.0f, 0)); nextL = s + 40; }
                else if (thm == TH_TUNNEL) { L.type = 3; L.c = C(255, 150, 50); L.base = add(P, mul(nv, 6.0f)); L.head = add(add(P, mul(nv, 6.0f)), vec(0, 7.6f, 0)); nextL = s + 14; }
                else if (thm == TH_BACKROOMS) { L.type = 4; L.c = C(255, 250, 215); L.base = add(P, mul(nv, 5.0f)); L.head = add(L.base, vec(0, 5.85f, 0)); nextL = s + 9; }
                else if (thm == TH_POOL) { L.type = 5; L.c = C(220, 245, 255); L.base = add(P, mul(nv, 4.0f)); L.head = add(L.base, vec(0, 5.85f, 0)); nextL = s + 10; }
                else if (thm == TH_SNOW) { L.type = 1; L.c = C(255, 190, 120); L.base = add(P, mul(nv, lim[i] + 1.2f)); L.head = add(add(P, mul(nv, lim[i] - 4.0f)), vec(0, 10.0f, 0)); nextL = s + 70; }
                else if (thm == TH_DESERT) { L.type = 6; L.c = C(255, 150, 50); L.base = add(P, mul(nv, lim[i] + 1.5f)); L.base.y = hAtLat(i, lim[i] + 1.5f); L.head = add(L.base, vec(0, 3.2f, 0)); nextL = s + 55; }
                else if (thm == TH_BEACH || thm == TH_LAGOON) { L.type = 6; L.c = C(255, 150, 50); L.base = add(P, mul(nv, lim[i] + 1.5f)); L.base.y = hAtLat(i, lim[i] + 1.5f); L.head = add(L.base, vec(0, 3.2f, 0)); nextL = s + 42; }
                else nextL = s + 20;
                if (L.type >= 0 && L.type != 4 && L.type != 5 && L.type != 6) { L.base.y = hAtLat(i, dot(sub(L.base, P), norv[i])); L.head.y += L.base.y; }
                if (L.type >= 0) { L.armYaw = yaw; L.id = lampId++; lamps.push_back(L); }
            }
            if (thm == TH_HIGHWAY && s >= nextBoard) {
                Board b; b.p = add(P, mul(nv, lim[i] + 12)); b.p.y = hAtLat(i, lim[i] + 12); b.yaw = yaw + (side > 0 ? 0.0f : PI_F); b.seed = GetRandomValue(0, 2);
                b.c = WHITE; if (clearOfTrack(b.p, 10)) boards.push_back(b);
                nextBoard = s + frand(350, 600);
            }
        }
    }
    // trees / rocks
    for (int k = 0; k < 9000; k++) {
        int i = GetRandomValue(0, N - 1); Theme thm = theme[i];
        float side = GetRandomValue(0, 1) ? 1.0f : -1.0f;
        bool beachTrack = curTrack == 1;
        if (thm == TH_COUNTRY || thm == TH_HIGHWAY || ((thm == TH_BEACH || thm == TH_LAGOON) && k % 3 == 0)) {
            float off = thm == TH_COUNTRY ? frand(hw[i] + 6, lim[i] + 140) : frand(lim[i] + 6, lim[i] + 150);
            if (thm == TH_COUNTRY && off < lim[i] && frand(0, 1) < 0.7f) continue;
            Tree t; t.p = add(track[i], mul(norv[i], side * off)); t.p.y = hAtLat(i, off); t.s = frand(0.8f, 1.5f);
            t.kind = beachTrack ? (frand(0, 1) < 0.8f ? 2 : 0) : GetRandomValue(0, 1);
            t.solid = off < lim[i] + 3;
            bool ok = true;
            if (t.solid) { for (int j = 0; j < N && ok; j += 2) if (dist2D(t.p, track[j]) < hw[j] + 5.0f || (theme[j] != TH_COUNTRY && dist2D(t.p, track[j]) < lim[j] + 4.0f)) ok = false; }
            else ok = clearOfTrack(t.p, 4.0f);
            if (ok) trees.push_back(t);
        } else if ((thm == TH_SNOW || thm == TH_FJORD || (thm == TH_ICE && k % 3 == 0)) && k % 2 == 0) {
            float base0 = thm == TH_ICE ? lim[i] + 62 : thm == TH_FJORD ? lim[i] + 46 : lim[i] + 6;
            float off = frand(base0, base0 + 170);
            Tree t; t.p = add(track[i], mul(norv[i], side * off)); t.p.y = hAtLat(i, off); t.s = frand(0.9f, 1.7f); t.kind = 5;
            t.solid = off < lim[i] + 3;
            if (clearOfTrack(t.p, 4.0f)) trees.push_back(t);
        } else if ((thm == TH_PEAKS && k % 3 == 0) || ((thm == TH_SNOW || thm == TH_ICE) && k % 17 == 0)) {
            Rock r; float off = thm == TH_PEAKS ? frand(lim[i] + 15, lim[i] + 240) : frand(lim[i] + 160, lim[i] + 380);
            r.p = add(track[i], mul(norv[i], side * off)); r.w = frand(30, 80); r.d = frand(30, 80); r.h = frand(50, 190);
            r.yaw = frand(0, 6.28f); r.seed = GetRandomValue(0, 99999); r.c = mixC(C(120, 125, 140), C(150, 155, 170), frand(0, 1));
            if (clearOfTrack(r.p, std::max(r.w, r.d) * 0.6f)) rocks.push_back(r);
        } else if ((thm == TH_DESERT || thm == TH_OASIS) && k % 2 == 0) {
            if (thm == TH_OASIS || k % 6 == 0) {
                float off = thm == TH_OASIS ? frand(lim[i] + 42, lim[i] + 70) : frand(lim[i] + 4, lim[i] + 140);
                Tree t; t.p = add(track[i], mul(norv[i], side * off)); t.p.y = hAtLat(i, off); t.s = frand(0.8f, 1.4f);
                t.kind = thm == TH_OASIS ? 2 : 4; t.solid = off < lim[i] + 3;
                if (clearOfTrack(t.p, 3.0f)) trees.push_back(t);
            } else if (k % 10 == 0) {
                Rock r; float off = frand(lim[i] + 80, lim[i] + 340);
                r.p = add(track[i], mul(norv[i], side * off)); r.w = frand(40, 110); r.d = frand(40, 110); r.h = frand(25, 70);
                r.yaw = frand(0, 6.28f); r.seed = GetRandomValue(0, 99999); r.c = mixC(C(200, 110, 70), C(170, 85, 55), frand(0, 1));
                if (clearOfTrack(r.p, std::max(r.w, r.d) * 0.6f)) rocks.push_back(r);
            }
        } else if ((thm == TH_VOLCANO && k % 4 == 0) || (thm == TH_CRATER && k % 3 == 0)) {
            Rock r; float off = thm == TH_CRATER ? frand(lim[i] + 140, lim[i] + 300) : frand(lim[i] + 34, lim[i] + 220);
            r.p = add(track[i], mul(norv[i], side * off)); r.w = frand(14, 50); r.d = frand(14, 50); r.h = thm == TH_CRATER ? frand(40, 120) : frand(8, 45);
            r.yaw = frand(0, 6.28f); r.seed = GetRandomValue(0, 99999); r.c = mixC(C(48, 42, 42), C(70, 58, 52), frand(0, 1));
            if (clearOfTrack(r.p, std::max(r.w, r.d) * 0.6f)) rocks.push_back(r);
        } else if (thm == TH_CANYON && k % 3 == 0) {
            Rock r; float off = frand(lim[i] + 15, lim[i] + 220);
            r.p = add(track[i], mul(norv[i], side * off)); r.w = frand(18, 60); r.d = frand(18, 60); r.h = frand(30, 170);
            r.yaw = frand(0, 6.28f); r.seed = GetRandomValue(0, 99999); r.c = mixC(C(190, 105, 65), C(150, 80, 55), frand(0, 1));
            if (clearOfTrack(r.p, std::max(r.w, r.d) * 0.6f)) rocks.push_back(r);
        } else if ((thm == TH_COUNTRY && k % 7 == 0) || (thm == TH_BAY && curTrack == 1 && k % 4 == 0)) {
            Rock r; float off = thm == TH_BAY ? frand(lim[i] + 20, lim[i] + 200) : frand(lim[i] + 8, lim[i] + 120);
            r.p = add(track[i], mul(norv[i], side * off));
            bool big = thm == TH_BAY;
            r.w = big ? frand(20, 50) : frand(3, 9); r.d = big ? frand(20, 50) : frand(3, 9); r.h = big ? frand(25, 90) : frand(2, 6);
            r.yaw = frand(0, 6.28f); r.seed = GetRandomValue(0, 99999); r.c = big ? C(150, 135, 110) : C(125, 120, 112);
            r.p.y = 0;
            if (clearOfTrack(r.p, big ? std::max(r.w, r.d) * 0.6f : 5)) rocks.push_back(r);
        }
    }
    // boost pads
    for (int i = 60; i < N - 30; i += 110) {
        if (zmode[i] != M_LAND || theme[i] == TH_TUNNEL || theme[i] == TH_ICE || zmode[(i + 30) % N] != M_LAND || zmode[(i + N - 30) % N] != M_LAND) continue;
        Pad p; p.p = add(track[i], mul(norv[i], frand(-8, 8))); p.p.y = th[i]; p.yaw = atan2f(tanv[i].x, tanv[i].z);
        pads.push_back(p);
    }
    // jump ramps on open roads
    float lastS = -1e9f;
    for (int i = 100; i < N - 40; i += 1) {
        Theme t = theme[i];
        if (!(t == TH_COUNTRY || t == TH_HIGHWAY || t == TH_BEACH || t == TH_SNOW || t == TH_DESERT || t == TH_STATION)) continue;
        if (sArc[i] - lastS < (curTrack == 1 || curTrack == 5 ? 330.0f : 480.0f)) continue;
        bool nearPad = false;
        for (const Pad& p : pads) if (dist2D(p.p, track[i]) < 40) nearPad = true;
        if (nearPad || curv[i] > 0.006f) continue;
        Ramp r; r.w = 9; r.len = 15; r.h = 2.8f;
        r.p = add(track[i], mul(norv[i], frand(-6, 6))); r.p.y = th[i]; r.yaw = atan2f(tanv[i].x, tanv[i].z);
        ramps.push_back(r); lastS = sArc[i];
    }
    // item boxes, coins, destructibles
    layoutItemBoxes(280);
    for (int i = 100; i < N; i += 75) {
        if (zmode[i] == M_AIR || zmode[(i + 10) % N] == M_AIR) continue;
        float lane = frand(-0.6f, 0.6f) * hw[i];
        for (int k = 0; k < 6; k++) {
            int j = (i + k * 2) % N;
            Coin c; c.p = add(track[j], mul(norv[j], lane)); c.p.y = (zmode[j] == M_WATER ? 0.0f : th[j]) + 1.0f; c.t = 0;
            coinsW.push_back(c);
        }
    }
    for (int i = 30; i < N; i += 28) {
        Theme t = theme[i];
        if (zmode[i] != M_LAND || t == TH_TUNNEL || t == TH_COUNTRY || t == TH_VOLCANO || t == TH_ICE) continue;
        bool corner = curv[i] > 0.005f;
        if (!corner && GetRandomValue(0, 100) < 55) continue;
        float side = GetRandomValue(0, 1) ? 1.0f : -1.0f;
        int n = corner ? 4 : 2;
        for (int k = 0; k < n; k++) {
            Dyn d; int j = (i + k) % N;
            d.home = add(track[j], mul(norv[j], side * (hw[j] - 0.8f - k % 2 * 1.2f))); d.home.y = th[j];
            d.p = d.home; d.v = vec(0, 0, 0); d.yaw = frand(0, 6.28f); d.rot = 0; d.spin = 0; d.t = 0; d.hit = false;
            if (t == TH_BEACH) { d.type = GetRandomValue(0, 1) ? 2 : 3; }
            else if (t == TH_BACKROOMS || t == TH_STATION) d.type = 4;
            else if (t == TH_DESERT) d.type = 2;
            else d.type = GetRandomValue(0, 2) == 0 ? 1 : 0;
            Color bc[4] = {C(230, 60, 50), C(40, 120, 230), C(255, 210, 40), C(60, 200, 110)};
            d.c = d.type == 1 ? bc[GetRandomValue(0, 1)] : bc[GetRandomValue(0, 3)];
            dyns.push_back(d);
        }
    }
    buildShortcuts();
    SetRandomSeed((unsigned)time(NULL));
}

// =====================================================================
//                 AUDIO: engine synth, music, effects
// =====================================================================
static bool audioOK = false;
static AudioStream engStream;
static Sound sBeep, sGo, sCrash, sShot, sWhoosh, sChime, sThunder, sClick, sLap, sScrape;
static Sound sCoin, sTick, sGet, sRocket, sBoom, sZap, sShield, sSpin, sCash, sTrick;
static const int AUD_RATE = 44100, AUD_BUF = 2048;
static short audBuf[AUD_BUF];
struct EngineParams { float rpm = 0, thr = 0, speed = 0, boost = 0, rain = 0, master = 0, siren = 0, hum = 0, walk = 0; int type = 0; int musicStyle = 0; };
static EngineParams aP;
static float aRpm = 0, aThr = 0, aPh1 = 0, aPh2 = 0, aPh3 = 0, aLp = 0, aLp2 = 0, aWind = 0, aRainLp = 0, aMaster = 0, aHiss = 0, aHissLp = 0, aClank = 0;
static float aSirenPh = 0, aSirenT = 0, aHumPh = 0, aMusicGain = 0;
static unsigned aSeed = 12345;
static bool ownMusicPlaying = false;     // a track from the music folder is playing: mute the built-in sequencer
static inline float nrand() { aSeed = aSeed * 1664525u + 1013904223u; return ((aSeed >> 9) & 0x7FFF) / 16384.0f - 1.0f; }
static inline float midiF(int m) { return 440.0f * powf(2.0f, (m - 69) / 12.0f); }

// ---------------- music sequencer ----------------
struct MusicState {
    double stepPos = 0; int step = -1;
    float kEnv = 0, kPh = 0, sEnv = 0, hEnv = 0, hLp = 0, bPh = 0, bEnv = 0, bFreq = 110, bLp = 0, lPh = 0, lEnv = 0, lFreq = 440, lLp = 0;
    float padPh[3] = {0, 0, 0}, padF[3] = {220, 261, 330}, padLp = 0;
};
static MusicState MS;
static const int PROG[4] = {45, 41, 48, 43};          // A F C G roots
static const int CHORD_MINOR[4] = {1, 0, 0, 0};
static float musicSample(int style) {
    if (style == 2) return 0;
    float bpm = style == 1 ? 128.0f : 98.0f;
    double spStep = AUD_RATE * 60.0 / bpm / 4.0;
    MS.stepPos += 1.0;
    if (MS.stepPos >= spStep || MS.step < 0) {
        MS.stepPos = 0; MS.step = (MS.step + 1) % 64;
        int st = MS.step % 16, ch = (MS.step / 16) % 4, root = PROG[ch];
        int third = CHORD_MINOR[ch] ? 3 : 4;
        int tones[3] = {0, third, 7};
        if (style == 1) {
            if (st % 4 == 0) { MS.kEnv = 1; MS.kPh = 0; }
            if (st == 4 || st == 12) MS.sEnv = 1;
            MS.hEnv = (st % 4 == 2) ? 0.9f : 0.35f;
            if (st % 2 == 0) { MS.bFreq = midiF(root + ((st / 2) % 2 ? 12 : 0)); MS.bEnv = 1; }
            static const int arp[16] = {0, 1, 2, 1, 0, 2, 1, 2, 0, 1, 2, 1, 2, 1, 0, 2};
            int oct = (st >= 8) ? 24 : 12;
            MS.lFreq = midiF(root + oct + tones[arp[st]]); MS.lEnv = 1;
        } else {
            if (st == 0 || st == 8) { MS.kEnv = 0.6f; MS.kPh = 0; }
            if (st % 4 == 0) { for (int k = 0; k < 3; k++) MS.padF[k] = midiF(root + 12 + tones[k]); }
            if (st % 2 == 0) { static const int arp2[8] = {0, 2, 1, 2, 0, 1, 2, 1}; MS.lFreq = midiF(root + 24 + tones[arp2[(st / 2) % 8]]); MS.lEnv = 0.7f; }
            if (st % 8 == 0) { MS.bFreq = midiF(root); MS.bEnv = 1; }
        }
    }
    float s = 0;
    if (MS.kEnv > 0.001f) { float f = 48 + 110 * MS.kEnv * MS.kEnv; MS.kPh += f / AUD_RATE; s += sinf(MS.kPh * 2 * PI_F) * MS.kEnv * 0.85f; MS.kEnv *= 0.99925f; }
    if (MS.sEnv > 0.001f) { s += nrand() * MS.sEnv * 0.28f; MS.sEnv *= 0.9988f; }
    if (MS.hEnv > 0.001f) { float n = nrand(); MS.hLp += (n - MS.hLp) * 0.35f; s += (n - MS.hLp) * MS.hEnv * 0.10f; MS.hEnv *= 0.9965f; }
    MS.bPh += MS.bFreq / AUD_RATE; if (MS.bPh > 1) MS.bPh -= 1;
    MS.bLp += ((MS.bPh * 2 - 1) - MS.bLp) * (style == 1 ? 0.07f : 0.04f);
    s += MS.bLp * MS.bEnv * 0.32f; MS.bEnv *= style == 1 ? 0.99988f : 0.99996f;
    MS.lPh += MS.lFreq / AUD_RATE; if (MS.lPh > 1) MS.lPh -= 1;
    MS.lLp += ((MS.lPh < 0.5f ? 1.0f : -1.0f) - MS.lLp) * 0.10f;
    s += MS.lLp * MS.lEnv * (style == 1 ? 0.09f : 0.07f); MS.lEnv *= 0.99975f;
    if (style == 0) {
        float p = 0;
        for (int k = 0; k < 3; k++) { MS.padPh[k] += MS.padF[k] * (1.0f + 0.003f * k) / AUD_RATE; if (MS.padPh[k] > 1) MS.padPh[k] -= 1; p += MS.padPh[k] * 2 - 1; }
        MS.padLp += (p - MS.padLp) * 0.02f; s += MS.padLp * 0.07f;
    }
    return s;
}

static void fillEngine() {
    float musicVol = S.music / 10.0f;
    for (int i = 0; i < AUD_BUF; i++) {
        aRpm += (aP.rpm - aRpm) * 0.0016f; aThr += (aP.thr - aThr) * 0.002f; aMaster += (aP.master - aMaster) * 0.0008f;
        float s = 0, f;
        switch (aP.type) {
            case 1: {
                f = 13 + aRpm * 42; aPh1 += f / AUD_RATE; if (aPh1 > 1) aPh1 -= 1;
                float w = tanhf(sinf(aPh1 * 2 * PI_F) * 3.0f) * 0.55f + (aPh1 * 2 - 1) * 0.3f;
                aLp += (w - aLp) * 0.06f; s = aLp * (0.5f + 0.5f * aThr);
                aPh3 += (aP.speed * 0.9f) / AUD_RATE; if (aPh3 > 1) { aPh3 -= 1; aClank = 1; }
                aClank *= 0.995f; s += nrand() * aClank * 0.25f;
            } break;
            case 2: {
                f = 160 + aP.speed * 14; aPh1 += f / AUD_RATE; if (aPh1 > 1) aPh1 -= 1;
                s = sinf(aPh1 * 2 * PI_F) * 0.16f * clampf(aP.speed / 8.0f, 0.15f, 1.0f);
                aPh2 += f * 1.5f / AUD_RATE; if (aPh2 > 1) aPh2 -= 1; s += sinf(aPh2 * 2 * PI_F) * 0.06f;
                float st = fmodf(aP.walk * 1.6f / PI_F, 1.0f);
                if (st < 0.04f && aP.speed > 2) s += sinf(st * 900) * (1 - st / 0.04f) * 0.4f;
            } break;
            case 3: {
                f = 32 + aRpm * 115; aPh1 += f / AUD_RATE; if (aPh1 > 1) aPh1 -= 1;
                float w = (aPh1 * 2 - 1) * 0.6f + (aPh1 < 0.3f ? 0.4f : -0.1f);
                aLp += (w - aLp) * (0.18f + 0.25f * aThr); s = aLp * (0.45f + 0.4f * aThr);
                aLp2 += (nrand() - aLp2) * 0.15f; s += aLp2 * clampf(aP.speed / 40.0f, 0, 1) * 0.35f;
            } break;
            case 4: {
                f = 42 + aRpm * 60; aPh1 += f / AUD_RATE; if (aPh1 > 1) aPh1 -= 1;
                float w = (aPh1 * 2 - 1); aLp += (w - aLp) * 0.09f;
                aPh2 += (f * 0.25f) / AUD_RATE; if (aPh2 > 1) aPh2 -= 1;
                s = aLp * 0.55f * (0.75f + 0.25f * sinf(aPh2 * 2 * PI_F));
            } break;
            case 5: {   // kart: buzzy single cylinder
                f = 45 + aRpm * 190; aPh1 += f / AUD_RATE; if (aPh1 > 1) aPh1 -= 1;
                float w = (aPh1 < 0.25f ? 1.0f : -0.33f) + (aPh1 * 2 - 1) * 0.3f;
                aLp += (w - aLp) * (0.2f + 0.3f * aThr); s = aLp * (0.35f + 0.4f * aThr);
            } break;
            default: {
                f = 26 + aRpm * 150; aPh1 += f / AUD_RATE; if (aPh1 > 1) aPh1 -= 1;
                aPh2 += f * 0.5f / AUD_RATE; if (aPh2 > 1) aPh2 -= 1;
                float w = (aPh1 * 2 - 1) * 0.45f + sinf(aPh2 * 2 * PI_F) * 0.35f + (aPh1 < 0.12f ? 0.5f : 0.0f) + nrand() * 0.05f * aThr;
                aLp += (w - aLp) * (0.10f + 0.35f * aThr); aLp2 += (aLp - aLp2) * 0.5f;
                s = aLp2 * (0.45f + 0.45f * aThr);
            } break;
        }
        float sp = clampf(aP.speed / 90.0f, 0, 1.3f);
        aWind += (nrand() - aWind) * (0.02f + sp * 0.06f);
        s += aWind * sp * sp * 0.45f;
        aHissLp += (nrand() - aHissLp) * 0.6f; aHiss += (aP.boost - aHiss) * 0.001f;
        s += (nrand() - aHissLp) * aHiss * 0.12f;
        aRainLp += (nrand() - aRainLp) * 0.35f; s += aRainLp * aP.rain * 0.10f;
        s *= aMaster;
        // police siren (wail)
        if (aP.siren > 0.01f) {
            aSirenT += 1.0f / AUD_RATE;
            float sf = 720 + 330 * sinf(aSirenT * 2 * PI_F * 0.45f);
            aSirenPh += sf / AUD_RATE; if (aSirenPh > 1) aSirenPh -= 1;
            s += (sinf(aSirenPh * 2 * PI_F) * 0.7f + (aSirenPh < 0.5f ? 0.15f : -0.15f)) * aP.siren * 0.16f;
        }
        // fluorescent hum (backrooms)
        if (aP.hum > 0.01f) {
            aHumPh += 60.0f / AUD_RATE; if (aHumPh > 1) aHumPh -= 1;
            s += (sinf(aHumPh * 2 * PI_F) * 0.5f + sinf(aHumPh * 4 * PI_F) * 0.35f + sinf(aHumPh * 6 * PI_F) * 0.15f + nrand() * 0.05f) * aP.hum * 0.07f;
        }
        aMusicGain += ((S.sound && !ownMusicPlaying ? musicVol : 0.0f) - aMusicGain) * 0.0005f;
        if (aMusicGain > 0.002f) s += musicSample(aP.musicStyle) * aMusicGain * 0.55f;
        audBuf[i] = (short)(clampf(s, -1, 1) * 26000.0f);
    }
}
template <class F> static Sound synth(float dur, F f) {
    int n = (int)(dur * AUD_RATE);
    std::vector<short> data(n);
    float st[4] = {0, 0, 0, 0};
    for (int i = 0; i < n; i++) data[i] = (short)(clampf(f((float)i / AUD_RATE, st), -1, 1) * 28000.0f);
    Wave w; w.frameCount = (unsigned)n; w.sampleRate = AUD_RATE; w.sampleSize = 16; w.channels = 1; w.data = data.data();
    return LoadSoundFromWave(w);
}
static void initAudio() {
    InitAudioDevice();
    audioOK = IsAudioDeviceReady();
    if (!audioOK) return;
    SetAudioStreamBufferSizeDefault(AUD_BUF);
    engStream = LoadAudioStream(AUD_RATE, 16, 1);
    PlayAudioStream(engStream);
    sBeep = synth(0.18f, [](float t, float*) { return sinf(2 * PI_F * 880 * t) * 0.5f * (1 - t / 0.18f); });
    sGo = synth(0.6f, [](float t, float*) { return (sinf(2 * PI_F * 1320 * t) + 0.5f * sinf(2 * PI_F * 1980 * t)) * 0.4f * (1 - t / 0.6f); });
    sCrash = synth(0.7f, [](float t, float* s) { s[0] += (nrand() - s[0]) * 0.25f; return s[0] * 1.6f * expf(-t * 6) + sinf(2 * PI_F * 55 * t) * expf(-t * 9) * 0.7f; });
    sScrape = synth(0.35f, [](float t, float* s) { s[0] += (nrand() - s[0]) * 0.7f; return (nrand() - s[0]) * 0.6f * sinf(PI_F * t / 0.35f); });
    sShot = synth(0.8f, [](float t, float* s) { s[1] += (90 - 60 * t) / AUD_RATE; s[0] += (nrand() - s[0]) * 0.3f;
        return sinf(2 * PI_F * s[1]) * expf(-t * 5) * 0.9f + s[0] * expf(-t * 10) * 1.2f; });
    sWhoosh = synth(1.0f, [](float t, float* s) { float c = 0.02f + 0.3f * sinf(PI_F * t); s[0] += (nrand() - s[0]) * c; return s[0] * 1.4f * sinf(PI_F * t); });
    sChime = synth(0.6f, [](float t, float*) { float a = sinf(2 * PI_F * 1046 * t) * expf(-t * 7);
        float b = t > 0.08f ? sinf(2 * PI_F * 1568 * (t - 0.08f)) * expf(-(t - 0.08f) * 6) : 0; return (a + b) * 0.35f; });
    sThunder = synth(3.0f, [](float t, float* s) { s[0] += (nrand() - s[0]) * 0.02f; s[1] += (nrand() - s[1]) * 0.2f;
        float env = t < 0.05f ? t / 0.05f : expf(-(t - 0.05f) * 1.2f); return (s[0] * 5.0f + s[1] * 0.4f * expf(-t * 4)) * env; });
    sClick = synth(0.04f, [](float t, float*) { return sinf(2 * PI_F * 1800 * t) * 0.35f * (1 - t / 0.04f); });
    sLap = synth(0.5f, [](float t, float*) { float f = t < 0.18f ? 784.0f : 1175.0f; return sinf(2 * PI_F * f * t) * 0.35f * (1 - t / 0.5f); });
    sCoin = synth(0.25f, [](float t, float*) { float f = t < 0.06f ? 1976.0f : 2637.0f; return ((sinf(2 * PI_F * f * t) > 0) ? 0.22f : -0.22f) * (1 - t / 0.25f); });
    sTick = synth(0.03f, [](float t, float*) { return ((sinf(2 * PI_F * 1200 * t) > 0) ? 0.2f : -0.2f) * (1 - t / 0.03f); });
    sGet = synth(0.35f, [](float t, float*) { float f = 660.0f * powf(2.0f, floorf(t / 0.07f) * 4 / 12.0f); return ((sinf(2 * PI_F * f * t) > 0) ? 0.2f : -0.2f) * (1 - t / 0.35f); });
    sRocket = synth(0.9f, [](float t, float* s) { s[0] += (nrand() - s[0]) * (0.1f + 0.4f * t); return s[0] * 1.2f * expf(-t * 2.5f); });
    sBoom = synth(1.2f, [](float t, float* s) { s[0] += (nrand() - s[0]) * 0.08f; s[1] += (30 + 40 * expf(-t * 8)) / AUD_RATE;
        return (s[0] * 2.5f + sinf(2 * PI_F * s[1]) * 0.8f) * expf(-t * 3.5f); });
    sZap = synth(0.7f, [](float t, float* s) { s[0] += (nrand() - s[0]) * 0.9f; float buzz = sinf(2 * PI_F * 120 * t) > 0 ? 1.0f : -1.0f;
        return (s[0] * 0.6f + buzz * 0.2f) * expf(-t * 4) * (0.6f + 0.4f * sinf(t * 90)); });
    sShield = synth(0.6f, [](float t, float* s) { s[0] += (300 + 900 * t) / AUD_RATE; return sinf(2 * PI_F * s[0]) * 0.3f * sinf(PI_F * t / 0.6f); });
    sSpin = synth(0.7f, [](float t, float* s) { s[0] += (1400 - 1600 * t) / AUD_RATE; return sinf(2 * PI_F * s[0]) * 0.25f * (1 - t / 0.7f); });
    sCash = synth(0.5f, [](float t, float*) { float a = sinf(2 * PI_F * 2093 * t) * expf(-t * 9), b = t > 0.1f ? sinf(2 * PI_F * 2637 * (t - 0.1f)) * expf(-(t - 0.1f) * 7) : 0; return (a + b) * 0.3f; });
    sTrick = synth(0.45f, [](float t, float* s) { s[0] += (500 + 1500 * t) / AUD_RATE; return ((sinf(2 * PI_F * s[0]) > 0) ? 0.18f : -0.18f) * (1 - t / 0.45f); });
}
static void play(Sound& s, float vol = 1.0f, float pitch = 1.0f) {
    if (!audioOK || !S.sound) return;
    SetSoundVolume(s, vol); SetSoundPitch(s, pitch); PlaySound(s);
}
static void updateAudio() {
    if (!audioOK) return;
    if (!S.sound) { aP.master = 0; aP.siren = 0; aP.hum = 0; }
    while (IsAudioStreamProcessed(engStream)) { fillEngine(); UpdateAudioStream(engStream, audBuf, AUD_BUF); }
}

// ------------------------------ own music (music folder) ------------------------------
static Music curMusic; static bool musicLoaded = false;
static std::vector<std::string> musicFiles; static int musicIdx = -1; static float musicRetryT = 0;
static void scanMusic() {
    musicFiles.clear();
    std::string dirs[2] = {std::string(GetApplicationDirectory()) + "music", "music"};
    for (const std::string& d : dirs) {
        if (!DirectoryExists(d.c_str())) continue;
        FilePathList fl = LoadDirectoryFiles(d.c_str());
        for (unsigned i = 0; i < fl.count; i++) if (IsFileExtension(fl.paths[i], ".mp3;.ogg;.wav;.qoa;.xm;.mod;.MP3;.OGG;.WAV")) musicFiles.push_back(fl.paths[i]);
        UnloadDirectoryFiles(fl);
        if (!musicFiles.empty()) break;
    }
    std::sort(musicFiles.begin(), musicFiles.end());
}
static void stopOwnMusic() {
    if (musicLoaded) { StopMusicStream(curMusic); UnloadMusicStream(curMusic); musicLoaded = false; }
    ownMusicPlaying = false;
}
static void nextSong() {
    stopOwnMusic();
    if (musicFiles.empty()) scanMusic();
    if (musicFiles.empty()) return;
    for (int tries = 0; tries < (int)musicFiles.size() && !musicLoaded; tries++) {
        musicIdx = (musicIdx + 1) % (int)musicFiles.size();
        curMusic = LoadMusicStream(musicFiles[musicIdx].c_str());
        if (curMusic.frameCount > 0) {
            curMusic.looping = false; PlayMusicStream(curMusic); musicLoaded = true;
            notify(std::string(T("\320\234\320\243\320\227\320\253\320\232\320\220: ", "MUSIC: ")) + GetFileNameWithoutExt(musicFiles[musicIdx].c_str()), C(200, 170, 255), 3.0f);
        }
    }
}
static void updateOwnMusic(float dt) {
    bool want = audioOK && S.sound && S.musicSrc;
    if (!want) { stopOwnMusic(); musicRetryT = 0; return; }
    if (!musicLoaded) {
        musicRetryT -= dt;
        if (musicRetryT <= 0) {
            nextSong(); musicRetryT = 8.0f;
            if (!musicLoaded) notify(T("\320\237\320\260\320\277\320\272\320\260 music \320\277\321\203\321\201\321\202\320\260 \342\200\224 \320\270\320\263\321\200\320\260\320\265\321\202 \320\262\321\201\321\202\321\200\320\276\320\265\320\275\320\275\320\260\321\217 \320\274\321\203\320\267\321\213\320\272\320\260", "The music folder is empty - built-in music plays"), C(255, 180, 120), 3.0f);
        }
        ownMusicPlaying = musicLoaded;
        return;
    }
    ownMusicPlaying = true;
    SetMusicVolume(curMusic, S.music / 10.0f);
    UpdateMusicStream(curMusic);
    if (!IsMusicStreamPlaying(curMusic) || GetMusicTimePlayed(curMusic) >= GetMusicTimeLength(curMusic) - 0.05f) nextSong();
}

// =====================================================================
//                 ATMOSPHERE: TIME OF DAY + WEATHER
// =====================================================================
static Color tSkyTop, tSkyHor, tFog, tAmb, tSun;
static float tFogS = 150, tFogE = 900;
static bool atmoInit = false;

static void computeAtmoTarget() {
    Color st, sh, fg, am, sn; float fs, fe;
    if (indoorTrack()) {
        tSkyTop = tSkyHor = tFog = C(62, 56, 30); tAmb = C(215, 205, 160); tSun = C(90, 85, 60);
        tFogS = 20; tFogE = 200; lightDir = norm(vec(0.2f, 1.0f, 0.1f)); nightK = 0.0f;
        return;
    }
    if (spaceTrack()) {   // deep space: black sky, hard white sun, neon lights always on
        tSkyTop = C(2, 3, 12); tSkyHor = C(14, 12, 40); tFog = C(6, 6, 20); tAmb = C(110, 118, 160); tSun = C(255, 248, 235);
        tFogS = 380; tFogE = 1500; lightDir = norm(vec(-0.5f, 0.7f, 0.35f)); nightK = 0.8f;
        return;
    }
    if (S.tod == TD_NIGHT) { st = C(6, 8, 24); sh = C(42, 30, 70); fg = C(26, 24, 46); am = C(78, 84, 120); sn = C(110, 130, 190); fs = 120; fe = 820; lightDir = norm(vec(0.3f, 0.7f, 0.4f)); nightK = 1.0f; }
    else if (S.tod == TD_SUNSET) { st = C(38, 58, 130); sh = C(255, 138, 80); fg = C(214, 128, 104); am = C(150, 118, 130); sn = C(255, 175, 115); fs = 160; fe = 950; lightDir = norm(vec(-0.85f, 0.28f, 0.3f)); nightK = 0.5f; }
    else { st = C(58, 118, 220); sh = C(185, 210, 240); fg = C(188, 208, 234); am = C(150, 160, 178); sn = C(255, 246, 228); fs = 220; fe = 980; lightDir = norm(vec(-0.4f, 0.85f, -0.3f)); nightK = 0.0f; }
    if (curTrack == 1 && S.tod == TD_DAY) { st = C(30, 125, 240); sh = C(165, 220, 250); fg = C(175, 215, 240); am = C(160, 168, 180); }
    if (curTrack == 3) {   // cold air, snow bounce light
        sh = mixC(sh, C(215, 228, 245), 0.35f * (1 - nightK) + 0.1f); fg = mixC(fg, shade(C(220, 230, 245), 1.0f - 0.75f * nightK), 0.45f);
        am = mixC(am, C(185, 195, 215), 0.4f); if (nightK > 0.5f) am = C(95, 105, 140);
    }
    if (curTrack == 4) {   // volcanic haze: red-orange sky, warm fog
        st = mixC(st, shade(C(70, 25, 18), 1.0f - 0.5f * nightK), 0.55f); sh = mixC(sh, shade(C(255, 120, 60), 1.0f - 0.55f * nightK), 0.55f);
        fg = mixC(fg, shade(C(170, 95, 70), 1.0f - 0.6f * nightK), 0.6f); am = mixC(am, C(170, 120, 100), 0.35f); sn = mixC(sn, C(255, 170, 120), 0.4f);
        fe = std::min(fe, 820.0f);
    }
    float k = wI;
    switch (curWeather) {
        case W_RAIN: st = mixC(st, shade(C(70, 76, 90), 1.0f - 0.7f * nightK), k * 0.8f); sh = mixC(sh, shade(C(120, 126, 136), 1.0f - 0.7f * nightK), k * 0.8f);
                     fg = mixC(fg, shade(C(110, 116, 126), 1.0f - 0.72f * nightK), k * 0.85f); am = shade(am, 1.0f - 0.25f * k); sn = shade(sn, 1.0f - 0.5f * k); fe = lerpf(fe, 480, k); fs = lerpf(fs, 60, k); break;
        case W_STORM: st = mixC(st, shade(C(30, 34, 46), 1.0f - 0.5f * nightK), k); sh = mixC(sh, shade(C(66, 72, 86), 1.0f - 0.6f * nightK), k);
                      fg = mixC(fg, shade(C(60, 66, 78), 1.0f - 0.6f * nightK), k); am = shade(am, 1.0f - 0.35f * k); sn = shade(sn, 1.0f - 0.6f * k); fe = lerpf(fe, 400, k); fs = lerpf(fs, 40, k); break;
        case W_FOG: sh = mixC(sh, shade(C(196, 200, 206), 1.0f - 0.78f * nightK), k); st = mixC(st, sh, k * 0.7f); fg = mixC(fg, shade(C(196, 200, 206), 1.0f - 0.78f * nightK), k);
                    sn = shade(sn, 1.0f - 0.4f * k); fe = lerpf(fe, 170, k); fs = lerpf(fs, 5, k); break;
        case W_SNOW: st = mixC(st, shade(C(150, 160, 180), 1.0f - 0.75f * nightK), k * 0.8f); sh = mixC(sh, shade(C(215, 222, 232), 1.0f - 0.75f * nightK), k * 0.9f);
                     fg = mixC(fg, shade(C(205, 212, 224), 1.0f - 0.75f * nightK), k); am = mixC(am, C(170, 175, 190), 0.3f * k); fe = lerpf(fe, 330, k); fs = lerpf(fs, 20, k); break;
        default: break;
    }
    tSkyTop = st; tSkyHor = sh; tFog = fg; tAmb = am; tSun = sn; tFogS = fs; tFogE = fe;
}
static void updateAtmosphere(float dt, bool instant) {
    computeAtmoTarget();
    float k = instant || !atmoInit ? 1.0f : clampf(dt * 0.8f, 0, 1);
    atmoInit = true;
    skyTop = mixC(skyTop, tSkyTop, k); skyHor = mixC(skyHor, tSkyHor, k); fogCol = mixC(fogCol, tFog, k);
    ambCol = mixC(ambCol, tAmb, k); sunCol = mixC(sunCol, tSun, k);
    fogS = lerpf(fogS, tFogS, k); fogE = lerpf(fogE, tFogE, k);
    float wt = (curWeather == W_RAIN || curWeather == W_STORM) ? wI : 0.0f;
    wetK = lerpf(wetK, wt, k); snowK = lerpf(snowK, curWeather == W_SNOW ? wI : 0.0f, k);
    waveAmp = lerpf(waveAmp, curWeather == W_STORM ? 1.6f * wI + 0.4f : curWeather == W_RAIN ? 0.9f : (curTrack == 2 ? 0.15f : 0.4f), k);
}
static float fogF(float d) { return smooth01((d - fogS) / (fogE - fogS)); }

static float turbulence() {
    float k = curWeather == W_STORM ? 1.0f : (curWeather == W_RAIN || curWeather == W_SNOW) ? 0.3f : 0.0f;
    return k * wI;
}
static float weatherGrip(Base b, Mode m) {
    float g = 1.0f;
    bool wheels = b == B_CAR || b == B_KART;
    if (m == M_LAND) {
        switch (curWeather) {
            case W_RAIN:  g = wheels ? 0.70f : b == B_TANK ? 0.88f : 0.9f; break;
            case W_SNOW:  g = wheels ? 0.45f : b == B_TANK ? 0.78f : 0.82f; break;
            case W_STORM: g = wheels ? 0.60f : b == B_TANK ? 0.84f : 0.86f; break;
            case W_FOG:   g = 0.96f; break;
            default: break;
        }
    } else if (m == M_WATER) {
        if (curWeather == W_STORM) g = 0.8f; else if (curWeather == W_RAIN) g = 0.92f;
    }
    return lerpf(1.0f, g, wI);
}
// extra grip factor from the surface itself (ice lake, packed snow)
static float trackGrip(Base b, Theme t, Mode m) {
    if (m != M_LAND) return 1.0f;
    bool wheels = b == B_CAR || b == B_KART;
    if (t == TH_ICE) return wheels ? 0.42f : b == B_TANK ? 0.62f : 0.58f;
    if (t == TH_SNOW) return wheels ? 0.86f : 0.95f;
    return 1.0f;
}
static float gravK() { return spaceTrack() ? 0.42f : 1.0f; }
static float waveH(float x, float z, float t) {
    return waveAmp * (sinf(x * 0.08f + t * 1.6f) * 0.5f + sinf(z * 0.11f - t * 1.2f) * 0.35f + sinf((x + z) * 0.05f + t * 0.9f) * 0.3f);
}
static bool noWeatherTrack() { return indoorTrack() || spaceTrack(); }
// weather that makes sense on each track
static bool weatherFits(Weather w) {
    if (noWeatherTrack()) return w == W_CLEAR;
    if (curTrack == 3) return w == W_CLEAR || w == W_SNOW || w == W_FOG || w == W_STORM;
    if (curTrack == 4) return w == W_CLEAR || w == W_FOG || w == W_STORM;
    if (curTrack == 1) return w != W_SNOW;
    return true;
}
static void setWeather(Weather w, bool announce) {
    if (noWeatherTrack()) w = W_CLEAR;
    curWeather = w; wI = 0;
    weatherTimer = frand(45, 75);
    if (announce) notify(std::string(T("\320\237\320\236\320\223\320\236\320\224\320\220: ", "WEATHER: ")) + weatherName(w), C(140, 200, 255), 3.0f);
}
static void updateWeather(float dt) {
    if (S.weather == 0 && !noWeatherTrack()) {
        weatherTimer -= dt;
        if (weatherTimer <= 0) {
            Weather w; int guard = 0;
            do { w = (Weather)GetRandomValue(0, W_COUNT - 1); guard++; } while ((w == curWeather || !weatherFits(w)) && guard < 50);
            setWeather(w, true);
        }
    }
    wI = std::min(1.0f, wI + dt / 6.0f);
    if (curWeather == W_STORM && wI > 0.5f && GetRandomValue(0, 1000) < 4) { flashT = 1.0f; thunderT = frand(0.4f, 2.0f); }
    if (thunderT > 0) { thunderT -= dt; if (thunderT <= 0) play(sThunder, 0.9f, frand(0.8f, 1.1f)); }
    flashT = std::max(0.0f, flashT - dt * 3.0f);
}

// =====================================================================
//                         RACER BASICS
// =====================================================================
static inline V3 fwdV(float yaw) { return vec(sinf(yaw), 0, cosf(yaw)); }
static inline V3 rgtV(float yaw) { return vec(-cosf(yaw), 0, sinf(yaw)); }
static inline V3 fwd3(const Racer& r) { return vec(sinf(r.yaw) * cosf(r.pitch), sinf(r.pitch), cosf(r.yaw) * cosf(r.pitch)); }
static inline float radiusOf(const Racer& r) { return ST[r.base][r.mode].rad * (r.shrinkT > 0 ? 0.65f : 1.0f); }
static inline float massOf(const Racer& r) { return ST[r.base][r.mode].mass * (r.shrinkT > 0 ? 0.5f : 1.0f); }
static inline float armorK(const Racer& r) { return (r.base == B_TANK ? 0.45f : r.base == B_ROBOT ? 0.75f : r.base == B_KART ? 1.1f : 1.0f) * r.upArmor; }
static inline float speedOf(const Racer& r) { return r.mode == M_AIR ? r.airSpeed : len(vec(r.vel.x, 0, r.vel.z)); }
static inline bool active(const Racer& r) { return !r.out; }

static void onLapComplete(Racer& r, float lapTime);    // defined with the game modes
static void onRacerWrecked(Racer& r);
static void popBalloon(Racer& v, int attacker);         // battle mode

// ---- per-player feedback: notifications, camera shake, gamepad rumble ----
static int padOfPlayer(int pid) {
    if (pid < 0) return -1;
    bool p0 = IsGamepadAvailable(0), p1 = IsGamepadAvailable(1);
    if (numHumans < 2) return p0 ? 0 : -1;
    if (p0 && p1) return pid;
    if (p0) return pid == 1 ? 0 : -1;       // one pad: player 2 gets it, player 1 uses the keyboard
    return -1;
}
static void rumblePad(int pid, float strength, float dur) {
    if (!S.vibration || pid < 0) return;
    int pad = padOfPlayer(pid);
    if (pad < 0) return;
#if defined(RAYLIB_VERSION_MAJOR) && defined(RAYLIB_VERSION_MINOR)
#if (RAYLIB_VERSION_MAJOR > 5) || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 5)
    strength = clampf(strength, 0, 1);
    SetGamepadVibration(pad, strength, strength * 0.7f, dur);
#endif
#endif
    (void)strength; (void)dur;
}
static void shakeR(const Racer& r, float amt) {
    if (r.pid < 0) return;
    shakeP[r.pid & 1] = std::max(shakeP[r.pid & 1], amt);
    rumblePad(r.pid, amt, 0.08f + amt * 0.25f);
}
static void notifyP(const Racer& r, const std::string& s, Color c = C(255, 255, 255), float t = 2.6f) {
    if (r.pid < 0) return;
    notify(s, c, t, numHumans > 1 ? r.pid : -1);
}

// road surface height under a world position (interpolated along the segment)
static float roadHeight(V3 p, int idx) {
    int i = idx, j = (idx + 1) % N;
    V3 d = sub(p, track[i]);
    float segL = std::max(0.1f, dist2D(track[j], track[i]));
    float t = dot(vec(d.x, 0, d.z), tanv[i]) / segL;
    int a = i, b = j;
    if (t < 0) { a = (i + N - 1) % N; b = i; t += 1; }
    t = clampf(t, 0, 1);
    float lat = d.x * norv[i].x + d.z * norv[i].z;
    return lerpf(hAtLat(a, lat), hAtLat(b, lat), t);
}
static float rampHeight(V3 p) {
    float best = 0;
    for (const Ramp& r : ramps) {
        float dx = p.x - r.p.x, dz = p.z - r.p.z;
        if (dx * dx + dz * dz > 400) continue;
        V3 f = fwdV(r.yaw), rt = rgtV(r.yaw);
        V3 d = vec(dx, 0, dz);
        float lz = dot(d, f), lx = dot(d, rt);
        if (fabsf(lx) < r.w / 2 && lz > -r.len / 2 && lz < r.len / 2) best = std::max(best, r.h * (lz + r.len / 2) / r.len);
    }
    return best;
}
static float groundHeight(const Racer& r) { return roadHeight(r.pos, r.lastIdx) + rampHeight(r.pos); }

static void killRacer(Racer& r) {
    r.dead = true; r.deadTimer = 1.8f;
    burst(r.pos, 40, 30, C(255, 150, 40), 1.0f, 1.4f, true, true);
    burst(r.pos, 24, 12, C(40, 40, 40, 200), 2.0f, 3.0f, false);
    r.vel = vec(0, 0, 0);
    shakeR(r, 1.0f);
    r.deaths++;
    if (dist2D(r.pos, camPos) < 200) play(sBoom, 0.9f, 0.9f);
    onRacerWrecked(r);
}
static void damage(Racer& r, float d, int attacker = -1) {
    if (attacker >= 0 && attacker != r.id) { r.lastHitBy = attacker; r.lastHitT = gTime; }
    if (r.dead || r.invuln > 0 || r.hitCd > 0 || d <= 0.5f) return;
    if (r.shieldT > 0) return;
    d *= armorK(r);
    if (r.boss) d *= 0.55f;
    r.hp -= d; r.hitCd = 0.15f; r.hurtT = 0.5f; r.regenT = 3.0f;
    shakeR(r, clampf(d / 25.0f, 0.1f, 0.8f));
    if (r.hp <= 0) killRacer(r);
}
static void setMode(Racer& r, Mode m) {
    if (r.mode == m) return;
    Mode old = r.mode; r.mode = m;
    r.driftCharge = 0; r.driftDir = 0;
    if (m == M_AIR) {
        V3 h = vec(r.vel.x, 0, r.vel.z);
        r.airSpeed = std::max(36.0f, len(h)); r.vel.y = 7.0f; r.pitch = 0.25f; r.grounded = false;
        burst(r.pos, 24, 18, C(140, 210, 255), 0.7f, 1.0f, false, true);
    } else {
        r.pitch = 0; r.roll = 0;
        if (old == M_AIR) r.vel.y = std::min(r.vel.y, 0.0f);
        burst(r.pos, 30, 14, m == M_WATER ? C(220, 240, 255) : C(180, 150, 100), 0.8f, 1.2f, true);
    }
    if (r.isPlayer) {
        Color c = m == M_WATER ? C(60, 180, 255) : m == M_AIR ? C(190, 120, 255) : C(255, 180, 40);
        notifyP(r, std::string(T("\320\242\320\240\320\220\320\235\320\241\320\244\320\236\320\240\320\234\320\220\320\246\320\230\320\257: ", "TRANSFORM: ")) + modeName(r.base, m), c, 3.0f);
        shakeR(r, 0.35f);
        play(sWhoosh, 0.8f);
        hintT = 5.0f;
    }
}
static void respawn(Racer& r) {
    if (r.isCop && !racers.empty()) { r.lastIdx = (racers[0].lastIdx - 50 + N) % N; r.parked = false; }
    if (!baseAllowed(r.base)) r.base = B_CAR;
    r.scIdx = -1;
    int i = ((r.lastIdx - 6) % N + N) % N;
    Mode m = zmode[i];
    r.mode = m; r.pos = track[i]; r.pos.y = m == M_AIR ? airAlt[i] + 2.0f : (m == M_WATER ? 0.0f : th[i]);
    r.yaw = atan2f(tanv[i].x, tanv[i].z); r.pitch = 0; r.roll = 0; r.steerV = 0;
    r.vel = mul(tanv[i], m == M_AIR ? 40.0f : 10.0f); r.airSpeed = 42;
    r.hp = 100; r.energy = std::max(r.energy, 50.0f); r.dead = false; r.invuln = 2.5f; r.boostLock = false; r.lastIdx = i;
    r.morphAir = m == M_AIR ? 1.0f : 0.0f; r.morphWater = m == M_WATER ? 1.0f : 0.0f; r.stuckTimer = 0; r.grounded = m != M_AIR; r.wrongT = 0;
    r.spinT = 0; r.shrinkT = 0; r.driftCharge = 0; r.trickT = 0; r.lastGround = r.pos.y;
}
static void switchBase(Racer& r, Base b) {
    if (r.base == b || r.switchCd > 0 || r.dead || r.isCop || r.boss) return;
    if (!baseAllowed(b)) { if (r.isPlayer) notifyP(r, T("\320\222 \320\232\320\236\320\241\320\234\320\236\320\241\320\225 \342\200\224 \320\242\320\236\320\233\320\254\320\232\320\236 \320\245\320\236\320\222\320\225\320\240\320\232\320\220\320\240 \320\230 \320\232\320\220\320\240\320\242", "SPACE: HOVERCAR AND KART ONLY"), C(150, 120, 255), 2.0f); return; }
    r.base = b; r.switchCd = 6.0f; r.invuln = std::max(r.invuln, 0.8f);
    burst(r.pos, 30, 20, C(120, 200, 255), 0.7f, 1.0f, false, true);
    if (r.isPlayer) { shakeR(r, 0.3f); notifyP(r, modeName(b, r.mode), C(120, 200, 255), 2.0f); play(sWhoosh, 0.6f, 1.3f); hintT = 5.0f; }
}
static float raceScore(const Racer& r) {
    if (r.out) return -100000.0f * (float)r.finishPlace;
    if (S.mode == GM_BATTLE) return r.finished ? 1e7f - r.finishPlace : r.balloons * 1000.0f + r.points * 10.0f - r.deaths * 0.5f;
    return r.finished ? 1e7f - r.finishPlace : (float)(r.lap * N + r.lastIdx);
}
static void computeOrder(int& myPos) {
    order.clear();
    for (size_t i = 0; i < racers.size(); i++) if (!racers[i].isCop) order.push_back((int)i);
    std::sort(order.begin(), order.end(), [](int a, int b) { return raceScore(racers[a]) > raceScore(racers[b]); });
    myPos = 1;
    for (size_t i = 0; i < order.size(); i++) if (order[i] == 0) myPos = (int)i + 1;
}
static int placeOf(int id) { for (size_t i = 0; i < order.size(); i++) if (order[i] == id) return (int)i + 1; return (int)order.size(); }

static void updateProgress(Racer& r) {
    int best = -1; float bd = 1e18f;
    auto test = [&](int i) {
        i = ((i % N) + N) % N;
        float dx = track[i].x - r.pos.x, dz = track[i].z - r.pos.z, d = dx * dx + dz * dz;
        if (d < bd) { bd = d; best = i; }
    };
    for (int k = -60; k <= 60; k++) test(r.lastIdx + k);
    if (sqrtf(bd) > 400) for (int i = 0; i < N; i++) test(i);
    int prev = r.lastIdx; r.lastIdx = best;
    r.distCenter = sqrtf(bd);
    V3 d = sub(r.pos, track[best]); r.lateral = d.x * norv[best].x + d.z * norv[best].z;
    if (r.isCop) return;
    int cps[3] = {N / 4, N / 2, 3 * N / 4};
    for (int c = 0; c < 3; c++) {
        int dd = std::abs(best - cps[c]); dd = std::min(dd, N - dd);
        if (dd < N / 30) r.checkMask |= (1 << c);
    }
    bool lapsMatter = S.mode == GM_RACE || S.mode == GM_TIMETRIAL;
    if (prev > N * 0.85f && best < N * 0.15f) {
        if (r.checkMask == 7) {
            r.lap++; r.checkMask = 0;
            if (r.lap >= 2) {
                r.lastLap = raceTime - r.lapStart;
                if (r.bestLap <= 0 || r.lastLap < r.bestLap) r.bestLap = r.lastLap;
                onLapComplete(r, r.lastLap);
            }
            r.lapStart = raceTime;
            if (lapsMatter && r.lap > S.laps && !r.finished) {
                r.finished = true; r.finishPlace = ++finishCount; r.finishTime = raceTime;
                if (r.isPlayer) { if (numHumans < 2 || r.pid == 0) finishBannerT = 4.0f; notifyP(r, T("\320\244\320\230\320\235\320\230\320\250!", "FINISH!"), C(120, 255, 160), 3.0f); play(sGo, 0.8f, 0.8f); }
            } else if (r.isPlayer && r.lap >= 2) {
                if (lapsMatter) notifyP(r, r.lap == S.laps ? T("\320\237\320\236\320\241\320\233\320\225\320\224\320\235\320\230\320\231 \320\232\320\240\320\243\320\223!", "FINAL LAP!") : TextFormat(T("\320\232\320\240\320\243\320\223 %d / %d", "LAP %d / %d"), r.lap, S.laps), C(255, 200, 60), 2.5f);
                else notifyP(r, TextFormat(T("\320\232\320\240\320\243\320\223 %d", "LAP %d"), r.lap), C(255, 200, 60), 2.0f);
                play(sLap, 0.7f);
            }
        }
    } else if (prev < N * 0.15f && best > N * 0.85f) {
        if (r.lap > 0) { r.lap--; r.checkMask = 7; }
    }
}
static void fireShell(Racer& r) {
    V3 f = fwd3(r);
    V3 muzzle = add(r.pos, add(mul(f, radiusOf(r) + 3.0f), vec(0, 1.9f, 0)));
    Shell s; s.p = muzzle; s.v = add(mul(f, 170.0f), mul(r.vel, 0.4f)); s.life = 1.7f; s.owner = r.id;
    shells.push_back(s);
    r.vel = sub(r.vel, mul(vec(f.x, 0, f.z), 5.0f / massOf(r)));
    burst(muzzle, 16, 22, C(255, 190, 70), 0.3f, 1.0f, false, true);
    shakeR(r, 0.25f);
    if (dist2D(r.pos, camPos) < 250) play(sShot, r.isPlayer ? 0.9f : 0.5f, frand(0.9f, 1.1f));
}
static float surfaceK(const Racer& r) {
    if (r.scIdx >= 0) return 0.9f;     // packed dirt of a shortcut
    float a = fabsf(r.lateral);
    if (a < hw[r.lastIdx]) return 1.0f;
    Theme t = theme[r.lastIdx];
    if (t == TH_CITY || t == TH_TUNNEL || t == TH_BACKROOMS) return 0.95f;
    if (t == TH_HIGHWAY) return 0.85f;
    if (t == TH_CANYON || t == TH_BEACH) return 0.78f;
    if (t == TH_STATION) return 0.95f;
    if (t == TH_ICE) return 0.9f;
    if (t == TH_SNOW || t == TH_DESERT) return 0.72f;
    if (t == TH_VOLCANO) return 0.66f;
    return 0.62f;
}

// =====================================================================
//                            PHYSICS
// =====================================================================
static inline float vmaxMul(const Racer& r) {
    float m = r.rubber * r.upV * (1.0f + 0.006f * std::min(r.coins, 10)) * (1.0f + 0.075f * r.draftK);
    if (r.shrinkT > 0) m *= 0.72f;
    return m;
}
static inline float boostK(const Racer& r) { return std::min(1.0f, (r.boosting ? 1.0f : 0.0f) + (r.boostT > 0 ? 0.8f : 0.0f)); }
static inline bool canDrift(const Racer& r) { return (r.base == B_CAR || r.base == B_KART) && r.mode != M_AIR; }
static int driftStage(const Racer& r) { return r.driftCharge > 3.6f ? 3 : r.driftCharge > 2.2f ? 2 : r.driftCharge > 1.0f ? 1 : 0; }
static Color driftColor(int st) { return st == 3 ? C(220, 90, 255) : st == 2 ? C(255, 150, 30) : C(60, 170, 255); }

// mini-turbo: hold drift while turning, release for a boost
static void updateDriftCharge(Racer& r, float dt, float vf, bool grounded) {
    if (!canDrift(r) || r.isCop) { r.driftCharge = 0; return; }
    bool drifting = r.in.special && grounded && fabsf(vf) > 14.0f && fabsf(r.steerV) > 0.25f;
    if (drifting) {
        if (r.driftDir == 0) r.driftDir = (int)sgn(r.steerV);
        r.driftCharge += dt * (0.6f + 0.7f * fabsf(r.steerV));
        int st = driftStage(r);
        if (st > 0 && GetRandomValue(0, 100) < 70) {
            V3 f = fwdV(r.yaw), rt = rgtV(r.yaw);
            for (int s = -1; s <= 1; s += 2) {
                V3 w = add(r.pos, add(add(mul(f, -1.3f), mul(rt, s * 0.9f)), vec(0, 0.3f, 0)));
                emit(w, vec(frand(-3, 3), frand(1, 4), frand(-3, 3)), 0.25f, 0.45f, driftColor(st), true, true);
            }
        }
    } else {
        int st = driftStage(r);
        if (st > 0 && !r.in.special) {
            r.boostT = std::max(r.boostT, st == 3 ? 1.3f : st == 2 ? 0.9f : 0.5f);
            if (st == 3) r.ultraTurbos++;
            burst(add(r.pos, mul(fwdV(r.yaw), -1.5f)), 12, 10, driftColor(st), 0.4f, 0.8f, false, true);
            if (r.isPlayer) { play(sWhoosh, 0.6f, 1.4f + st * 0.15f); rumblePad(r.pid, 0.25f + st * 0.1f, 0.15f); if (st == 3) notifyP(r, T("\320\241\320\243\320\237\320\225\320\240-\320\242\320\243\320\240\320\221\320\236!", "ULTRA MINI-TURBO!"), driftColor(3), 1.4f); }
        }
        if (!r.in.special) { r.driftCharge = 0; r.driftDir = 0; }
    }
}

static void physLand(Racer& r, float dt, float gripW) {
    const Stats& st = ST[r.base][M_LAND];
    const Input& in = r.in;
    float surf = surfaceK(r);
    bool onRoad = surf >= 0.99f;
    float bk = boostK(r), vm = vmaxMul(r);
    r.steerV += (in.steer - r.steerV) * clampf(9 * dt, 0, 1);
    bool g = r.grounded;
    V3 f = fwdV(r.yaw);
    float vf = dot(r.vel, f), yawRate;
    bool wheeled = r.base == B_CAR || r.base == B_KART;
    float turnK = st.turn * (1.0f + (r.upGrip - 1.0f) * 0.6f);
    if (wheeled) {
        float sp = clampf(fabsf(vf) / 8.0f, 0, 1) / (1.0f + 0.75f * powf(fabsf(vf) / 60.0f, 2));
        float drift = in.special ? (r.base == B_KART ? 1.45f : 1.55f) : 1.0f;
        yawRate = r.steerV * turnK * sp * sgn(vf) * drift * (0.65f + 0.35f * gripW * surf);
        if (in.special && r.driftDir != 0) yawRate += r.driftDir * 0.35f * sp;   // hold the drift angle
    } else if (r.base == B_TANK) {
        yawRate = r.steerV * turnK * (1.0f - 0.35f * clampf(fabsf(vf) / st.vmax, 0, 1));
    } else yawRate = r.steerV * turnK;
    r.yaw -= yawRate * (g ? 1.0f : 0.35f) * dt;

    f = fwdV(r.yaw); V3 rt = rgtV(r.yaw);
    vf = dot(r.vel, f); float vr = dot(r.vel, rt);
    float oldVf = vf;
    if (r.base != B_ROBOT) {
        if (g) {
            float vmax = st.vmax * vm * (1.0f + 0.2f * bk) * (onRoad ? 1.0f : 0.55f + 0.45f * surf);
            float acc = st.acc * r.rubber * r.upA * (1.0f + 0.6f * bk) * (0.55f + 0.45f * gripW * surf) * (r.startBoostT > 0 ? 0.2f : 1.0f);
            if (r.boostT > 0 && vf < vmax) vf += 7.0f * dt;
            if (in.throttle > 0) vf += acc * in.throttle * (1.0f - powf(clampf(vf / vmax, 0, 1), 2)) * dt;
            else if (in.throttle < 0) {
                if (vf > 0.5f) vf += in.throttle * (r.base == B_TANK ? 40.0f : 60.0f) * dt * (0.5f + 0.5f * gripW * surf);
                else { vf += in.throttle * acc * 0.6f * dt; vf = std::max(vf, -vmax * 0.3f); }
            } else vf -= sgn(vf) * std::min(fabsf(vf), (onRoad ? 2.0f : 7.0f) * dt);
            vf -= vf * (onRoad ? 0.01f : 0.25f * (1.0f - surf)) * dt;
            if (vf > vmax) vf -= (vf - vmax) * 0.8f * dt;
            float lat = st.lat * gripW * surf * (r.isPlayer ? r.upGrip : 1.0f);
            if (wheeled && in.special) { lat *= r.base == B_KART ? 0.3f : 0.18f; vf -= vf * 0.12f * dt; }
            vr *= expf(-lat * dt);
            r.slip = fabsf(vr);
            r.vel.x = f.x * vf + rt.x * vr; r.vel.z = f.z * vf + rt.z * vr;
        }
        if (r.base == B_TANK && in.special && r.specialCd <= 0 && g) { r.specialCd = 2.5f; fireShell(r); }
    } else {
        float vmax = st.vmax * vm * (1.0f + 0.22f * bk) * (onRoad ? 1.0f : 0.8f);
        V3 tgt = add(mul(f, in.throttle * vmax), mul(rt, in.strafe * vmax * 0.65f));
        if (r.boostT > 0) tgt = add(tgt, mul(f, vmax * 0.15f));
        float k = (g ? 9.0f : 1.4f) * (0.65f + 0.35f * gripW * surf);
        V3 vh = vec(r.vel.x, 0, r.vel.z);
        vh = add(vh, mul(sub(tgt, vh), clampf(k * dt, 0, 1)));
        r.vel.x = vh.x; r.vel.z = vh.z; vf = dot(vh, f); r.slip = 0;
        if (in.special && !r.prevSpecial && g && r.specialCd <= 0) { r.vel.y = 13.0f; r.grounded = false; r.specialCd = 0.45f; r.airTime = 0; r.trickDone = false; burst(r.pos, 10, 10, C(190, 170, 120), 0.5f, 1.0f); }
        if (in.special2 && r.special2Cd <= 0) {
            V3 dir = len(tgt) > 1 ? norm(tgt) : f;
            r.vel = add(r.vel, mul(dir, 24.0f)); r.special2Cd = 2.0f; r.invuln = std::max(r.invuln, 0.25f);
            burst(r.pos, 14, 16, C(255, 230, 90), 0.4f, 0.8f, false, true);
        }
        r.walk += len(vh) * dt * 0.85f;
    }
    updateDriftCharge(r, dt, vf, g);
    r.wheelRot += vf * dt / 0.42f;
    r.accelL += ((vf - oldVf) / std::max(dt, 0.001f) - r.accelL) * clampf(6 * dt, 0, 1);
    // body pitch follows the slope (or the flight path when airborne)
    float slopeP;
    if (g) {
        V3 a = add(r.pos, mul(f, 2.0f)), b = sub(r.pos, mul(f, 2.0f));
        slopeP = atanf((roadHeight(a, r.lastIdx) + rampHeight(a) - roadHeight(b, r.lastIdx) - rampHeight(b)) / 4.0f);
    } else slopeP = atan2f(r.vel.y, std::max(8.0f, fabsf(vf))) * 0.6f;
    float tp = slopeP + clampf(-r.accelL * 0.004f, -0.07f, 0.07f);
    float tr = r.base == B_ROBOT ? r.steerV * clampf(len(vec(r.vel.x, 0, r.vel.z)) / st.vmax, 0, 1) * 0.12f
                                 : -r.steerV * clampf(fabsf(vf) / st.vmax, 0, 1) * (in.special && wheeled ? 0.12f : 0.07f);
    r.pitch += (tp - r.pitch) * clampf((g ? 10.0f : 3.0f) * dt, 0, 1);
    r.roll += (tr - r.roll) * clampf(6 * dt, 0, 1);
    if (g && wheeled && r.slip > 6.0f && GetRandomValue(0, 100) < 70) {
        V3 back = add(r.pos, add(mul(f, -1.7f), vec(0, 0.3f, 0)));
        Color sc = theme[r.lastIdx] == TH_BEACH ? C(220, 195, 150, 140) : C(200, 200, 205, 110);
        emit(add(back, mul(rt, 0.9f)), vec(frand(-1, 1), 1.2f, frand(-1, 1)), 1.2f, 2.0f, sc);
        emit(sub(back, mul(rt, 0.9f)), vec(frand(-1, 1), 1.2f, frand(-1, 1)), 1.2f, 2.0f, sc);
    }
    if (g && !onRoad && surf < 0.8f && len(r.vel) > 8 && GetRandomValue(0, 100) < 50)
        emit(add(r.pos, mul(f, -2.0f)), vec(frand(-2, 2), 2.0f, frand(-2, 2)), 0.9f, 1.8f, snowK > 0.5f ? C(240, 240, 250, 180) : C(130, 105, 70, 170));
    if (g && wetK > 0.3f && fabsf(vf) > 20 && GetRandomValue(0, 100) < 40)
        emit(add(r.pos, mul(f, -2.2f)), vec(frand(-1, 1), 1.5f, frand(-1, 1)), 0.5f, 1.6f, C(200, 210, 225, 70));
    if (!r.grounded) r.vel.y -= 32.0f * gravK() * dt;
    float frac = clampf(fabsf(vf) / (st.vmax * 1.05f), 0, 1.2f);
    if (wheeled) {
        static const float GB[7] = {0, 0.16f, 0.31f, 0.47f, 0.63f, 0.80f, 1.25f};
        int gr = 1; while (gr < 6 && frac > GB[gr]) gr++;
        if (gr != r.gear) { r.gear = gr; r.gearShiftT = 0.12f; }
        float lo = GB[gr - 1], hi = GB[gr];
        r.rpm = clampf(0.28f + 0.72f * (frac - lo) / (hi - lo), 0.2f, 1.0f);
        if (vf < -0.5f) { r.gear = 0; r.rpm = 0.3f + frac * 2; }
    } else { r.gear = 1; r.rpm = 0.2f + 0.8f * frac; }
    r.gearShiftT = std::max(0.0f, r.gearShiftT - dt);
}

static void physWater(Racer& r, float dt, float gripW) {
    const Stats& st = ST[r.base][M_WATER];
    const Input& in = r.in;
    float bk = boostK(r);
    r.steerV += (in.steer - r.steerV) * clampf(6 * dt, 0, 1);
    V3 f = fwdV(r.yaw);
    float vf = dot(r.vel, f);
    float sp = clampf(fabsf(vf) / 5.0f, r.base == B_TANK ? 0.35f : 0.2f, 1.0f) / (1.0f + 0.8f * powf(fabsf(vf) / 50.0f, 2));
    r.yaw -= r.steerV * st.turn * sp * sgn(vf) * (in.special && canDrift(r) ? 1.4f : 1.0f) * dt;
    f = fwdV(r.yaw); V3 rt = rgtV(r.yaw);
    vf = dot(r.vel, f); float vr = dot(r.vel, rt);
    float oldVf = vf;
    float vmax = st.vmax * vmaxMul(r) * (1.0f + 0.2f * bk);
    float acc = st.acc * r.rubber * r.upA * (1.0f + 0.6f * bk) * (0.6f + 0.4f * gripW);
    if (r.boostT > 0 && vf < vmax) vf += 6.0f * dt;
    if (in.throttle > 0) vf += acc * in.throttle * (1.0f - powf(clampf(vf / vmax, 0, 1), 2)) * dt;
    else if (in.throttle < 0) { if (vf > 0.5f) vf += in.throttle * 30.0f * dt; else vf = std::max(vf + in.throttle * acc * 0.4f * dt, -vmax * 0.25f); }
    vf -= vf * 0.07f * dt + vf * fabsf(vf) * 0.0004f * dt;
    if (vf > vmax) vf -= (vf - vmax) * 1.0f * dt;
    if (r.base == B_ROBOT) vr += in.strafe * 16.0f * dt;
    vr *= expf(-st.lat * (0.8f + 0.2f * gripW) * dt);
    r.slip = fabsf(vr);
    r.vel.x = f.x * vf + rt.x * vr; r.vel.z = f.z * vf + rt.z * vr;
    if (r.base == B_TANK && in.special && r.specialCd <= 0) { r.specialCd = 2.5f; fireShell(r); }
    if (r.base == B_ROBOT && in.special2 && r.special2Cd <= 0) {
        r.vel = add(r.vel, mul(f, 20.0f)); r.special2Cd = 2.0f; r.invuln = std::max(r.invuln, 0.25f);
        burst(r.pos, 12, 14, C(210, 235, 255), 0.4f, 1.0f, false);
    }
    updateDriftCharge(r, dt, vf, true);
    float wy = waveH(r.pos.x, r.pos.z, gTime) - 0.1f;
    r.pos.y += (wy - r.pos.y) * clampf(9 * dt, 0, 1); r.vel.y = 0; r.grounded = true;
    r.accelL += ((vf - oldVf) / std::max(dt, 0.001f) - r.accelL) * clampf(5 * dt, 0, 1);
    float tp = clampf(r.accelL * 0.01f + vf / st.vmax * 0.06f, -0.06f, 0.2f) + 0.05f * sinf(gTime * 1.8f + r.id);
    float tr = r.steerV * clampf(fabsf(vf) / st.vmax, 0, 1) * 0.35f + 0.04f * sinf(gTime * 1.4f + r.id * 2);
    r.pitch += (tp - r.pitch) * clampf(5 * dt, 0, 1); r.roll += (tr - r.roll) * clampf(5 * dt, 0, 1);
    r.walk += dt * 3.0f;
    if (fabsf(vf) > 6.0f && GetRandomValue(0, 100) < 85) {
        V3 back = add(r.pos, mul(f, -radiusOf(r)));
        Color wc = curTrack == 2 ? C(230, 250, 255, 210) : C(230, 245, 255, 210);
        emit(back, vec(frand(-3, 3) - f.x * 4, frand(3, 7), frand(-3, 3) - f.z * 4), 0.7f, 1.3f, wc, true);
        V3 side = add(r.pos, add(mul(f, radiusOf(r) * 0.6f), mul(rt, frand(-1.5f, 1.5f))));
        emit(side, add(mul(rt, frand(-5, 5)), vec(0, frand(2, 4), 0)), 0.5f, 1.0f, C(230, 245, 255, 160), true);
    }
    r.rpm = clampf(0.2f + 0.8f * fabsf(vf) / st.vmax, 0, 1); r.gear = 1;
}

static void physAir(Racer& r, float dt) {
    const Stats& st = ST[r.base][M_AIR];
    const Input& in = r.in;
    r.steerV += (in.steer - r.steerV) * clampf(5 * dt, 0, 1);
    float target = st.vmax * 0.70f * r.rubber * r.upV;
    if (boostK(r) > 0) target = st.vmax * 1.12f * r.upV;
    if (in.special) target = st.vmax * 0.42f;
    if (r.shrinkT > 0) target *= 0.75f;
    r.airSpeed += (target - r.airSpeed) * clampf(1.3f * dt, 0, 1);
    float tb = turbulence();
    r.pitch += in.throttle * 1.15f * dt;
    if (fabsf(in.throttle) < 0.01f) r.pitch *= expf(-0.8f * dt);
    float stall = st.vmax * 0.38f;
    if (r.airSpeed < stall) r.pitch -= (stall - r.airSpeed) / stall * 0.9f * dt;
    if (r.pos.y > 220) r.pitch -= 0.8f * dt;
    r.pitch = clampf(r.pitch, -1.0f, 0.9f);
    r.yaw -= (r.steerV * st.turn * (in.special ? 1.5f : 1.0f) * (0.75f + 0.25f * (1.0f - r.airSpeed / (st.vmax * 1.2f)))
              + sinf(gTime * 7.0f + r.id * 3.0f) * 0.25f * tb) * dt;
    float tr = r.steerV * 0.95f + sinf(gTime * 5.0f + r.id) * 0.18f * tb;
    r.roll += (tr - r.roll) * clampf(5 * dt, 0, 1);
    V3 want = mul(fwd3(r), r.airSpeed);
    want.y += sinf(gTime * 1.3f + r.id) * 6.0f * tb;
    want.x += sinf(gTime * 0.9f + r.id * 2) * 7.0f * tb;
    r.vel = add(r.vel, mul(sub(want, r.vel), clampf(4.5f * dt, 0, 1)));
    r.grounded = false;
    float floorY = (theme[r.lastIdx] == TH_BAY) ? 0.8f : 1.5f;
    if (r.pos.y < floorY) {
        if (r.vel.y < -9.0f) { damage(r, (-r.vel.y - 9.0f) * 1.2f); burst(r.pos, 14, 14, theme[r.lastIdx] == TH_BAY ? C(230, 245, 255) : C(150, 130, 100), 0.7f, 1.3f); }
        r.pos.y = floorY; if (r.vel.y < 0) r.vel.y = 0; if (r.pitch < 0) r.pitch = 0.05f;
        r.airSpeed *= (1.0f - 0.6f * dt);
    }
    if (boostK(r) > 0) {
        V3 f = fwd3(r);
        emit(sub(r.pos, mul(f, 4.0f)), add(mul(f, -r.airSpeed * 0.4f), vec(frand(-1, 1), frand(-1, 1), frand(-1, 1))), 0.4f, 1.6f, C(255, 160, 60), false, true);
    }
    if (GetRandomValue(0, 100) < 40) {
        V3 f = fwd3(r), l = vec(cosf(r.yaw), 0, -sinf(r.yaw));
        float span = r.base == B_TANK ? 7.0f : 5.0f;
        emit(add(sub(r.pos, mul(f, 1.0f)), mul(l, span)), vec(0, 0, 0), 0.8f, 0.5f, C(240, 245, 255, 120));
        emit(sub(sub(r.pos, mul(f, 1.0f)), mul(l, span)), vec(0, 0, 0), 0.8f, 0.5f, C(240, 245, 255, 120));
    }
    r.rpm = clampf(0.3f + 0.7f * r.airSpeed / (st.vmax * 1.2f), 0, 1); r.gear = 1;
}

// inside an open shortcut the road walls are replaced by the path's own side walls
static bool shortcutWalls(Racer& r) {
    if (r.mode != M_LAND) { r.scIdx = -1; return false; }
    int k = shortcutAt(r.pos, 1.5f);
    if (k < 0) { r.scIdx = -1; return false; }
    Shortcut& s = shortcuts[k];
    float rad = radiusOf(r) * 0.8f;
    if (fabsf(r.lateral) <= lim[r.lastIdx] - rad && r.scIdx != k) return false;   // still on the road
    if (!s.open && r.scIdx != k && r.base != B_TANK) return false;                   // closed: normal walls (tanks may go and smash it)
    r.scIdx = k;
    V3 d = sub(r.pos, s.pa); d.y = 0;
    float v = dot(d, s.nrm), lw = s.w / 2 - rad;
    if (fabsf(v) > lw && fabsf(r.lateral) > lim[r.lastIdx] - rad) {
        V3 n = mul(s.nrm, sgn(v));
        float pen = fabsf(v) - lw;
        r.pos = sub(r.pos, mul(n, pen));
        float vn = dot(r.vel, n);
        if (vn > 0) { r.vel = sub(r.vel, mul(n, vn * 1.3f)); if (vn > 12) damage(r, (vn - 12) * 0.6f); }
    }
    return true;
}
static void breakShortcut(int k, int by) {
    Shortcut& s = shortcuts[k];
    if (s.open) return;
    s.open = true; s.openT = 30.0f;
    for (int c = 0; c < 5; c++) burst(add(s.wallP, add(mul(s.nrm, (c - 2) * 2.0f), vec(0, 1.2f, 0))), 12, 16, C(170, 120, 70), 1.2f, 0.9f, true);
    if (dist2D(s.wallP, camPos) < 250) play(sCrash, 1.0f, 0.7f);
    if (by >= 0 && by < (int)racers.size() && racers[by].isPlayer) {
        notifyP(racers[by], T("\320\241\320\240\320\225\320\227\320\232\320\220 \320\236\320\242\320\232\320\240\320\253\320\242\320\220!", "SHORTCUT OPEN!"), C(255, 200, 60), 2.2f);
        shakeR(racers[by], 0.6f); P.walls++;
    }
}
static void updateShortcuts(float dt) {
    for (size_t k = 0; k < shortcuts.size(); k++) {
        Shortcut& s = shortcuts[k];
        if (s.open) {
            s.openT -= dt;
            if (s.openT <= 0) {
                bool busy = false;
                for (const Racer& r : racers) if (!r.dead && shortcutAt(r.pos, 6) == (int)k) busy = true;
                if (!busy) s.open = false; else s.openT = 1.0f;
            }
            continue;
        }
        // the crate barricade along the road edge: tanks smash through, everything else bounces off
        for (Racer& r : racers) {
            if (r.dead || r.out || r.mode != M_LAND) continue;
            V3 d = sub(r.pos, s.wallP); d.y = 0;
            float u = dot(d, s.wdir), v = dot(d, s.wn);          // v < 0: road side
            if (fabsf(u) > s.whl || fabsf(v) > radiusOf(r) + 3.0f) continue;
            float vout = dot(r.vel, s.wn) * -sgn(v);            // speed towards the crates
            if (r.base == B_TANK) {
                if (fabsf(v) < radiusOf(r) + 1.0f) {
                    if (vout > 5.0f) { breakShortcut((int)k, r.id); r.vel = mul(r.vel, 0.85f); }
                    else { r.pos = add(r.pos, mul(s.wn, sgn(v) * (radiusOf(r) + 1.0f - fabsf(v)))); if (vout > 0) r.vel = add(r.vel, mul(s.wn, sgn(v) * vout * 1.2f)); }
                }
            } else if (r.isPlayer && r.scrapeT <= 0 && fabsf(v) < radiusOf(r) + 1.5f && vout > 2) {
                notifyP(r, T("\320\242\320\236\320\233\320\254\320\232\320\236 \320\242\320\220\320\235\320\232 \320\237\320\240\320\236\320\233\320\236\320\234\320\230\320\242 \320\257\320\251\320\230\320\232\320\230!", "ONLY A TANK CAN SMASH THESE!"), C(255, 200, 60), 1.6f); r.scrapeT = 1.0f;
            }
        }
    }
}
static void wallCollide(Racer& r) {
    if (shortcutWalls(r)) return;
    int i = r.lastIdx;
    float L = lim[i] - (r.mode == M_AIR ? 0.0f : radiusOf(r) * 0.8f);
    if (fabsf(r.lateral) <= L) return;
    if (r.mode == M_AIR && r.pos.y > 30.0f && theme[i] != TH_TUNNEL) L = lim[i] * 1.6f;
    if (fabsf(r.lateral) <= L) return;
    V3 n = mul(norv[i], sgn(r.lateral));
    float pen = fabsf(r.lateral) - L;
    r.pos = sub(r.pos, mul(n, pen)); r.lateral -= sgn(r.lateral) * pen;
    float vn = dot(r.vel, n);
    if (vn > 0) {
        r.vel = sub(r.vel, mul(n, vn * 1.35f));
        r.vel.x *= 0.985f; r.vel.z *= 0.985f;
        if (r.mode == M_AIR) { r.yaw += wrapA(atan2f(tanv[i].x, tanv[i].z) - r.yaw) * 0.25f; r.airSpeed *= 0.97f; }
        if (vn > 7.0f) {
            if (vn > 12.0f) damage(r, (vn - 12.0f) * 0.8f);
            shakeR(r, clampf(vn / 40.0f, 0.1f, 0.5f));
            if (r.isPlayer || dist2D(r.pos, camPos) < 60) play(sCrash, clampf(vn / 25.0f, 0.2f, 0.9f), frand(0.9f, 1.2f));
        }
        V3 sp = add(r.pos, add(mul(n, radiusOf(r)), vec(0, 0.7f, 0)));
        int cnt = (int)clampf(vn, 3, 18);
        for (int k = 0; k < cnt; k++) emit(sp, add(mul(fwdV(r.yaw), frand(-4, 4) + len(r.vel) * 0.3f), vec(frand(-3, 3), frand(1, 5), frand(-3, 3))), 0.4f, 0.35f, C(255, 200, 80), true, true);
        if (r.isPlayer && r.scrapeT <= 0) { play(sScrape, 0.5f, frand(0.9f, 1.2f)); r.scrapeT = 0.3f; }
    }
}
static void obstacleCollide(Racer& r) {
    if (r.mode == M_AIR || r.pos.y > 10.0f + th[r.lastIdx]) return;
    float rad = radiusOf(r);
    for (const Tree& t : trees) {
        if (!t.solid) continue;
        float dx = r.pos.x - t.p.x, dz = r.pos.z - t.p.z;
        if (fabsf(dx) > 10 || fabsf(dz) > 10) continue;
        float d = sqrtf(dx * dx + dz * dz), minD = rad + (t.kind == 3 ? 1.0f : 0.9f * t.s);
        if (d < minD && d > 0.01f) {
            V3 n = vec(dx / d, 0, dz / d);
            r.pos.x = t.p.x + n.x * minD; r.pos.z = t.p.z + n.z * minD;
            float vn = dot(r.vel, n);
            if (vn < 0) {
                r.vel = sub(r.vel, mul(n, vn * 1.4f));
                if (-vn > 7.0f) { damage(r, (-vn - 6.0f) * 0.9f); burst(r.pos, 10, 10, t.kind == 3 ? C(200, 180, 100) : C(90, 140, 60), 0.6f, 0.8f); if (r.isPlayer) play(sCrash, 0.6f, 1.2f); }
            }
            r.vel = mul(r.vel, 0.98f);
        }
    }
    if (r.mode == M_LAND && r.grounded) {
        for (const Pad& p : pads) {
            float dx = r.pos.x - p.p.x, dz = r.pos.z - p.p.z;
            if (dx * dx + dz * dz < 40.0f && r.padCd <= 0) {
                r.boostT = std::max(r.boostT, 0.6f); r.padCd = 0.8f; r.energy = std::min(100.0f, r.energy + 15);
                burst(r.pos, 16, 18, C(80, 200, 255), 0.5f, 0.8f, false, true);
                if (r.isPlayer) { play(sWhoosh, 0.5f, 1.6f); shakeR(r, 0.2f); }
            }
        }
    }
}

// slipstream: tuck in behind another racer to cut the drag and charge a tow boost
static void updateDraft(Racer& r, float dt) {
    bool tow = false;
    if (r.mode != M_AIR && !r.isCop && speedOf(r) > 25.0f) {
        V3 f = fwdV(r.yaw);
        for (const Racer& o : racers) {
            if (o.id == r.id || o.dead || o.out || o.mode == M_AIR) continue;
            V3 d = sub(o.pos, r.pos); d.y = 0;
            float along = dot(d, f), side = fabsf(dot(d, rgtV(r.yaw)));
            if (along > 4.0f && along < 30.0f && side < 2.6f && dot(fwdV(o.yaw), f) > 0.9f) { tow = true; break; }
        }
    }
    if (tow) {
        r.draftT += dt;
        if (r.draftT > 0.7f && r.draftK < 0.5f && r.isPlayer) { notifyP(r, T("\320\241\320\233\320\230\320\237\320\241\320\242\320\240\320\230\320\234!", "SLIPSTREAM!"), C(160, 220, 255), 1.2f); r.drafts++; }
        if (r.draftT > 0.7f) r.draftK = std::min(1.0f, r.draftK + dt * 2.0f);
        if (r.draftK > 0.5f && GetRandomValue(0, 100) < 50) {
            V3 f = fwdV(r.yaw), rt = rgtV(r.yaw);
            emit(add(r.pos, add(mul(rt, frand(-2.5f, 2.5f)), vec(0, frand(0.5f, 2.5f), 0))), mul(f, -25.0f), 0.25f, 0.35f, C(200, 230, 255), false, true);
        }
    } else {
        // leaving the tow pocket with a full charge gives a short slingshot
        if (r.draftK > 0.9f) r.boostT = std::max(r.boostT, 0.4f);
        r.draftT = 0; r.draftK = std::max(0.0f, r.draftK - dt * 1.5f);
    }
}
static void updateRacer(Racer& r, float dt) {
    if (r.out) return;
    r.specialCd = std::max(0.0f, r.specialCd - dt); r.special2Cd = std::max(0.0f, r.special2Cd - dt);
    r.switchCd = std::max(0.0f, r.switchCd - dt); r.hitCd = std::max(0.0f, r.hitCd - dt);
    r.invuln = std::max(0.0f, r.invuln - dt); r.padCd = std::max(0.0f, r.padCd - dt);
    r.scrapeT = std::max(0.0f, r.scrapeT - dt); r.hurtT = std::max(0.0f, r.hurtT - dt);
    r.shieldT = std::max(0.0f, r.shieldT - dt); r.shrinkT = std::max(0.0f, r.shrinkT - dt); r.boostT = std::max(0.0f, r.boostT - dt);
    r.squashT = std::max(0.0f, r.squashT - dt); r.trickT = std::max(0.0f, r.trickT - dt);
    if (r.dead) { r.deadTimer -= dt; if (r.deadTimer <= 0) respawn(r); return; }
    r.regenT -= dt; if (r.regenT <= 0 && r.hp < 100) r.hp = std::min(100.0f, r.hp + 4.0f * dt);   // slow self-repair
    if (r.parked) { r.vel = vec(0, 0, 0); updateProgress(r); return; }
    r.popCd = std::max(0.0f, r.popCd - dt); r.startBoostT = std::max(0.0f, r.startBoostT - dt);
    if (r.startBoostT > 0 && GetRandomValue(0, 100) < 40) emit(add(r.pos, add(mul(fwdV(r.yaw), -radiusOf(r)), vec(0, 0.6f, 0))), vec(frand(-1, 1), 2, frand(-1, 1)), 0.8f, 1.6f, C(60, 60, 60, 160));
    updateDraft(r, dt);
    if (r.spinT > 0) {
        r.spinT -= dt; r.spinAng += dt * 15.0f;
        r.in = Input();
        r.vel.x *= expf(-2.2f * dt); r.vel.z *= expf(-2.2f * dt);
        if (r.spinT <= 0) r.spinAng = 0;
    }
    Mode zm = zmode[r.lastIdx];
    if (zm != r.mode) {
        if (r.mode == M_AIR) { if (r.pos.y < 7.0f) setMode(r, zm); else r.pitch = lerpf(r.pitch, -0.25f, clampf(2 * dt, 0, 1)); }
        else setMode(r, zm);
    }
    r.morphAir += ((r.mode == M_AIR ? 1.0f : 0.0f) - r.morphAir) * clampf(3.0f * dt, 0, 1);
    r.morphWater += ((r.mode == M_WATER ? 1.0f : 0.0f) - r.morphWater) * clampf(3.5f * dt, 0, 1);

    bool wantBoost = r.in.boost && !r.boostLock && r.energy > 0;
    r.boosting = wantBoost;
    if (wantBoost) { r.energy -= 30 * r.upN * dt; if (r.energy <= 0) { r.energy = 0; r.boostLock = true; } }
    else { r.energy = std::min(100.0f, r.energy + 7 / r.upN * dt); if (r.boostLock && r.energy > 25) r.boostLock = false; }

    // tricks off jumps (press SPACE while airborne)
    if (r.mode == M_LAND && !r.grounded && r.airTime > 0.12f && r.in.special && !r.prevSpecial && !r.trickDone && r.base != B_TANK) {
        r.trickT = 0.55f; r.trickDone = true;
        if (r.isPlayer) play(sTrick, 0.7f);
    }
    float gripW = weatherGrip(r.base, r.mode) * trackGrip(r.base, theme[r.lastIdx], r.mode);
    if (r.mode == M_LAND) physLand(r, dt, gripW);
    else if (r.mode == M_WATER) physWater(r, dt, gripW);
    else physAir(r, dt);

    r.pos = add(r.pos, mul(r.vel, dt));
    updateProgress(r);
    if (r.mode == M_LAND) {
        float g = groundHeight(r);
        if (r.grounded) {
            float gv = (g - r.lastGround) / std::max(dt, 0.001f);
            if (g < r.lastGround - 0.03f && r.vel.y > gv + 2.0f) { r.grounded = false; r.airTime = 0; r.trickDone = false; }
            else { r.pos.y = g; r.vel.y = clampf(gv, -30, 30); }
        } else {
            r.airTime += dt;
            if (r.pos.y <= g) {
                float impact = -r.vel.y;
                r.pos.y = g; r.vel.y = 0; r.grounded = true;
                if (impact > 22.0f) { damage(r, (impact - 22.0f) * 0.8f); burst(r.pos, 14, 12, C(150, 130, 100), 0.6f, 1.2f); }
                if (impact > 5.0f) { r.squashT = 0.3f; burst(r.pos, 10, 8, theme[r.lastIdx] == TH_BEACH ? C(225, 200, 150, 200) : C(170, 160, 150, 160), 0.6f, 1.6f); shakeR(r, clampf(impact / 30.0f, 0.1f, 0.5f)); }
                if (r.trickDone) {
                    r.boostT = std::max(r.boostT, 0.7f); r.tricks++;
                    if (r.isPlayer) { notifyP(r, T("\320\242\320\240\320\256\320\232! +\320\242\320\243\320\240\320\221\320\236", "TRICK! +BOOST"), C(255, 220, 60), 1.4f); play(sWhoosh, 0.6f, 1.5f); }
                }
                r.trickDone = false; r.airTime = 0;
            }
        }
        r.lastGround = g;
    } else r.lastGround = r.pos.y;
    wallCollide(r);
    obstacleCollide(r);
    if (r.mode == M_AIR) {
        if (r.ringHit.size() != rings.size()) r.ringHit.assign(rings.size(), 0);
        for (size_t k = 0; k < rings.size(); k++) {
            if (r.ringHit[k]) continue;
            int di = std::abs(rings[k].idx - r.lastIdx); di = std::min(di, N - di);
            if (di > 8) continue;
            if (len(sub(rings[k].p, r.pos)) < 12.5f) {
                r.ringHit[k] = 1; r.rings++; r.energy = std::min(100.0f, r.energy + 22);
                r.airSpeed += 6.0f; burst(rings[k].p, 24, 22, C(255, 220, 80), 0.7f, 1.0f, false, true);
                if (r.isPlayer) play(sChime, 0.7f, 1.0f + 0.05f * (r.rings % 5));
            }
        }
    }
    float vt = dot(vec(r.vel.x, 0, r.vel.z), tanv[r.lastIdx]);
    if (vt < -4.0f && !r.isCop) r.wrongT += dt; else r.wrongT = std::max(0.0f, r.wrongT - dt * 2);
    if (boostK(r) > 0 && r.mode != M_AIR) {
        V3 f = fwdV(r.yaw), rt = rgtV(r.yaw);
        V3 b = add(sub(r.pos, mul(f, radiusOf(r) * 0.95f)), vec(0, r.base == B_TANK ? 1.3f : 0.55f, 0));
        Color fc = r.boostT > 0 && !r.boosting ? C(255, 150, 40) : C(90, 170, 255);
        emit(add(b, mul(rt, 0.5f)), add(mul(f, -10.0f), vec(0, 0.5f, 0)), 0.25f, 0.9f, fc, false, true);
        emit(sub(b, mul(rt, 0.5f)), add(mul(f, -10.0f), vec(0, 0.5f, 0)), 0.25f, 0.9f, fc, false, true);
    }
    if (r.hp < 40 && GetRandomValue(0, 100) < 30) emit(add(r.pos, vec(0, 1.5f, 0)), vec(frand(-1, 1), 3, frand(-1, 1)), 1.2f, 2.0f, C(40, 40, 40, 150));
    if (r.hp < 20 && GetRandomValue(0, 100) < 20) emit(add(r.pos, vec(0, 1.2f, 0)), vec(frand(-1, 1), 2, frand(-1, 1)), 0.3f, 0.8f, C(255, 120, 30), false, true);
    r.prevSpecial = r.in.special;
}

static void collideRacers() {
    for (size_t i = 0; i < racers.size(); i++)
        for (size_t j = i + 1; j < racers.size(); j++) {
            Racer &a = racers[i], &b = racers[j];
            if (a.dead || b.dead || a.out || b.out) continue;
            V3 d = sub(b.pos, a.pos); float dist = len(d), minD = radiusOf(a) + radiusOf(b);
            if (dist >= minD || dist < 0.001f) continue;
            V3 n = mul(d, 1.0f / dist);
            float ma = massOf(a), mb = massOf(b), over = minD - dist;
            if (a.parked) { ma = 50; } if (b.parked) { mb = 50; }
            a.pos = sub(a.pos, mul(n, over * mb / (ma + mb)));
            b.pos = add(b.pos, mul(n, over * ma / (ma + mb)));
            float vn = dot(sub(b.vel, a.vel), n);
            if (vn < 0) {
                float j2 = -(1.0f + 0.5f) * vn / (1.0f / ma + 1.0f / mb);
                if (!a.parked) a.vel = sub(a.vel, mul(n, j2 / ma));
                if (!b.parked) b.vel = add(b.vel, mul(n, j2 / mb));
                float impact = -vn;
                if (impact > 4.0f) {
                    // a bigger mass or a shielded ram hurts the other one more
                    float ka = (a.shieldT > 0 ? 0.0f : 1.0f) * (b.shieldT > 0 ? 1.6f : 1.0f), kb = (b.shieldT > 0 ? 0.0f : 1.0f) * (a.shieldT > 0 ? 1.6f : 1.0f);
                    damage(a, std::max(0.0f, impact - 6.0f) * (mb / (ma + mb)) * 0.8f * ka, b.id); damage(b, std::max(0.0f, impact - 6.0f) * (ma / (ma + mb)) * 0.8f * kb, a.id);
                    burst(add(a.pos, add(mul(n, radiusOf(a)), vec(0, 0.8f, 0))), 14, 16, C(255, 210, 90), 0.35f, 0.4f, true, true);
                    if (a.isPlayer || b.isPlayer) { shakeR(a, clampf(impact / 30.0f, 0.15f, 0.8f)); shakeR(b, clampf(impact / 30.0f, 0.15f, 0.8f)); play(sCrash, clampf(impact / 30.0f, 0.3f, 1.0f), frand(0.9f, 1.15f)); }
                    // battle: a hard ram pops a balloon of the slower car
                    if (S.mode == GM_BATTLE && impact > 20.0f) {
                        bool aFaster = speedOf(a) > speedOf(b);
                        if (aFaster && b.shieldT <= 0) popBalloon(b, a.id); else if (!aFaster && a.shieldT <= 0) popBalloon(a, b.id);
                    }
                }
            }
        }
}
static void updateShells(float dt) {
    for (auto& s : shells) {
        s.p = add(s.p, mul(s.v, dt)); s.life -= dt;
        if (s.p.y < 0.2f) { burst(s.p, 14, 14, C(160, 130, 90), 0.5f, 1.2f); s.life = 0; continue; }
        for (auto& r : racers) {
            if (r.dead || r.out || r.id == s.owner) continue;
            if (len(sub(r.pos, s.p)) < radiusOf(r) + 1.8f) {
                if (r.shieldT > 0) { r.shieldT = 0; burst(s.p, 20, 15, C(120, 220, 255), 0.5f, 1.0f, false, true); s.life = 0; break; }
                r.vel = add(r.vel, mul(norm(s.v), 12.0f / massOf(r)));
                damage(r, 12, s.owner); if (r.invuln <= 0) { r.spinT = std::max(r.spinT, 0.4f); r.invuln = 1.0f; }
                popBalloon(r, s.owner);
                burst(s.p, 26, 26, C(255, 150, 40), 0.6f, 1.4f, true, true);
                shakeR(r, 0.6f);
                if (dist2D(s.p, camPos) < 150) play(sBoom, 0.7f, 1.1f);
                s.life = 0; break;
            }
        }
        for (size_t k = 0; k < shortcuts.size() && s.life > 0; k++)
            if (!shortcuts[k].open && fabsf(dot(sub(s.p, shortcuts[k].wallP), shortcuts[k].wdir)) < shortcuts[k].whl && fabsf(dot(sub(s.p, shortcuts[k].wallP), shortcuts[k].wn)) < 2.0f && s.p.y < 4.0f) { breakShortcut((int)k, s.owner); s.life = 0; }
        if (s.life > 0) emit(s.p, vec(0, 0, 0), 0.25f, 0.8f, C(255, 200, 100), false, true);
    }
    shells.erase(std::remove_if(shells.begin(), shells.end(), [](const Shell& s) { return s.life <= 0; }), shells.end());
}

// =====================================================================
//          KART-RACING LAYER: item boxes, items, coins, props
// =====================================================================
struct Proj { V3 p, v; int owner, target, idx; float life; };
struct Hazard { int type; V3 p; float life, arm; int owner; };    // 0 oil, 1 mine, 2 ice patch (boss), 3 fire (boss)
struct LavaBomb { V3 p; float t; };                                 // volcano: falling lava rock with a warning ring
static std::vector<Proj> rocketsP;
static std::vector<Hazard> hazards;
static std::vector<LavaBomb> lavaBombs;
static float lavaBombT = 3.0f;
static bool itemsOn() { return (S.items || S.mode == GM_BATTLE) && S.mode != GM_TIMETRIAL; }
static Color itemColor(int it) {
    switch (it) {
        case IT_TURBO: return C(255, 140, 30);
        case IT_ROCKET: return C(255, 60, 60);
        case IT_OIL: return C(60, 60, 70);
        case IT_SHIELD: return C(80, 200, 255);
        case IT_BOMB: return C(40, 40, 40);
        case IT_LIGHTNING: return C(255, 230, 60);
        default: return WHITE;
    }
}
static int rollItem(const Racer& r) {
    int place = placeOf(r.id);
    float f = numRacers > 1 ? (place - 1) / (float)(numRacers - 1) : 0.5f;   // 0 = leader, 1 = last
    if (S.mode == GM_PURSUIT) f = 0.5f;
    float w[IT_COUNT] = {0, 2.5f + 3.0f * f, 0.6f + 2.0f * f, 2.5f - 1.5f * f, 2.5f - 1.0f * f, 2.0f - 1.4f * f, f > 0.7f ? 1.2f * f : 0.0f};
    if (S.mode == GM_BATTLE) { float wb[IT_COUNT] = {0, 1.6f, 3.2f, 2.4f, 2.2f, 2.6f, 0.35f}; for (int k = 0; k < IT_COUNT; k++) w[k] = wb[k]; }
    float tot = 0; for (int k = 1; k < IT_COUNT; k++) tot += w[k];
    float x = frand(0, tot);
    for (int k = 1; k < IT_COUNT; k++) { x -= w[k]; if (x <= 0) return k; }
    return IT_TURBO;
}
static void dropCoins(Racer& r, int n) {
    int lost = std::min(n, r.coins);
    r.coins -= lost;
    for (int k = 0; k < lost; k++) emit(add(r.pos, vec(0, 1.5f, 0)), vec(frand(-6, 6), frand(5, 9), frand(-6, 6)), 1.0f, 0.6f, C(255, 210, 40), true, true);
}
// explosion / big hit: spin, pop up, lose coins (shield absorbs)
static void itemHit(Racer& v, int attacker, float dmg, bool pop) {
    if (v.dead || v.out) return;
    if (v.shieldT > 0) {
        v.shieldT = 0; burst(add(v.pos, vec(0, 1, 0)), 24, 14, C(120, 220, 255), 0.5f, 1.0f, false, true);
        notifyP(v, T("\320\251\320\230\320\242 \320\237\320\236\320\223\320\233\320\236\320\242\320\230\320\233 \320\243\320\224\320\220\320\240", "SHIELD BLOCKED IT"), C(120, 220, 255), 1.5f);
        play(sShield, 0.5f, 0.7f);
        return;
    }
    v.spinT = std::max(v.spinT, 0.9f); v.invuln = std::max(v.invuln, 1.5f);
    v.driftCharge = 0; v.boostT = 0;
    if (pop && v.mode == M_LAND) { v.vel.y = 9.0f; v.grounded = false; v.airTime = 0; v.trickDone = false; }
    if (v.mode == M_AIR) { v.airSpeed *= 0.6f; v.pitch -= 0.3f; }
    dropCoins(v, 3);
    damage(v, dmg, attacker);
    popBalloon(v, attacker);
    if (v.isPlayer) { shakeR(v, 0.7f); notifyP(v, T("\320\237\320\236\320\237\320\220\320\224\320\220\320\235\320\230\320\225!", "HIT!"), C(255, 90, 60), 1.4f); }
}
static void explode(V3 p, int owner, float radius) {
    burst(p, 40, 24, C(255, 150, 40), 0.7f, 1.6f, true, true);
    burst(p, 20, 10, C(60, 60, 60, 200), 1.4f, 2.6f, false);
    if (dist2D(p, camPos) < 250) play(sBoom, 0.9f, frand(0.9f, 1.1f));
    for (auto& r : racers) {
        if (r.dead || r.out) continue;
        if (len(sub(r.pos, p)) < radius + radiusOf(r)) itemHit(r, owner, 8, true);
    }
}
static int findRocketTarget(const Racer& r) {
    if (S.mode == GM_PURSUIT && r.isPlayer) {
        int best = -1; float bd = 400;
        for (auto& o : racers) if (o.isCop && !o.dead && !o.out) { float d = len(sub(o.pos, r.pos)); if (d < bd) { bd = d; best = o.id; } }
        return best;
    }
    if (S.mode == GM_BATTLE) {   // nearest rival, preferring the ones in front
        int best = -1; float bd = 1e9f; V3 f = fwdV(r.yaw);
        for (auto& o : racers) {
            if (o.id == r.id || o.dead || o.out || o.isCop) continue;
            V3 d = sub(o.pos, r.pos); float dl = len(d);
            float score = dl * (dot(d, f) > 0 ? 1.0f : 2.5f);
            if (dl < 320 && score < bd) { bd = score; best = o.id; }
        }
        return best;
    }
    int place = placeOf(r.id);
    for (int p = place - 2; p >= 0; p--) {
        const Racer& o = racers[order[p]];
        if (!o.dead && !o.out) return o.id;
    }
    return -1;
}
static void useItem(Racer& r) {
    if (r.item == IT_NONE || r.rouletteT > 0 || r.dead || r.spinT > 0) return;
    V3 f = r.mode == M_AIR ? fwd3(r) : fwdV(r.yaw);
    bool near = r.isPlayer || dist2D(r.pos, camPos) < 120;
    switch (r.item) {
        case IT_TURBO:
            r.boostT = std::max(r.boostT, 1.0f);
            burst(sub(r.pos, mul(f, 2.0f)), 16, 12, C(255, 160, 40), 0.4f, 1.0f, false, true);
            if (near) play(sWhoosh, 0.6f, 1.6f);
            break;
        case IT_ROCKET: {
            Proj p; p.p = add(add(r.pos, mul(f, radiusOf(r) + 2.0f)), vec(0, 1.2f, 0)); p.v = add(mul(f, 90.0f), mul(r.vel, 0.5f));
            p.owner = r.id; p.target = findRocketTarget(r); p.idx = r.lastIdx; p.life = 6.0f;
            rocketsP.push_back(p);
            if (near) play(sRocket, 0.8f);
        } break;
        case IT_OIL: case IT_BOMB: {
            Hazard h; h.type = r.item == IT_OIL ? 0 : 1; h.p = sub(r.pos, mul(fwdV(r.yaw), radiusOf(r) + 2.5f));
            h.p.y = r.mode == M_WATER ? 0.0f : roadHeight(h.p, r.lastIdx);
            if (r.mode == M_AIR) h.p.y = r.pos.y;      // mines dropped from the air float as aerial mines
            h.life = 15; h.arm = 0.6f; h.owner = r.id;
            hazards.push_back(h);
            if (near) play(sClick, 0.8f, 0.6f);
        } break;
        case IT_SHIELD:
            r.shieldT = 8.0f;
            if (near) play(sShield, 0.7f);
            break;
        case IT_LIGHTNING: {
            int place = placeOf(r.id);
            for (auto& o : racers) {
                if (o.id == r.id || o.dead || o.out) continue;
                bool victim = S.mode == GM_PURSUIT ? (r.isPlayer ? o.isCop : o.isPlayer) : S.mode == GM_BATTLE ? !o.isCop : (!o.isCop && placeOf(o.id) < place);
                if (!victim) continue;
                if (o.shieldT > 0) { o.shieldT = 0; continue; }
                o.shrinkT = 3.5f; o.spinT = std::max(o.spinT, 0.5f); o.item = IT_NONE; o.itemCount = 0; o.rouletteT = 0;
                if (o.isPlayer) { notifyP(o, T("\320\222\320\220\320\241 \320\243\320\224\320\220\320\240\320\230\320\233\320\220 \320\234\320\236\320\233\320\235\320\230\320\257!", "STRUCK BY LIGHTNING!"), C(255, 230, 60), 2.0f); rumblePad(o.pid, 0.8f, 0.4f); }
                emit(add(o.pos, vec(0, 8, 0)), vec(0, -30, 0), 0.25f, 3.0f, C(255, 240, 120), false, true);
            }
            flashT = std::max(flashT, 0.5f);
            play(sZap, 0.9f);
            notifyP(r, T("\320\234\320\236\320\233\320\235\320\230\320\257! \320\241\320\236\320\237\320\225\320\240\320\235\320\230\320\232\320\230 \320\243\320\234\320\225\320\235\320\254\320\250\320\225\320\235\320\253", "LIGHTNING! RIVALS SHRUNK"), C(255, 230, 60), 2.0f);
        } break;
        default: break;
    }
    r.itemCount--;
    if (r.itemCount <= 0) { r.item = IT_NONE; r.itemCount = 0; }
    r.itemHoldT = 0;
}
static bool rocketIncoming(const Racer& r) {
    for (const Proj& p : rocketsP) if (p.target == r.id && len(sub(p.p, r.pos)) < 120) return true;
    return false;
}
static void aiUseItem(Racer& r, float dt) {
    if (r.item == IT_NONE || r.rouletteT > 0 || r.isPlayer) return;
    r.itemHoldT += dt;
    int place = placeOf(r.id);
    bool use = false;
    if (S.mode == GM_BATTLE) {   // arena: spend items quickly and aggressively
        switch (r.item) {
            case IT_ROCKET: { int t = findRocketTarget(r); use = t >= 0 && len(sub(racers[t].pos, r.pos)) < 160 && r.itemHoldT > 0.8f; } break;
            case IT_OIL: case IT_BOMB:
                for (auto& o : racers) if (o.id != r.id && !o.out && !o.dead) { V3 d = sub(o.pos, r.pos); if (len(d) < 35 && dot(d, fwdV(r.yaw)) < 0) use = true; }
                if (r.itemHoldT > 6) use = true;
                break;
            case IT_SHIELD: use = rocketIncoming(r) || r.itemHoldT > 3; break;
            default: use = r.itemHoldT > 2.0f; break;
        }
        if (use) useItem(r);
        return;
    }
    switch (r.item) {
        case IT_TURBO: { float mc = 0; for (int k = 2; k < 30; k += 4) mc = std::max(mc, curv[(r.lastIdx + k) % N]); use = mc < 0.004f && r.itemHoldT > 1.5f; } break;
        case IT_ROCKET: { int t = findRocketTarget(r); use = t >= 0 && len(sub(racers[t].pos, r.pos)) < 200 && r.itemHoldT > 2.5f; } break;
        case IT_OIL: case IT_BOMB: {
            for (auto& o : racers) if (o.id != r.id && !o.out && !o.dead) {
                V3 d = sub(o.pos, r.pos);
                if (len(d) < 30 && dot(d, fwdV(r.yaw)) < 0 && r.itemHoldT > 1.5f) use = true;
            }
            if (r.itemHoldT > 12) use = true;
        } break;
        case IT_SHIELD: use = rocketIncoming(r) || r.itemHoldT > 6; break;
        case IT_LIGHTNING: use = (place >= numRacers - 1 && r.itemHoldT > 4.0f) || r.itemHoldT > 15; break;
        default: break;
    }
    if (use) useItem(r);
}
static void updateItems(float dt) {
    // item boxes
    for (auto& b : itemBoxes) {
        if (b.t > 0) { b.t -= dt; continue; }
        if (!itemsOn()) continue;
        for (auto& r : racers) {
            if (r.dead || r.out || r.isCop) continue;
            if (len(sub(r.pos, b.p)) < radiusOf(r) + 2.4f) {
                b.t = 3.0f;
                burst(b.p, 18, 10, C(255, 255, 255), 0.5f, 0.8f, false, true);
                if (r.item == IT_NONE && r.rouletteT <= 0) { r.rouletteT = r.isPlayer ? 1.1f : 0.7f; if (r.isPlayer) play(sTick, 0.7f); }
                if (S.mode == GM_BATTLE) b.t = 1.5f;
                break;
            }
        }
    }
    // roulettes and item use
    for (auto& r : racers) {
        if (r.out || r.isCop) continue;
        if (r.rouletteT > 0) {
            float before = r.rouletteT;
            r.rouletteT -= dt;
            if (r.isPlayer && (int)(before * 12) != (int)(r.rouletteT * 12)) play(sTick, 0.4f, 1.0f + frand(0, 0.3f));
            if (r.rouletteT <= 0) {
                r.item = rollItem(r);
                int place = placeOf(r.id);
                r.itemCount = r.item == IT_TURBO ? (place >= numRacers - 1 && numRacers > 2 ? 3 : 1) : 1;
                r.itemHoldT = 0;
                if (r.isPlayer) { play(sGet, 0.7f); hintT = std::max(hintT, 2.5f); }
            }
        }
        if (r.isPlayer) { if (r.in.item && !r.prevItem) useItem(r); r.prevItem = r.in.item; }
        else aiUseItem(r, dt);
    }
    // coins
    for (auto& c : coinsW) {
        if (c.t > 0) { c.t -= dt; continue; }
        for (auto& r : racers) {
            if (r.dead || r.out || r.isCop) continue;
            if (len(sub(r.pos, c.p)) < radiusOf(r) + 1.6f) {
                c.t = 10.0f; r.coins = std::min(r.coins + 1, 10); r.coinsTotal++;
                emit(c.p, vec(0, 4, 0), 0.4f, 1.4f, C(255, 220, 60), false, true);
                if (r.isPlayer) { play(sCoin, 0.5f, 1.0f + 0.03f * r.coins); rumblePad(r.pid, 0.15f, 0.05f); }
                break;
            }
        }
    }
    // rockets
    for (auto& p : rocketsP) {
        p.life -= dt;
        int best = p.idx; float bd = 1e9f;
        for (int k = -6; k <= 10; k++) { int i = (p.idx + k + N) % N; float d = dist2D(track[i], p.p); if (d < bd) { bd = d; best = i; } }
        p.idx = best;
        V3 goal;
        bool hasT = p.target >= 0 && p.target < (int)racers.size() && !racers[p.target].dead && !racers[p.target].out;
        if (hasT && len(sub(racers[p.target].pos, p.p)) < 180) goal = add(racers[p.target].pos, vec(0, 0.8f, 0));
        else { int g = (p.idx + 12) % N; goal = track[g]; goal.y = zmode[g] == M_AIR ? airAlt[g] + 1.5f : th[g] + 1.2f; }
        V3 dir = norm(p.v), want = norm(sub(goal, p.p));
        dir = norm(add(dir, mul(sub(want, dir), clampf(4.5f * dt, 0, 1))));
        float spd = std::min(len(p.v) + 60 * dt, 112.0f);
        p.v = mul(dir, spd);
        p.p = add(p.p, mul(p.v, dt));
        if (zmode[p.idx] != M_AIR) p.p.y = std::max(p.p.y, th[p.idx] + 0.6f);
        emit(sub(p.p, mul(dir, 1.2f)), vec(frand(-1, 1), frand(-1, 1), frand(-1, 1)), 0.5f, 1.2f, C(200, 200, 200, 140));
        emit(p.p, vec(0, 0, 0), 0.15f, 1.0f, C(255, 160, 50), false, true);
        for (auto& r : racers) {
            if (r.id == p.owner || r.dead || r.out) continue;
            if (len(sub(r.pos, p.p)) < radiusOf(r) + 1.6f) { explode(p.p, p.owner, 3.0f); p.life = 0; break; }
        }
        if (p.life <= 0 && p.life > -1) { burst(p.p, 16, 10, C(255, 150, 40), 0.4f, 1.0f, false, true); p.life = -2; }
    }
    rocketsP.erase(std::remove_if(rocketsP.begin(), rocketsP.end(), [](const Proj& p) { return p.life <= 0; }), rocketsP.end());
    // hazards
    for (auto& h : hazards) {
        h.life -= dt; h.arm -= dt;
        if (h.arm > 0) continue;
        for (auto& r : racers) {
            if (r.dead || r.out) continue;
            if (h.type != 1 && r.mode == M_AIR) continue;
            float d = len(sub(r.pos, h.p));
            if (h.type == 2 && d < 4.5f + radiusOf(r) * 0.5f && r.spinT <= 0 && r.invuln <= 0 && r.id != h.owner) {   // ice patch
                if (r.shieldT > 0) { r.shieldT = 0; continue; }
                r.spinT = 0.6f; r.driftCharge = 0; r.invuln = std::max(r.invuln, 1.0f);
                if (r.isPlayer) { play(sSpin, 0.7f, 1.3f); notifyP(r, T("\320\233\320\201\320\224!", "ICE!"), C(170, 230, 255), 1.2f); }
            }
            if (h.type == 3 && d < 3.2f + radiusOf(r) * 0.5f && r.id != h.owner) {                              // fire trail
                damage(r, 4, h.owner); r.vel.x *= 0.97f; r.vel.z *= 0.97f;
                if (GetRandomValue(0, 100) < 40) emit(add(r.pos, vec(0, 1, 0)), vec(frand(-1, 1), 4, frand(-1, 1)), 0.4f, 1.0f, C(255, 120, 30), false, true);
            }
            if (h.type == 0 && d < 3.8f + radiusOf(r) * 0.5f && r.spinT <= 0 && r.invuln <= 0) {
                if (r.shieldT > 0) { r.shieldT = 0; continue; }
                r.spinT = 0.85f; r.driftCharge = 0; r.invuln = std::max(r.invuln, 1.2f);
                if (r.isPlayer) { play(sSpin, 0.8f); notifyP(r, T("\320\234\320\220\320\241\320\233\320\236!", "OIL SLICK!"), C(200, 200, 210), 1.2f); rumblePad(r.pid, 0.5f, 0.3f); }
                r.lastHitBy = h.owner; r.lastHitT = gTime;
                popBalloon(r, h.owner);
            }
            if (h.type == 1 && d < 2.8f + radiusOf(r)) { explode(h.p, h.owner, 5.0f); h.life = 0; break; }
        }
    }
    hazards.erase(std::remove_if(hazards.begin(), hazards.end(), [](const Hazard& h) { return h.life <= 0; }), hazards.end());
    // volcano: lava bombs fall ahead of racers on the volcanic slopes
    if (curTrack == 4 && !racers.empty()) {
        lavaBombT -= dt;
        if (lavaBombT <= 0) {
            lavaBombT = frand(0.9f, 2.0f);
            const Racer& who = racers[GetRandomValue(0, (int)racers.size() - 1)];
            int idx = (who.lastIdx + GetRandomValue(20, 60)) % N;
            if (theme[idx] == TH_VOLCANO && !who.dead) {
                LavaBomb b; b.p = add(track[idx], mul(norv[idx], frand(-hw[idx] + 2, hw[idx] - 2))); b.p.y = th[idx]; b.t = 2.2f;
                lavaBombs.push_back(b);
            }
        }
    }
    for (auto& b : lavaBombs) {
        b.t -= dt;
        if (b.t > 0) {
            V3 rock = add(b.p, vec(b.t * 18.0f, b.t * 70.0f, b.t * 10.0f));
            if (GetRandomValue(0, 100) < 70) emit(rock, vec(frand(-2, 2), 6, frand(-2, 2)), 0.6f, 2.2f, C(255, 120, 30), false, true);
            continue;
        }
        burst(b.p, 40, 22, C(255, 140, 30), 0.8f, 1.8f, true, true);
        burst(b.p, 18, 8, C(50, 40, 40, 200), 1.6f, 3.0f, false);
        if (dist2D(b.p, camPos) < 260) play(sBoom, 0.8f, frand(0.7f, 0.9f));
        for (auto& r : racers) {
            if (r.dead || r.out || r.mode != M_LAND) continue;
            if (len(sub(r.pos, b.p)) < 5.5f + radiusOf(r)) itemHit(r, -1, 6, true);
        }
    }
    lavaBombs.erase(std::remove_if(lavaBombs.begin(), lavaBombs.end(), [](const LavaBomb& b) { return b.t <= 0; }), lavaBombs.end());
    // destructible cones / barrels / crates
    for (auto& d : dyns) {
        if (d.hit) {
            d.t += dt; d.v.y -= 25 * dt; d.p = add(d.p, mul(d.v, dt)); d.rot += d.spin * dt;
            float gy = d.home.y;
            if (d.p.y < gy) { d.p.y = gy; d.v.y *= -0.35f; d.v.x *= 0.6f; d.v.z *= 0.6f; d.spin *= 0.6f; }
            if (d.t > 7.0f && dist2D(d.home, camPos) > 80) { d.hit = false; d.p = d.home; d.v = vec(0, 0, 0); d.rot = 0; d.t = 0; }
            continue;
        }
        if (dist2D(d.p, camPos) > 400) continue;
        for (auto& r : racers) {
            if (r.dead || r.out || r.mode == M_AIR) continue;
            if (dist2D(r.pos, d.p) < radiusOf(r) + 0.7f && fabsf(r.pos.y - d.p.y) < 3) {
                d.hit = true; d.t = 0;
                d.v = add(mul(r.vel, 0.9f), vec(frand(-3, 3), 6 + speedOf(r) * 0.12f, frand(-3, 3)));
                d.spin = frand(-12, 12);
                r.vel.x *= 0.985f; r.vel.z *= 0.985f;
                burst(d.p, 6, 6, d.c, 0.4f, 0.4f, true);
                if (r.isPlayer) play(sScrape, 0.35f, 0.6f);
                break;
            }
        }
    }
}

// =====================================================================
//                 TRAFFIC, POLICE AND RACER AI
// =====================================================================
struct Traffic { V3 pos, vel; float yaw, speed, lane, spinV, len, wid, hgt, mass, t; int idx, dir, type; Color c; bool wreck, dead; };
static std::vector<Traffic> traffic;
static bool trafficTheme(Theme t) { return t == TH_CITY || t == TH_HIGHWAY || t == TH_COUNTRY || t == TH_TUNNEL || t == TH_BEACH || t == TH_SNOW || t == TH_DESERT; }
static bool trafficOn() { return S.traffic && !indoorTrack() && !spaceTrack() && S.mode != GM_TIMETRIAL; }
static float humanDist(V3 p) {     // distance to the nearest human player
    float d = 1e9f;
    for (const Racer& r : racers) if (r.isPlayer) d = std::min(d, dist2D(r.pos, p));
    return d;
}

static void spawnTraffic(const Racer& pl) {
    if (!trafficOn()) return;
    int target = S.mode == GM_PURSUIT ? 11 : 7;
    int alive = 0; for (auto& t : traffic) if (!t.dead) alive++;
    for (int tries = 0; tries < 3 && alive < target; tries++) {
        int off = GetRandomValue(55, 210);
        int idx = (pl.lastIdx + off) % N;
        if (zmode[idx] != M_LAND || !trafficTheme(theme[idx]) || hw[idx] < 13.5f) continue;
        Traffic t; t.dir = GetRandomValue(0, 100) < 55 ? 1 : -1;
        float lanes[2] = {3.5f, 10.5f};
        t.lane = lanes[GetRandomValue(0, 1)] * (t.dir > 0 ? -1.0f : 1.0f);
        V3 p = add(track[idx], mul(norv[idx], t.lane)); p.y = th[idx];
        bool ok = humanDist(p) > 120;
        for (auto& o : traffic) if (!o.dead && dist2D(o.pos, p) < 30) ok = false;
        if (!ok) continue;
        float r = frand(0, 1);
        t.type = r < 0.55f ? 0 : r < 0.75f ? 1 : r < 0.9f ? 2 : 3;
        if (t.type == 3 && theme[idx] != TH_CITY) t.type = 1;
        const float L[4] = {4.4f, 5.0f, 8.5f, 11.0f}, W[4] = {1.9f, 2.1f, 2.5f, 2.6f}, H[4] = {1.4f, 2.2f, 3.3f, 3.2f}, M[4] = {1.5f, 2.2f, 5.0f, 6.0f};
        t.len = L[t.type]; t.wid = W[t.type]; t.hgt = H[t.type]; t.mass = M[t.type];
        Color pal[8] = {C(200, 200, 205), C(40, 40, 45), C(160, 30, 30), C(30, 70, 140), C(220, 200, 60), C(90, 110, 90), C(240, 240, 240), C(120, 80, 50)};
        t.c = t.type == 3 ? C(240, 190, 30) : pal[GetRandomValue(0, 7)];
        t.speed = t.dir > 0 ? frand(15, 24) : frand(13, 20);
        t.pos = p; t.idx = idx; t.yaw = atan2f(tanv[idx].x * t.dir, tanv[idx].z * t.dir); t.vel = mul(fwdV(t.yaw), t.speed);
        t.spinV = 0; t.t = 0; t.wreck = false; t.dead = false;
        traffic.push_back(t); alive++;
    }
}
static void updateTraffic(float dt) {
    if (racers.empty()) return;
    const Racer& pl = racers[0];
    spawnTraffic(pl);
    if (numHumans > 1 && racers.size() > 1) spawnTraffic(racers[1]);
    for (auto& t : traffic) {
        if (t.dead) continue;
        int best = t.idx; float bd = 1e9f;
        for (int k = -8; k <= 8; k++) { int i = (t.idx + k + N) % N; float d = dist2D(track[i], t.pos); if (d < bd) { bd = d; best = i; } }
        t.idx = best;
        if (t.wreck) {
            t.t += dt;
            t.vel.x *= expf(-1.6f * dt); t.vel.z *= expf(-1.6f * dt);
            t.pos = add(t.pos, mul(vec(t.vel.x, 0, t.vel.z), dt));
            t.yaw += t.spinV * dt; t.spinV *= expf(-2.0f * dt);
            t.pos.y = roadHeight(t.pos, t.idx);
            if (t.t < 2 && GetRandomValue(0, 100) < 30) emit(add(t.pos, vec(0, t.hgt, 0)), vec(frand(-1, 1), 3, frand(-1, 1)), 1.2f, 2.0f, C(50, 50, 50, 140));
            if (t.t > 14 && humanDist(t.pos) > 150) t.dead = true;
        } else {
            int ahead = (t.idx + t.dir * 5 + N) % N;
            if (zmode[ahead] != M_LAND || !trafficTheme(theme[ahead])) { t.speed = std::max(0.0f, t.speed - 10 * dt); if (humanDist(t.pos) > 200) t.dead = true; }
            V3 target = add(track[ahead], mul(norv[ahead], t.lane));
            V3 d = sub(target, t.pos); d.y = 0;
            V3 dir = norm(d);
            // brake behind racers in the same lane
            float want = t.speed;
            for (const Racer& r : racers) {
                if (r.dead || r.out) continue;
                V3 rr = sub(r.pos, t.pos); float along = dot(rr, dir), side = fabsf(dot(rr, vec(dir.z, 0, -dir.x)));
                if (along > 0 && along < 18 && side < 2.5f) want = std::min(want, speedOf(r) * 0.8f);
            }
            float cur = len(vec(t.vel.x, 0, t.vel.z));
            cur += clampf(want - cur, -12 * dt, 4 * dt);
            t.vel = mul(dir, cur);
            t.pos = add(t.pos, mul(t.vel, dt));
            t.pos.y = roadHeight(t.pos, t.idx);
            float ty = atan2f(dir.x, dir.z);
            t.yaw += wrapA(ty - t.yaw) * clampf(4 * dt, 0, 1);
        }
        if (humanDist(t.pos) > 950) t.dead = true;
        // collisions with racers (oriented box vs circle)
        for (auto& r : racers) {
            if (r.dead || r.out || r.mode == M_AIR || fabsf(r.pos.y - t.pos.y) > t.hgt + 1.5f) continue;
            V3 dd = sub(r.pos, t.pos); dd.y = 0;
            if (dd.x * dd.x + dd.z * dd.z > 200) continue;
            V3 f = fwdV(t.yaw), rt = rgtV(t.yaw);
            float lz = dot(dd, f), lx = dot(dd, rt);
            float cx = clampf(lx, -t.wid / 2, t.wid / 2), cz = clampf(lz, -t.len / 2, t.len / 2);
            float ex = lx - cx, ez = lz - cz, dist = sqrtf(ex * ex + ez * ez), rad = radiusOf(r) * 0.85f;
            if (dist >= rad) continue;
            V3 n = dist > 0.01f ? norm(add(mul(rt, ex), mul(f, ez))) : norm(dd);
            r.pos = add(r.pos, mul(n, rad - dist));
            V3 vrel = sub(r.vel, t.vel); vrel.y = 0;
            float vn = dot(vrel, n);
            if (vn >= 0) continue;
            float mr = massOf(r), mt = t.mass;
            float j = -(1.4f) * vn / (1.0f / mr + 1.0f / mt);
            r.vel = add(r.vel, mul(n, j / mr));
            t.vel = sub(t.vel, mul(n, j / mt));
            float impact = -vn;
            damage(r, std::max(0.0f, impact - 6.0f) * (mt / (mr + mt)) * 0.7f);
            if (impact > 9 && !t.wreck) {
                t.wreck = true; t.spinV = frand(-1, 1) * impact * 0.12f; t.t = 0;
                burst(add(t.pos, vec(0, 1, 0)), 20, 14, C(255, 210, 90), 0.4f, 0.5f, true, true);
                if (r.isPlayer && S.mode == GM_PURSUIT) heat = std::min(5.0f, heat + 0.15f);
            } else if (!t.wreck) t.speed *= 0.8f;
            if (r.isPlayer || dist2D(r.pos, camPos) < 80) { play(sCrash, clampf(impact / 30.0f, 0.3f, 1.0f), frand(0.8f, 1.1f)); shakeR(r, clampf(impact / 30.0f, 0.15f, 0.8f)); }
        }
    }
    traffic.erase(std::remove_if(traffic.begin(), traffic.end(), [](const Traffic& t) { return t.dead; }), traffic.end());
}

// ------------------------------ police -------------------------------
static int copsAlive() { int n = 0; for (auto& r : racers) if (r.isCop && !r.out) n++; return n; }
static void spawnCop(bool roadblock, float lateral) {
    const Racer& pl = racers[0];
    int idx = -1;
    for (int tries = 0; tries < 20; tries++) {
        int cand = roadblock ? (pl.lastIdx + GetRandomValue(130, 170)) % N : (pl.lastIdx - GetRandomValue(45, 80) + N) % N;
        if (zmode[cand] == M_LAND && theme[cand] != TH_TUNNEL) { idx = cand; break; }
    }
    if (idx < 0) return;
    Racer c; c.id = (int)racers.size(); c.isCop = true; c.base = B_CAR; c.color = C(20, 22, 28); c.helmet = WHITE; c.name = T("\320\237\320\236\320\233\320\230\320\246\320\230\320\257", "POLICE");
    c.lastIdx = idx; c.pos = add(track[idx], mul(norv[idx], lateral)); c.pos.y = th[idx]; c.lastGround = c.pos.y;
    c.yaw = atan2f(tanv[idx].x, tanv[idx].z) + (roadblock ? PI_F / 2 : 0.0f);
    c.parked = roadblock; c.aggro = 1; c.skill = 1; c.invuln = 1.0f;
    c.vel = roadblock ? vec(0, 0, 0) : mul(tanv[idx], speedOf(pl) * 0.9f);
    c.ringHit.assign(rings.size(), 0);
    racers.push_back(c);
}
static void aiCop(Racer& c, float dt) {
    (void)dt;
    const Racer& pl = racers[0];
    Input in;
    float d = len(sub(pl.pos, c.pos));
    if (c.parked) { if (d < 60) c.parked = false; else { c.in = in; return; } }
    V3 tgt;
    if (d < 120) tgt = add(pl.pos, mul(pl.vel, clampf(d / 60.0f, 0.15f, 0.8f)));
    else { int i = (c.lastIdx + 16) % N; tgt = track[i]; tgt.y = zmode[i] == M_AIR ? airAlt[i] + 1 : th[i]; }
    V3 toT = sub(tgt, c.pos);
    float diff = wrapA(atan2f(toT.x, toT.z) - c.yaw), ad = fabsf(diff);
    in.steer = clampf(-diff * 2.6f, -1, 1);
    c.rubber = 0.97f + 0.02f * heat + (d > 250 ? 0.12f : d > 120 ? 0.05f : 0.0f) + (S.difficulty - 1) * 0.03f;
    if (c.mode == M_AIR) {
        float hd = std::max(25.0f, sqrtf(toT.x * toT.x + toT.z * toT.z));
        in.throttle = clampf((clampf(atan2f(toT.y, hd), -0.6f, 0.6f) - c.pitch) * 3.0f, -1, 1);
        in.boost = d > 120;
    } else {
        in.throttle = ad > 1.3f ? 0.3f : 1.0f;
        if (d > 120) { float mc = 0.0001f; for (int k = 2; k <= 40; k += 4) mc = std::max(mc, curv[(c.lastIdx + k) % N]);
            float vt = clampf(sqrtf(24.0f / mc), 14.0f, 90.0f); if (speedOf(c) > vt + 5) in.throttle = -0.6f; }
        in.boost = (d > 100 && ad < 0.15f) || (d < 40 && ad < 0.3f);
    }
    if (!c.dead && c.mode != M_AIR && speedOf(c) < 2.5f) c.stuckTimer += dt; else c.stuckTimer = 0;
    if (c.stuckTimer > 3.0f) respawn(c);
    c.in = in;
}

// ------------------------------ bosses -------------------------------
static const char* bossName(int b) {
    switch (b) {
        case 1: return T("\320\226\320\225\320\233\320\225\320\227\320\235\320\253\320\231 \320\221\320\220\320\240\320\236\320\235", "IRON BARON");
        case 2: return T("\320\241\320\235\320\225\320\226\320\235\320\220\320\257 \320\232\320\236\320\240\320\236\320\233\320\225\320\222\320\220", "SNOW QUEEN");
        case 3: return T("\320\234\320\220\320\223\320\234\320\220", "MAGMA");
        default: return T("\320\232\320\236\320\234\320\220\320\235\320\224\320\236\320\240 \320\234\320\225\320\223\320\220", "COMMANDER MEGA");
    }
}
static void bossAbility(Racer& r) {
    r.bossT = frand(10, 16);
    if (r.mode != M_LAND || r.dead) { r.bossT = 2; return; }
    bool seen = humanDist(r.pos) < 220;
    V3 f = fwdV(r.yaw);
    switch (r.boss) {
        case 1:   // triple cannon volley
            for (int k = -1; k <= 1; k++) {
                float a = r.yaw + k * 0.07f;
                V3 d = vec(sinf(a), 0.02f, cosf(a));
                Shell s; s.p = add(r.pos, add(mul(d, radiusOf(r) + 3.0f), vec(0, 1.9f, 0))); s.v = add(mul(d, 165.0f), mul(r.vel, 0.4f)); s.life = 1.6f; s.owner = r.id;
                shells.push_back(s);
            }
            burst(add(r.pos, add(mul(f, 4.0f), vec(0, 2, 0))), 30, 24, C(255, 190, 70), 0.4f, 1.2f, false, true);
            if (seen) play(sShot, 0.9f, 0.8f);
            break;
        case 2:   // ice patches behind
            for (int k = 0; k < 3; k++) {
                Hazard h; h.type = 2; h.p = sub(r.pos, mul(f, radiusOf(r) + 4.0f + k * 7.0f)); h.p = add(h.p, mul(rgtV(r.yaw), frand(-4, 4)));
                h.p.y = roadHeight(h.p, r.lastIdx); h.life = 12; h.arm = 0.4f; h.owner = r.id; hazards.push_back(h);
            }
            burst(r.pos, 30, 14, C(200, 240, 255), 0.8f, 1.4f, false, true);
            if (seen) play(sShield, 0.8f, 1.6f);
            break;
        case 3:   // fire trail
            for (int k = 0; k < 6; k++) {
                Hazard h; h.type = 3; h.p = sub(r.pos, mul(f, radiusOf(r) + 2.0f + k * 4.0f)); h.p.y = roadHeight(h.p, r.lastIdx);
                h.life = 6.0f + k * 0.3f; h.arm = 0.2f; h.owner = r.id; hazards.push_back(h);
            }
            if (seen) play(sRocket, 0.8f, 0.6f);
            break;
        default: {   // homing rockets at the racers in front
            int place = placeOf(r.id), fired = 0;
            for (int p = place - 2; p >= 0 && fired < 2; p--) {
                const Racer& o = racers[order[p]];
                if (o.dead || o.out) continue;
                Proj q; q.p = add(add(r.pos, mul(f, radiusOf(r) + 2.0f)), vec(fired ? 1.2f : -1.2f, 1.4f, 0)); q.v = add(mul(f, 90.0f), mul(r.vel, 0.5f));
                q.owner = r.id; q.target = o.id; q.idx = r.lastIdx; q.life = 6.0f; rocketsP.push_back(q); fired++;
            }
            if (!fired) { r.boostT = std::max(r.boostT, 1.2f); }
            if (seen) play(sRocket, 0.9f, 1.2f);
        } break;
    }
    if (seen) for (Racer& h : racers) if (h.isPlayer) notifyP(h, std::string(bossName(r.boss)) + T(": \320\220\320\242\320\220\320\232\320\220!", ": ATTACK!"), C(255, 90, 60), 1.6f);
}

// ------------------------------ racers -------------------------------
static bool aiWantsShortcut(const Racer& r, int k) {
    const Shortcut& s = shortcuts[k];
    float h = hash1(r.id * 131 + k * 17 + r.lap * 7);
    if (r.base == B_TANK) return h < 0.85f;
    return s.open && h < 0.9f;
}
static void aiThink(Racer& r, float dt) {
    if (r.isCop) { aiCop(r, dt); return; }
    Input in;
    const Racer* bestH = &racers[0];
    for (const Racer& o : racers) if (o.isPlayer && raceScore(o) > raceScore(*bestH)) bestH = &o;
    const Racer& pl = *bestH;
    r.aiTimer -= dt; r.aiLaneT -= dt;
    if (!r.isPlayer && !r.boss && r.aiTimer <= 0 && r.switchCd <= 0 && !r.dead && r.mode == M_LAND) {
        Base b = (Base)GetRandomValue(0, B_COUNT - 1);
        if (!baseAllowed(b)) b = GetRandomValue(0, 1) ? B_CAR : B_KART;
        if (b != r.base) switchBase(r, b);
        r.aiTimer = frand(60, 120);
    }
    if (r.boss) { r.bossT -= dt; if (r.bossT <= 0 && raceTime > 6.0f) bossAbility(r); }
    if (r.aiLaneT <= 0) { r.lane = frand(-0.6f, 0.6f) * hw[r.lastIdx]; r.aiLaneT = frand(4, 9); }
    float base = S.difficulty == 0 ? 0.88f : S.difficulty == 1 ? 0.96f : 1.02f;
    if (careerEvent >= 0) base += careerEvent * 0.0022f;
    float gap = (raceScore(r) - raceScore(pl)) * (trackLen / N);
    float rb = clampf(1.0f - gap / 3000.0f * 0.08f, 0.93f, 1.07f);
    if (r.boss) { base = std::max(base, 0.98f) + 0.01f; rb = clampf(1.0f - gap / 2000.0f * 0.1f, 0.95f, 1.08f); }
    r.rubber = r.isPlayer ? 1.0f : base * r.skill * rb;

    int look = r.mode == M_AIR ? 30 : (r.base == B_TANK ? 11 : 15);
    int ti = (r.lastIdx + look) % N;
    float laneW = clampf(r.lane, -(hw[ti] - 3.0f), hw[ti] - 3.0f);
    // dodge pillars, traffic and hazards on the chosen lane
    V3 fw = fwdV(r.yaw);
    bool dodge = false; V3 dodgeP = vec(0, 0, 0); float dodgeAlong = 1e9f;
    for (const Tree& t : trees) {
        if (!t.solid) continue;
        V3 d = sub(t.p, r.pos); float along = dot(d, fw);
        if (along < -1 || along > 45 || len(d) > 50) continue;
        // lateral offset measured at the pillar's own track slice (correct in corners)
        int pi = r.lastIdx; float pb = 1e18f;
        for (int k = 0; k < 14; k++) { int q = (r.lastIdx + k) % N; float dd = dist2D(track[q], t.p); if (dd < pb) { pb = dd; pi = q; } }
        float tl = dot(sub(t.p, track[pi]), norv[pi]), cl = radiusOf(r) + 2.4f;
        float myLat = dot(sub(r.pos, track[pi]), norv[pi]);
        if (fabsf(tl - laneW) < cl || (fabsf(tl - myLat) < cl && along < 30)) {
            float side = (laneW > tl) ? 1.0f : -1.0f;
            if (fabsf(tl + side * (cl + 1.0f)) > hw[pi] - 2.0f) side = -side;   // not enough room on that side
            laneW = tl + side * (cl + 1.0f);
            if (along > 0 && along < dodgeAlong) { dodgeAlong = along; dodge = true; dodgeP = add(add(t.p, mul(norv[pi], side * (cl + 1.5f))), mul(tanv[pi], 3.0f)); }
        }
    }
    for (const Traffic& t : traffic) {
        if (t.dead) continue;
        V3 d = sub(t.pos, r.pos); float along = dot(d, fw);
        if (along < 0 || along > 45 || len(d) > 50) continue;
        float tl = dot(sub(t.pos, track[r.lastIdx]), norv[r.lastIdx]);
        if (fabsf(tl - laneW) < 3.5f) laneW = tl + (laneW > tl ? 5.0f : -5.0f);
    }
    for (const Hazard& h : hazards) {
        if (h.owner == r.id) continue;
        V3 d = sub(h.p, r.pos); float along = dot(d, fw);
        if (along < 0 || along > 40 || len(d) > 45) continue;
        float tl = dot(sub(h.p, track[r.lastIdx]), norv[r.lastIdx]);
        if (fabsf(tl - laneW) < 4.0f) laneW = tl + (laneW > tl ? 5.0f : -5.0f);
    }
    for (const LavaBomb& b : lavaBombs) {
        V3 d = sub(b.p, r.pos); float along = dot(d, fw);
        if (along < -5 || along > 70 || len(d) > 75 || hash1(r.id * 37 + (int)(b.p.x)) > 0.75f) continue;   // most racers see it coming
        float tl = dot(sub(b.p, track[r.lastIdx]), norv[r.lastIdx]);
        if (fabsf(tl - laneW) < 7.5f) laneW = tl + (laneW > tl ? 8.0f : -8.0f);
    }
    laneW = clampf(laneW, -(hw[ti] - 2.5f), hw[ti] - 2.5f);
    V3 tgt = add(track[ti], mul(norv[ti], r.mode == M_AIR ? 0.0f : laneW));
    tgt.y = r.mode == M_AIR ? airAlt[ti] + 1.0f : th[ti];
    if (dodge && r.mode != M_AIR && dodgeAlong < 35) { tgt = dodgeP; tgt.y = th[ti]; }   // steer around the obstacle, not just toward a far lane
    // tank shortcuts: smash the barricade (tanks) or use an open path
    bool onShortcut = false;
    if (r.mode == M_LAND)
        for (size_t k = 0; k < shortcuts.size(); k++) {
            const Shortcut& s = shortcuts[k];
            int rel = (r.lastIdx - s.a + N) % N; if (rel > N / 2) rel -= N;
            int span = (s.b - s.a + N) % N;
            if (rel < -30 || rel > span + 6 || !aiWantsShortcut(r, (int)k)) continue;
            V3 d = sub(r.pos, s.pa); d.y = 0;
            float u = dot(d, s.dir), v = dot(d, s.nrm);
            if (fabsf(v) < s.w / 2 + 2 && u > -3 && u < s.len) {          // in the corridor: follow it
                tgt = add(s.pa, mul(s.dir, std::min(s.len + 10.0f, u + 20.0f))); tgt.y = r.pos.y; onShortcut = true;
            } else if (rel < 0 && rel > -30) {                              // approaching: line up with the entry
                tgt = add(s.wallP, mul(s.dir, 6.0f)); tgt.y = r.pos.y; onShortcut = true;
            }
            break;
        }
    // chase item boxes when empty-handed
    if (itemsOn() && r.item == IT_NONE && r.rouletteT <= 0 && r.mode != M_AIR && !onShortcut) {
        for (const ItemBox& b : itemBoxes) {
            if (b.t > 0) continue;
            V3 d = sub(b.p, r.pos); float along = dot(d, fw), side = fabsf(dot(d, rgtV(r.yaw)));
            if (along > 8 && along < 60 && side < along * 0.5f && fabsf(d.y) < 6) { tgt = b.p; break; }
        }
    }
    const Racer* victim = nullptr; float bestD = 60.0f;
    for (auto& o : racers) {
        if (o.id == r.id || o.dead || o.out) continue;
        V3 d = sub(o.pos, r.pos); float dl = len(d);
        if (dl < bestD && dl > 1 && dot(fw, mul(d, 1.0f / dl)) > 0.55f && fabsf(d.y) < 10.0f) { bestD = dl; victim = &o; }
    }
    float aggroNeed = S.mode == GM_BATTLE ? 0.35f : 0.75f + (2 - S.difficulty) * 0.08f;
    if (!r.isPlayer && victim && r.aggro > aggroNeed && !onShortcut) tgt = add(victim->pos, mul(victim->vel, 0.25f));
    else if (victim && bestD < 25) {
        float side = dot(sub(victim->pos, r.pos), rgtV(r.yaw)) > 0 ? -1.0f : 1.0f;
        tgt = add(tgt, mul(rgtV(r.yaw), side * 6.0f));
    }
    V3 toT = sub(tgt, r.pos);
    float diff = wrapA(atan2f(toT.x, toT.z) - r.yaw), ad = fabsf(diff);
    float spd = r.mode == M_AIR ? r.airSpeed : dot(r.vel, fw);
    in.steer = clampf(-diff * 2.4f, -1, 1);
    const Stats& st = ST[r.base][r.mode];
    if (r.mode == M_AIR) {
        float hd = std::max(25.0f, sqrtf(toT.x * toT.x + toT.z * toT.z));
        float des = clampf(atan2f(toT.y, hd), -0.6f, 0.6f);
        in.throttle = clampf((des - r.pitch) * 3.0f, -1, 1);
        in.boost = ad < 0.1f && r.energy > 55 && GetRandomValue(0, 1000) < 20;
    } else {
        bool wheeled = r.base == B_CAR || r.base == B_KART;
        float aLat = (r.base == B_KART ? 22.0f : wheeled ? 20.0f : r.base == B_TANK ? 17.0f : 20.0f) * (r.mode == M_WATER ? 0.55f : 1.0f) * weatherGrip(r.base, r.mode) * trackGrip(r.base, theme[(r.lastIdx + 20) % N], r.mode);
        float mc = 0.0001f;
        for (int k = 2; k <= 40; k += 4) mc = std::max(mc, curv[(r.lastIdx + k) % N]);
        float vt = clampf(sqrtf(aLat / mc), 14.0f, st.vmax * 0.98f * std::max(r.rubber, 0.9f));
        if (spd < vt) in.throttle = 1.0f; else if (spd > vt + 5.0f) in.throttle = -0.7f; else in.throttle = 0.3f;
        if (ad > 1.2f) in.throttle = std::min(in.throttle, 0.15f);
        in.boost = ad < 0.1f && r.energy > 45 && spd >= vt - 2.0f;
        // drift through corners for a mini-turbo, release when straight again
        if (wheeled) {
            float cnow = curv[(r.lastIdx + 6) % N];
            if (r.in.special) in.special = cnow > 0.0035f || fabsf(in.steer) > 0.35f;
            else in.special = cnow > 0.006f && spd > 26 && fabsf(in.steer) > 0.3f;
        }
        if (r.base == B_ROBOT && r.mode == M_LAND && victim && GetRandomValue(0, 1000) < 15) in.special2 = true;
        float lat = r.lateral, L = lim[r.lastIdx];
        if (fabsf(lat) > L - 4.0f && !onShortcut && r.scIdx < 0) in.steer = clampf(in.steer + sgn(lat) * 0.6f, -1, 1);
        if (onShortcut && r.base == B_TANK) { in.throttle = 1.0f; in.special = false; }
    }
    if (r.base == B_TANK && r.mode != M_AIR && victim && bestD < 45 && ad < 0.08f && GetRandomValue(0, 100) < 4) in.special = true;
    if (!r.dead && r.mode != M_AIR && spd < 2.5f) r.stuckTimer += dt; else r.stuckTimer = 0;
    if (r.stuckTimer > 3.0f || r.wrongT > 4.0f) respawn(r);
    r.in = in;
}

// =====================================================================
//        GAME MODES, CAREER, SAVE FILE, GHOST, REPLAY RECORDING
// =====================================================================
// weather column: 0 dynamic, 1 clear, 2 rain, 3 fog, 4 snow, 5 storm.  boss: 0 none, 1..4
struct CareerEv { const char* ru; const char* en; int mode, track, laps, opp, tod, weather, diff, reward; float goal; int boss; };
static const int CAREER_N = 20;
static const CareerEv CAREER[CAREER_N] = {
    {"\320\237\320\265\321\200\320\262\321\213\320\271 \320\267\320\260\320\265\320\267\320\264", "First Ride", GM_RACE, 1, 2, 3, TD_DAY, 1, 0, 800, 0, 0},
    {"\320\235\320\265\320\276\320\275\320\276\320\262\320\260\321\217 \320\275\320\276\321\207\321\214", "Neon Night", GM_RACE, 0, 1, 5, TD_NIGHT, 1, 1, 1200, 0, 0},
    {"\320\237\321\200\320\276\321\202\320\270\320\262 \321\207\320\260\321\201\320\276\320\262", "Beat the Clock", GM_TIMETRIAL, 1, 1, 0, TD_SUNSET, 1, 1, 1000, 0, 0},
    {"\320\236\320\261\320\273\320\260\320\262\320\260", "Dragnet", GM_PURSUIT, 0, 0, 0, TD_NIGHT, 1, 1, 1500, 120, 0},
    {"\320\221\320\236\320\241\320\241: \320\226\320\265\320\273\320\265\320\267\320\275\321\213\320\271 \320\221\320\260\321\200\320\276\320\275", "BOSS: Iron Baron", GM_RACE, 0, 2, 4, TD_NIGHT, 1, 1, 2500, 0, 1},
    {"\320\237\320\276\321\201\320\273\320\265\320\264\320\275\320\270\320\271 \320\262\321\213\320\261\321\213\320\262\320\260\320\265\321\202", "Last One Out", GM_ELIM, 1, 0, 5, TD_SUNSET, 1, 1, 1600, 0, 0},
    {"\320\233\320\265\320\264\321\217\320\275\320\276\320\271 \320\277\320\265\321\200\320\265\320\262\320\260\320\273", "Frost Pass", GM_RACE, 3, 2, 5, TD_DAY, 4, 1, 2000, 0, 0},
    {"\320\223\321\200\320\276\320\267\320\276\320\262\320\276\320\271 \321\200\320\265\320\271\321\201", "Storm Run", GM_RACE, 0, 2, 7, TD_NIGHT, 5, 1, 2000, 0, 0},
    {"\320\237\320\273\321\217\320\266\320\275\320\260\321\217 \320\277\320\276\320\263\320\276\320\275\321\217", "Beach Chase", GM_PURSUIT, 1, 0, 0, TD_DAY, 1, 1, 2200, 150, 0},
    {"\320\221\320\236\320\241\320\241: \320\241\320\275\320\265\320\266\320\275\320\260\321\217 \320\232\320\276\321\200\320\276\320\273\320\265\320\262\320\260", "BOSS: Snow Queen", GM_RACE, 3, 2, 5, TD_NIGHT, 4, 1, 3500, 0, 2},
    {"\320\237\321\203\321\201\321\202\321\213\320\275\320\275\321\213\320\271 \320\274\320\270\321\200\320\260\320\266", "Desert Mirage", GM_RACE, 4, 2, 6, TD_DAY, 1, 1, 2500, 0, 0},
    {"\320\221\320\270\321\202\320\262\320\260 \320\262 \320\272\321\200\320\260\321\202\320\265\321\200\320\265", "Crater Brawl", GM_BATTLE, 4, 0, 5, TD_SUNSET, 1, 1, 2600, 0, 0},
    {"\320\227\320\260\320\272\320\260\321\202 \320\275\320\260 \320\277\320\276\320\261\320\265\321\200\320\265\320\266\321\214\320\265", "Coastal Sunset", GM_RACE, 1, 3, 7, TD_SUNSET, 1, 2, 2800, 0, 0},
    {"\320\235\320\276\321\207\320\275\320\276\320\265 \320\262\321\213\320\261\321\213\320\262\320\260\320\275\320\270\320\265", "Night Elimination", GM_ELIM, 0, 0, 7, TD_NIGHT, 3, 2, 3000, 0, 0},
    {"\320\221\320\236\320\241\320\241: \320\234\320\260\320\263\320\274\320\260", "BOSS: Magma", GM_RACE, 4, 2, 5, TD_NIGHT, 1, 2, 4500, 0, 3},
    {"\320\241\320\270\320\263\320\275\320\260\320\273 \320\270\320\267 \320\227\320\260\320\272\321\203\320\273\320\270\321\201\321\214\321\217", "Signal from the Backrooms", GM_RACE, 2, 2, 5, TD_NIGHT, 1, 1, 3500, 0, 0},
    {"\320\221\320\260\321\201\321\201\320\265\320\271\320\275\321\213\320\265 \320\272\320\276\320\274\320\275\320\260\321\202\321\213", "The Poolrooms", GM_TIMETRIAL, 2, 2, 0, TD_NIGHT, 1, 1, 3500, 0, 0},
    {"\320\222\321\213\321\205\320\276\320\264 \320\275\320\260 \320\276\321\200\320\261\320\270\321\202\321\203", "Into Orbit", GM_RACE, 5, 2, 6, TD_NIGHT, 1, 2, 4000, 0, 0},
    {"\320\235\320\265\320\262\320\265\321\201\320\276\320\274\320\276\321\201\321\202\321\214", "Zero-G Brawl", GM_BATTLE, 5, 0, 7, TD_NIGHT, 1, 2, 4200, 0, 0},
    {"\320\244\320\230\320\235\320\220\320\233: \320\232\320\276\320\274\320\260\320\275\320\264\320\276\321\200 \320\234\320\265\320\263\320\260", "FINAL: Commander Mega", GM_RACE, 5, 3, 7, TD_NIGHT, 1, 2, 8000, 0, 4}};
static Settings savedS;
static bool careerAvailable(int i) { return i == 0 || P.career[i - 1] > 0; }
static bool backroomsUnlocked() { return P.backrooms || P.career[13] > 0; }
static bool paintUnlocked(int p) { return p < NPAINT_FREE || (p == 12 && (P.bossMask & 1)) || (p == 13 && (P.bossMask & 2)); }
static bool decalUnlocked(int d) { return d < 6 || (d == 6 && (P.bossMask & 4)) || (d == 7 && (P.bossMask & 8)); }
static int curBoss() { return careerEvent >= 0 ? CAREER[careerEvent].boss : 0; }
static float battleT = 0;
static void metaAfterRace(int myPos);     // achievements + daily challenge (next module)

// ------------------------------ save file ----------------------------
static std::string dataPath(const char* name) { return std::string(GetApplicationDirectory()) + name; }
static void saveProfile() {
    std::string s;
    char b[512];
    snprintf(b, sizeof b, "money %d\n", P.money); s += b;
    snprintf(b, sizeof b, "upg %d %d %d %d\n", P.upg[0], P.upg[1], P.upg[2], P.upg[3]); s += b;
    snprintf(b, sizeof b, "owned %d %d %d %d\n", P.owned[0], P.owned[1], P.owned[2], P.owned[3]); s += b;
    snprintf(b, sizeof b, "backrooms %d\n", P.backrooms); s += b;
    s += "bestlap"; for (int i = 0; i < NUM_TRACKS; i++) { snprintf(b, sizeof b, " %.3f", P.bestLap[i]); s += b; } s += "\n";
    s += "bestrace"; for (int i = 0; i < NUM_TRACKS; i++) { snprintf(b, sizeof b, " %.3f", P.bestRace[i]); s += b; } s += "\n";
    s += "career2"; for (int i = 0; i < CAREER_MAX; i++) { snprintf(b, sizeof b, " %d", P.career[i]); s += b; } s += "\n";
    snprintf(b, sizeof b, "meta %llu %d %d %d %d %d %d %d %d %d %d %d %d %d\n", P.ach, P.bossMask, P.wins, P.tricks, P.rings, P.ultras, P.drafts, P.walls,
             P.dailyDone, P.dailyDay, P.earned, P.battles, P.escapes, P.races2p); s += b;
    snprintf(b, sizeof b, "stats %d %d\n", P.totalTakedowns, P.totalCoins); s += b;
    const Settings& Q = careerEvent >= 0 ? savedS : S;
    snprintf(b, sizeof b, "settings %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", Q.laps, Q.opponents, Q.difficulty, Q.tod, Q.weather, S.sound, S.music, S.post,
             S.paint, (int)S.base, Q.mode, Q.track, Q.items, Q.traffic, langSel); s += b;
    snprintf(b, sizeof b, "settings2 %d %d %d %d %d %d %d %d %d %d\n", Q.players, (int)S.base2, S.paint2, S.decal, S.driver, S.driver2, S.quality, S.vibration, S.musicSrc, S.showFps); s += b;
    FILE* f = fopen(dataPath("megarace_save.txt").c_str(), "wb");
    if (f) { fwrite(s.data(), 1, s.size(), f); fclose(f); }
}
static void loadProfile() {
    FILE* f = fopen(dataPath("megarace_save.txt").c_str(), "rb");
    if (!f) return;
    char line[1024];
    bool haveCareer2 = false; int oldCareer[16] = {0};
    auto floats = [](const char* v, float* out, int n) { int i = 0; char* e; const char* p = v; while (i < n) { float x = strtof(p, &e); if (e == p) break; out[i++] = x; p = e; } };
    while (fgets(line, sizeof line, f)) {
        char key[32] = {0};
        if (sscanf(line, "%31s", key) != 1) continue;
        std::string k = key; const char* v = line + k.size();
        if (k == "money") sscanf(v, "%d", &P.money);
        else if (k == "upg") sscanf(v, "%d %d %d %d", &P.upg[0], &P.upg[1], &P.upg[2], &P.upg[3]);
        else if (k == "owned") sscanf(v, "%d %d %d %d", &P.owned[0], &P.owned[1], &P.owned[2], &P.owned[3]);
        else if (k == "career") {      // pre-v4 career layout (12 events)
            int idx = 0; char* tok = strtok(line + k.size(), " \r\n");
            while (tok && idx < 16) { oldCareer[idx++] = atoi(tok); tok = strtok(nullptr, " \r\n"); }
        }
        else if (k == "career2") {
            haveCareer2 = true;
            int idx = 0; char* tok = strtok(line + k.size(), " \r\n");
            while (tok && idx < CAREER_MAX) { P.career[idx++] = atoi(tok); tok = strtok(nullptr, " \r\n"); }
        }
        else if (k == "meta") {
            sscanf(v, "%llu %d %d %d %d %d %d %d %d %d %d %d %d %d", &P.ach, &P.bossMask, &P.wins, &P.tricks, &P.rings, &P.ultras, &P.drafts, &P.walls,
                   &P.dailyDone, &P.dailyDay, &P.earned, &P.battles, &P.escapes, &P.races2p);
        }
        else if (k == "settings2") {
            int a[10]; int n = sscanf(v, "%d %d %d %d %d %d %d %d %d %d", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7], &a[8], &a[9]);
            if (n == 10) {
                S.players = a[0] == 2 ? 2 : 1; S.base2 = (Base)(((a[1] % B_COUNT) + B_COUNT) % B_COUNT); S.paint2 = ((a[2] % NPAINT_FREE) + NPAINT_FREE) % NPAINT_FREE;
                S.decal = ((a[3] % NDECAL) + NDECAL) % NDECAL; S.driver = ((a[4] % NDRIVER) + NDRIVER) % NDRIVER; S.driver2 = ((a[5] % NDRIVER) + NDRIVER) % NDRIVER;
                S.quality = (int)clampf((float)a[6], 0, 2); S.vibration = a[7] != 0; S.musicSrc = a[8] != 0; S.showFps = a[9] != 0;
            }
        }
        else if (k == "backrooms") sscanf(v, "%d", &P.backrooms);
        else if (k == "bestlap") floats(v, P.bestLap, NUM_TRACKS);
        else if (k == "bestrace") floats(v, P.bestRace, NUM_TRACKS);
        else if (k == "stats") sscanf(v, "%d %d", &P.totalTakedowns, &P.totalCoins);
        else if (k == "settings") {
            int a[15]; int n = sscanf(v, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7], &a[8], &a[9], &a[10], &a[11], &a[12], &a[13], &a[14]);
            if (n == 15) {
                S.laps = clampf((float)a[0], 1, 5); S.opponents = (int)clampf((float)a[1], 1, 7); S.difficulty = a[2] % 3; S.tod = a[3] % 3; S.weather = a[4] % 6;
                S.sound = a[5] != 0; S.music = (int)clampf((float)a[6], 0, 10); S.post = a[7] != 0; S.paint = ((a[8] % NPAINT) + NPAINT) % NPAINT; S.base = (Base)(a[9] % B_COUNT);
                S.mode = a[10] % GM_COUNT; S.track = a[11] % NUM_TRACKS; S.items = a[12] != 0; S.traffic = a[13] != 0; if (cyrOK) langSel = a[14] ? 1 : 0;
            }
        }
    }
    fclose(f);
    if (!haveCareer2) {   // map the old 12-event career onto the new list
        const int map[12] = {0, 1, 2, 3, 5, 7, 8, 12, 13, 15, 16, -1};
        for (int i = 0; i < 12; i++) if (map[i] >= 0) P.career[map[i]] = oldCareer[i];
    }
    P.owned[0] = 1; P.owned[3] = 1;
    if (!P.owned[S.base]) S.base = B_CAR;
    if (!P.owned[S.base2]) S.base2 = B_KART;
    if (!paintUnlocked(S.paint)) S.paint = 0;
    if (!decalUnlocked(S.decal)) S.decal = 1;
    if (S.track == 2 && !backroomsUnlocked()) S.track = 0;
}

// ------------------------------ ghost --------------------------------
struct GhostFrame { float t; V3 p; float yaw, pitch, roll, ma, mw; int idx; unsigned char mode, base, pad1, pad2; };
static std::vector<GhostFrame> ghostRec, ghostBest;
static float ghostBestTime = 0, ghostRecT = 0; static int ghostTrack = -1;
static void saveGhost() {
    FILE* f = fopen(dataPath(TextFormat("megarace_ghost_%d.bin", curTrack)).c_str(), "wb");
    if (!f) return;
    int n = (int)ghostBest.size();
    fwrite(&n, sizeof n, 1, f); fwrite(&ghostBestTime, sizeof ghostBestTime, 1, f);
    if (n) fwrite(ghostBest.data(), sizeof(GhostFrame), n, f);
    fclose(f);
}
static void loadGhost(int tr) {
    if (ghostTrack == tr) return;
    ghostTrack = tr; ghostBest.clear(); ghostBestTime = 0;
    FILE* f = fopen(dataPath(TextFormat("megarace_ghost_%d.bin", tr)).c_str(), "rb");
    if (!f) return;
    int n = 0; float t = 0;
    if (fread(&n, sizeof n, 1, f) == 1 && fread(&t, sizeof t, 1, f) == 1 && n > 0 && n < 200000) {
        ghostBest.resize(n);
        if (fread(ghostBest.data(), sizeof(GhostFrame), n, f) == (size_t)n) ghostBestTime = t; else { ghostBest.clear(); }
    }
    fclose(f);
}
static bool ghostSample(float t, GhostFrame& out) {
    if (ghostBest.size() < 2 || t < 0 || t > ghostBest.back().t) return false;
    size_t lo = 0, hi = ghostBest.size() - 1;
    while (hi - lo > 1) { size_t m = (lo + hi) / 2; if (ghostBest[m].t <= t) lo = m; else hi = m; }
    const GhostFrame &a = ghostBest[lo], &b = ghostBest[hi];
    float k = (b.t - a.t) > 1e-4f ? (t - a.t) / (b.t - a.t) : 0;
    out = a; out.p = add(a.p, mul(sub(b.p, a.p), k)); out.yaw = a.yaw + wrapA(b.yaw - a.yaw) * k;
    out.pitch = lerpf(a.pitch, b.pitch, k); out.roll = lerpf(a.roll, b.roll, k);
    return true;
}
static float ghostTimeAtIdx(int idx) {     // time the ghost passed a track index (for the split delta)
    for (const GhostFrame& g : ghostBest) if (std::abs(g.idx - idx) < 3) return g.t;
    return -1;
}

// ------------------------------ replay -------------------------------
struct RecActor { V3 p; float yaw, pitch, roll, spin, ma, mw, steer; unsigned char base, mode, flags, id; Color c; };
struct RecTraffic { V3 p; float yaw; unsigned char type, wreck; Color c; };
struct RecFrame { float t; std::vector<RecActor> a; std::vector<RecTraffic> tr; };
static std::vector<RecFrame> replay;
static float recT = 0;
static void recordFrame() {
    RecFrame f; f.t = raceTime;
    for (const Racer& r : racers) {
        RecActor a; a.p = r.pos; a.yaw = r.yaw; a.pitch = r.pitch; a.roll = r.roll; a.spin = r.spinAng; a.ma = r.morphAir; a.mw = r.morphWater; a.steer = r.steerV;
        a.base = (unsigned char)r.base; a.mode = (unsigned char)r.mode; a.id = (unsigned char)r.id; a.c = r.color;
        a.flags = (r.dead ? 1 : 0) | (r.boosting || r.boostT > 0 ? 2 : 0) | (r.isCop ? 4 : 0) | (r.shieldT > 0 ? 8 : 0) | (r.shrinkT > 0 ? 16 : 0) | (r.out ? 32 : 0) | (r.isPlayer ? 64 : 0);
        f.a.push_back(a);
    }
    for (const Traffic& t : traffic) { if (t.dead) continue; RecTraffic q; q.p = t.pos; q.yaw = t.yaw; q.type = (unsigned char)t.type; q.wreck = t.wreck; q.c = t.c; f.tr.push_back(q); }
    replay.push_back(f);
}

// ------------------------------ events -------------------------------
static float heatBonus = 0, roadblockT = 40;
static void onRacerWrecked(Racer& r) {
    if (r.isCop) r.deadTimer = 5.0f;
    bool recent = gTime - r.lastHitT < 2.5f;
    if (r.lastHitBy >= 0 && r.lastHitBy < (int)racers.size() && r.lastHitBy != r.id && recent && racers[r.lastHitBy].isPlayer) {
        Racer& h = racers[r.lastHitBy];
        h.takedowns++;
        if (numHumans < 2) { slowmoT = 1.3f; takedownVictim = r.id; }
        takedownName = r.name; takedownBannerT = 2.4f;
        notifyP(h, std::string(T("\320\222\320\253\320\221\320\230\320\242: ", "TAKEDOWN: ")) + r.name, C(255, 90, 60), 2.0f);
        play(sCash, 0.8f); rumblePad(h.pid, 0.9f, 0.35f);
        if (r.isCop) heatBonus += 0.4f;
    }
    popBalloon(r, recent ? r.lastHitBy : -1);
}
// battle: lose a balloon; the last balloon knocks you out of the arena
static void popBalloon(Racer& v, int attacker) {
    if (S.mode != GM_BATTLE || v.out || v.isCop || v.balloons <= 0 || v.popCd > 0 || resultState != 0 || countdown > 1.0f || raceTime < 6.0f) return;
    v.balloons--; v.popCd = 2.5f;
    burst(add(v.pos, vec(0, 3.8f, 0)), 26, 12, balloonCol(v.balloons), 0.6f, 0.8f, false, true);
    if (dist2D(v.pos, camPos) < 200) play(sTick, 1.0f, 0.5f);
    if (attacker >= 0 && attacker < (int)racers.size() && attacker != v.id) {
        racers[attacker].points++;
        if (racers[attacker].isPlayer) notifyP(racers[attacker], T("\320\250\320\220\320\240\320\230\320\232 \320\233\320\236\320\237\320\235\320\243\320\233! +1", "BALLOON POPPED! +1"), C(255, 220, 60), 1.6f);
    }
    if (v.isPlayer) { notifyP(v, v.balloons > 0 ? TextFormat(T("\320\236\320\241\320\242\320\220\320\233\320\236\320\241\320\254 \320\250\320\220\320\240\320\230\320\232\320\236\320\222: %d", "BALLOONS LEFT: %d"), v.balloons) : T("\320\222\320\241\320\225 \320\250\320\220\320\240\320\230\320\232\320\230 \320\233\320\236\320\237\320\235\320\243\320\233\320\230!", "ALL BALLOONS GONE!"), C(255, 120, 80), 2.0f); rumblePad(v.pid, 0.7f, 0.3f); }
    if (v.balloons == 0) {
        int act = 0; for (auto& r : racers) if (!r.out && !r.isCop) act++;
        v.out = true; v.finishPlace = act;
        burst(v.pos, 50, 30, C(255, 120, 40), 1.0f, 1.6f, true, true);
        notify(std::string(v.name) + T(" \320\222\320\253\320\221\320\253\320\222\320\220\320\225\320\242!", " IS OUT!"), C(255, 90, 60), 3.0f);
        bool anyHuman = false; for (auto& r : racers) if (r.isPlayer && !r.out) anyHuman = true;
        if (v.isPlayer && !anyHuman) { resultState = 2; finishBannerT = 4.0f; }
    }
}
static void onLapComplete(Racer& r, float lapTime) {
    if (!r.isPlayer || r.pid != 0) return;
    if (P.bestLap[curTrack] <= 0 || lapTime < P.bestLap[curTrack]) {
        P.bestLap[curTrack] = lapTime;
        notifyP(r, T("\320\240\320\225\320\232\320\236\320\240\320\224 \320\232\320\240\320\243\320\223\320\220!", "LAP RECORD!"), C(120, 255, 160), 2.5f);
    }
    if (S.mode == GM_TIMETRIAL) {
        if (ghostBestTime <= 0 || lapTime < ghostBestTime) { ghostBest = ghostRec; ghostBestTime = lapTime; ghostTrack = curTrack; saveGhost(); }
    }
    ghostRec.clear(); ghostRecT = 0;
}

// ------------------------------ race setup ---------------------------
static void applyUpgrades(Racer& r) {
    r.upV = 1.0f + 0.035f * P.upg[0]; r.upA = 1.0f + 0.07f * P.upg[0];
    r.upN = 1.0f - 0.1f * P.upg[1]; r.upArmor = 1.0f - 0.11f * P.upg[2]; r.upGrip = 1.0f + 0.07f * P.upg[3];
}
static void resetRace() {
    if (careerEvent >= 0) {
        const CareerEv& e = CAREER[careerEvent];
        S.mode = e.mode; S.track = e.track; S.laps = std::max(1, e.laps); S.opponents = std::max(1, e.opp); S.tod = e.tod; S.weather = e.weather;
        S.difficulty = e.diff; S.items = 1; S.traffic = 1;
        pursuitGoal = e.goal > 0 ? e.goal : 150;
    } else pursuitGoal = 150;
    if (curTrack != S.track || N == 0) { buildTrack(S.track); placeProps(); }
    numHumans = (careerEvent < 0 && S.players == 2 && S.mode != GM_TIMETRIAL && S.mode != GM_PURSUIT) ? 2 : 1;
    racers.clear(); shells.clear(); parts.clear(); notes.clear(); rocketsP.clear(); hazards.clear(); traffic.clear(); replay.clear(); lavaBombs.clear();
    layoutItemBoxes(S.mode == GM_BATTLE ? 120 : 280);
    for (auto& sc : shortcuts) { sc.open = false; sc.openT = 0; }
    lavaBombT = 4.0f; battleT = 180.0f; shakeP[0] = shakeP[1] = 0;
    for (auto& b : itemBoxes) b.t = 0;
    for (auto& c : coinsW) c.t = 0;
    for (auto& d : dyns) { d.hit = false; d.p = d.home; d.v = vec(0, 0, 0); d.rot = 0; d.t = 0; }
    finishCount = 0; raceTime = 0; countdown = 4.0f; raceOver = false; shakeAmt = 0; finishBannerT = 0;
    slowmoT = 0; takedownBannerT = 0; elimT = 30; pursuitT = 0; heat = 1; heatBonus = 0; bustedT = 0; copSpawnT = 4; roadblockT = 40; resultState = 0; recT = 0;
    numRacers = (S.mode == GM_TIMETRIAL || S.mode == GM_PURSUIT) ? 1 : std::min(8, numHumans + S.opponents);
    if (S.weather == 0) setWeather(W_CLEAR, false); else setWeather((Weather)(S.weather - 1), false);
    wI = 1.0f;
    if (S.weather == 0) weatherTimer = frand(35, 55);
    atmoInit = false; updateAtmosphere(0, true);
    genFacades(S.tod);
    const float ttSpeed[NUM_TRACKS] = {56, 56, 44, 50, 52, 52};
    ttTarget = S.laps * trackLen / ttSpeed[curTrack];
    const char* names[7] = {"RAZOR", "VIPER", "GHOST", "BLAZE", "NOVA", "TITAN", "ONYX"};
    int p2paint = (S.paint + 4) % NPAINT_FREE;
    std::vector<int> cols; for (int k = 0; k < NPAINT_FREE; k++) if (k != S.paint % NPAINT_FREE && (numHumans < 2 || k != p2paint)) cols.push_back(k);
    int boss = curBoss();
    for (int k = 0; k < numRacers; k++) {
        Racer r; r.id = k; r.isPlayer = k < numHumans; r.pid = r.isPlayer ? k : -1;
        r.color = k == 0 ? PAINT[S.paint] : (r.isPlayer ? PAINT[p2paint] : PAINT[cols[(k - numHumans) % cols.size()]]);
        r.number = k + 1;
        if (r.isPlayer) {
            int d = k == 0 ? S.driver : S.driver2;
            r.name = driverName(d); r.helmet = DRIVERS[d].helmet; r.driverStyle = DRIVERS[d].style;
            r.decal = k == 0 ? S.decal : (S.decal + 2) % 6; r.color2 = k == 0 ? PAINT[S.paint2] : WHITE;
            r.base = k == 0 ? S.base : S.base2;
            if ((int)r.base < 0 || (int)r.base >= B_COUNT) r.base = B_CAR;
            if (!baseAllowed(r.base)) r.base = k == 0 ? B_CAR : B_KART;
        } else {
            r.name = names[(k - numHumans) % 7]; r.helmet = PAINT[(k * 3 + 2) % NPAINT_FREE]; r.driverStyle = GetRandomValue(0, NDRIVER - 1);
            r.decal = GetRandomValue(0, 5); r.color2 = PAINT[GetRandomValue(0, NPAINT_FREE - 1)];
            r.base = (Base)GetRandomValue(0, B_COUNT - 1);
            if (!baseAllowed(r.base)) r.base = GetRandomValue(0, 1) ? B_CAR : B_KART;
        }
        if (boss && k == numHumans) {   // the boss starts on the front row next to you
            const Base bb[5] = {B_CAR, B_TANK, B_KART, B_ROBOT, B_CAR};
            const Color bc[5] = {WHITE, C(40, 38, 36), C(210, 240, 255), C(200, 40, 20), C(110, 40, 200)};
            const Color b2[5] = {WHITE, C(255, 190, 40), C(120, 200, 255), C(255, 170, 30), C(255, 210, 60)};
            r.boss = boss; r.base = bb[boss]; r.color = bc[boss]; r.color2 = b2[boss]; r.decal = boss == 3 ? 6 : boss == 4 ? 7 : 1;
            r.name = bossName(boss); r.skill = 1.06f; r.aggro = 0.9f; r.bossT = 9.0f;
        }
        int slot = k;
        int idx = N - 10 - (slot / 2) * 3;
        float lane = (slot % 2) ? 5.0f : -5.0f;
        r.pos = add(track[idx], mul(norv[idx], lane)); r.pos.y = th[idx]; r.lastGround = r.pos.y;
        r.yaw = atan2f(tanv[idx].x, tanv[idx].z); r.lastIdx = idx;
        r.lane = frand(-6, 6); r.aggro = r.boss ? 0.9f : frand(0, 1); r.aiTimer = r.boss ? 1e9f : frand(40, 90); if (!r.boss) r.skill = frand(0.97f, 1.03f);
        if (S.mode == GM_BATTLE) { r.balloons = 3; r.points = 0; }
        r.ringHit.assign(rings.size(), 0);
        if (r.isPlayer) applyUpgrades(r);
        racers.push_back(r);
    }
    if (S.mode == GM_PURSUIT) { spawnCop(false, -5); spawnCop(false, 5); }
    if (S.mode == GM_TIMETRIAL) { loadGhost(curTrack); ghostRec.clear(); ghostRecT = 0; }
    aP.musicStyle = indoorTrack() ? 2 : spaceTrack() ? 0 : 1;
    aP.hum = indoorTrack() ? 1.0f : 0.0f;
    hintT = 7.0f;
}

// ------------------------------ per-frame mode logic -----------------
static void updateModes(float dt) {
    if (racers.empty() || countdown > 1.0f) return;
    Racer& pl = racers[0];
    if (S.mode == GM_BATTLE && resultState == 0) {
        battleT -= dt;
        int act = 0; for (auto& r : racers) if (!r.out && !r.isCop) act++;
        if (act <= 1 || battleT <= 0) {
            int dummy; computeOrder(dummy);
            int place = 1;
            for (int id : order) { Racer& r = racers[id]; if (!r.out) { r.finished = true; r.finishPlace = place; r.finishTime = raceTime; } place++; }
            const Racer& w = racers[order[0]];
            resultState = w.isPlayer ? 1 : 2; finishBannerT = 4.0f; play(sGo, 0.8f, 0.8f);
            notify(std::string(T("\320\237\320\236\320\221\320\225\320\224\320\230\320\242\320\225\320\233\320\254: ", "WINNER: ")) + w.name, C(255, 220, 60), 4.0f);
        }
        return;
    }
    if (S.mode == GM_TIMETRIAL && pl.lap >= 1 && !pl.finished) {
        ghostRecT -= dt;
        if (ghostRecT <= 0) {
            ghostRecT = 0.05f;
            GhostFrame g; g.t = raceTime - pl.lapStart; g.p = pl.pos; g.yaw = pl.yaw; g.pitch = pl.pitch; g.roll = pl.roll; g.ma = pl.morphAir; g.mw = pl.morphWater;
            g.idx = pl.lastIdx; g.mode = (unsigned char)pl.mode; g.base = (unsigned char)pl.base; g.pad1 = g.pad2 = 0;
            ghostRec.push_back(g);
        }
    }
    if (resultState != 0) return;
    if (S.mode == GM_ELIM) {
        int act = 0; for (auto& r : racers) if (!r.out && !r.isCop) act++;
        elimT -= dt;
        if (elimT <= 0 && act > 1) {
            int dummy; computeOrder(dummy);
            int last = -1;
            for (int i = (int)order.size() - 1; i >= 0; i--) if (!racers[order[i]].out) { last = order[i]; break; }
            if (last >= 0) {
                Racer& v = racers[last];
                v.out = true; v.finishPlace = act;
                burst(v.pos, 50, 30, C(255, 120, 40), 1.0f, 1.6f, true, true);
                play(sBoom, 0.8f, 0.8f);
                notify(std::string(v.name) + T(" \320\222\320\253\320\221\320\253\320\222\320\220\320\225\320\242!", " IS OUT!"), C(255, 90, 60), 3.0f);
                bool anyHuman = false; for (auto& r : racers) if (r.isPlayer && !r.out) anyHuman = true;
                if (v.isPlayer && !anyHuman) { resultState = 2; finishBannerT = 4.0f; }
                act--;
            }
            elimT = act > 4 ? 30.0f : 25.0f;
        }
        if (act == 1 && resultState == 0) {
            for (auto& r : racers) if (!r.out && !r.isCop) { r.finished = true; r.finishPlace = 1; r.finishTime = raceTime; }
            bool humanWon = false; for (auto& r : racers) if (r.isPlayer && !r.out) humanWon = true;
            if (humanWon) { resultState = 1; finishBannerT = 4.0f; play(sGo, 0.8f, 0.8f); }
        }
    }
    if (S.mode == GM_PURSUIT) {
        pursuitT += dt;
        heat = std::min(5.0f, 1.0f + pursuitT / 45.0f + heatBonus);
        int target = std::min(5, 1 + (int)(heat * 0.8f));
        int totalCops = 0; for (auto& r : racers) if (r.isCop) totalCops++;
        copSpawnT -= dt; roadblockT -= dt;
        if (copsAlive() < target && copSpawnT <= 0 && totalCops < 10) { spawnCop(false, frand(-6, 6)); copSpawnT = 5.0f; }
        if (heat >= 4 && roadblockT <= 0 && totalCops < 9) { spawnCop(true, -6); spawnCop(true, 6); roadblockT = 45; notify(T("\320\222\320\237\320\225\320\240\320\225\320\224\320\230 \320\227\320\220\320\241\320\233\320\236\320\235!", "ROADBLOCK AHEAD!"), C(255, 80, 80), 2.5f); }
        Racer& p0 = racers[0];   // re-fetch: spawning may reallocate
        float nearest = 1e9f;
        for (auto& r : racers) if (r.isCop && !r.dead && !r.parked) nearest = std::min(nearest, len(sub(r.pos, p0.pos)));
        if (speedOf(p0) < 4 && nearest < 12 && !p0.dead) bustedT += dt; else bustedT = std::max(0.0f, bustedT - dt * 0.7f);
        aP.siren = clampf(1.0f - nearest / 260.0f, 0, 1);
        if (bustedT > 4.0f) { resultState = 2; finishBannerT = 4.0f; play(sBoom, 0.4f, 0.6f); }
        else if (pursuitT >= pursuitGoal) { resultState = 1; finishBannerT = 4.0f; p0.finished = true; p0.finishPlace = 1; p0.finishTime = raceTime; play(sGo, 0.8f, 0.8f); }
    } else aP.siren = 0;
}

// ------------------------------ rewards ------------------------------
struct Reward { int placeMoney = 0, coinMoney = 0, takedownMoney = 0, total = 0, medal = 0; bool success = false; };
static Reward lastReward;
static bool rewardGiven = false;
static bool bossBeaten = false;
static void computeRewards(int myPos) {
    if (rewardGiven) return;
    rewardGiven = true;
    // with two players the better-placed one counts for the result
    int hp = myPos; const Racer* bestH = &racers[0];
    for (const Racer& r : racers) if (r.isPlayer) { int p = placeOf(r.id); if (p < hp) { hp = p; bestH = &r; } }
    myPos = hp;
    Racer& pl = racers[bestH->id];
    Reward rw;
    bool raceLike = S.mode == GM_RACE || S.mode == GM_ELIM || S.mode == GM_BATTLE;
    int boss = curBoss();
    bossBeaten = false;
    if (boss) {
        for (const Racer& r : racers) if (r.boss && pl.finished && raceScore(pl) > raceScore(r)) bossBeaten = true;
        rw.success = bossBeaten; rw.medal = bossBeaten ? std::min(3, myPos) : 0;
    } else if (S.mode == GM_BATTLE) { rw.success = careerEvent >= 0 ? myPos == 1 : myPos <= 3; rw.medal = rw.success ? myPos : 0; }
    else if (raceLike) { rw.success = myPos <= 3 && resultState != 2; rw.medal = rw.success ? myPos : 0; }
    else if (S.mode == GM_TIMETRIAL) { rw.success = pl.finished && pl.finishTime <= ttTarget; rw.medal = rw.success ? 1 : 0; }
    else { rw.success = resultState == 1; rw.medal = rw.success ? 1 : 0; }
    int base = careerEvent >= 0 ? CAREER[careerEvent].reward : 0;
    if (careerEvent >= 0) {
        float k = raceLike ? (myPos == 1 ? 1.0f : myPos == 2 ? 0.7f : myPos == 3 ? 0.5f : 0.0f) : (rw.success ? 1.0f : 0.0f);
        if (resultState == 2 && raceLike && S.mode != GM_BATTLE) k = 0;
        if (boss) k = bossBeaten ? 1.0f : 0.15f;
        if (S.mode == GM_BATTLE && !rw.success) k = std::min(k, 0.3f);
        rw.placeMoney = (int)(base * k);
        if (rw.success) {
            int prev = P.career[careerEvent];
            int m = rw.medal; if (prev == 0 || m < prev) P.career[careerEvent] = m;
            if (careerEvent == 13 || careerEvent == 15) P.backrooms = 1;
        }
        if (bossBeaten && !(P.bossMask & (1 << (boss - 1)))) {
            P.bossMask |= 1 << (boss - 1);
            const char* rewardTxt[4] = {T("\320\235\320\220\320\223\320\240\320\220\320\224\320\220: \320\227\320\236\320\233\320\236\320\242\320\220\320\257 \320\232\320\240\320\220\320\241\320\232\320\220", "REWARD: GOLD PAINT"), T("\320\235\320\220\320\223\320\240\320\220\320\224\320\220: \320\233\320\225\320\224\320\257\320\235\320\236\320\231 \320\245\320\240\320\236\320\234", "REWARD: ICE CHROME PAINT"),
                                        T("\320\235\320\220\320\223\320\240\320\220\320\224\320\220: \320\235\320\220\320\232\320\233\320\225\320\231\320\232\320\220 \302\253\320\234\320\220\320\223\320\234\320\220\302\273", "REWARD: MAGMA DECAL"), T("\320\235\320\220\320\223\320\240\320\220\320\224\320\220: \320\232\320\236\320\240\320\236\320\235\320\220", "REWARD: CROWN")};
            notify(rewardTxt[boss - 1], C(255, 210, 60), 6.0f);
        }
    } else {
        if (raceLike) rw.placeMoney = resultState == 2 ? 50 : (myPos == 1 ? 500 : myPos == 2 ? 300 : myPos == 3 ? 200 : 80);
        else rw.placeMoney = rw.success ? 500 : 50;
    }
    int coins = 0, tds = 0;
    for (const Racer& r : racers) if (r.isPlayer) { coins += r.coinsTotal; tds += r.takedowns; }
    rw.coinMoney = coins * 5; rw.takedownMoney = tds * 100;
    rw.total = rw.placeMoney + rw.coinMoney + rw.takedownMoney;
    P.money += rw.total; P.earned += rw.total; P.totalTakedowns += tds; P.totalCoins += coins;
    for (const Racer& r : racers) if (r.isPlayer) { P.tricks += r.tricks; P.rings += r.rings; P.ultras += r.ultraTurbos; P.drafts += r.drafts; }
    if (myPos == 1 && (S.mode == GM_RACE || S.mode == GM_ELIM || S.mode == GM_BATTLE) && resultState != 2) P.wins++;
    if (S.mode == GM_BATTLE && myPos == 1) P.battles++;
    if (S.mode == GM_PURSUIT && resultState == 1) P.escapes++;
    if (numHumans > 1) P.races2p++;
    if (S.mode == GM_RACE && racers[0].finished && (P.bestRace[curTrack] <= 0 || racers[0].finishTime < P.bestRace[curTrack])) P.bestRace[curTrack] = racers[0].finishTime;
    lastReward = rw;
    metaAfterRace(myPos);
    saveProfile();
}

// =====================================================================
//              ACHIEVEMENTS + DAILY CHALLENGE
// =====================================================================
enum AchId { A_FIRST_WIN = 0, A_HATTRICK, A_TD10, A_TD50, A_COINS100, A_RICH, A_ULTRA, A_ACRO, A_PILOT, A_ESCAPE, A_LASTHERO, A_ARENA,
             A_BOSS1, A_BOSSALL, A_ICE, A_FIREPROOF, A_COSMO, A_WALL, A_DRAFT, A_TEAM, A_CHAMP, A_TUNED, A_STYLE, A_DAILY5, A_COUNT };
struct AchDef { const char* ru; const char* en; const char* dru; const char* den; int reward; };
static const AchDef ACH[A_COUNT] = {
    {"\320\237\320\265\321\200\320\262\320\260\321\217 \320\277\320\276\320\261\320\265\320\264\320\260", "First Win", "\320\222\321\213\320\270\320\263\321\200\320\260\320\271\321\202\320\265 \320\273\321\216\320\261\321\203\321\216 \320\263\320\276\320\275\320\272\321\203", "Win any race", 300},
    {"\320\245\320\265\321\202-\321\202\321\200\320\270\320\272", "Hat Trick", "\320\236\320\264\320\265\321\200\320\266\320\270\321\202\320\265 3 \320\277\320\276\320\261\320\265\320\264\321\213", "Win 3 times", 600},
    {"\320\242\320\260\321\200\320\260\320\275-\320\274\320\260\321\201\321\202\320\265\321\200", "Rammer", "\320\222\321\213\320\261\320\265\320\271\321\202\320\265 10 \321\201\320\276\320\277\320\265\321\200\320\275\320\270\320\272\320\276\320\262", "Take down 10 rivals", 500},
    {"\320\240\320\260\320\267\321\200\321\203\321\210\320\270\321\202\320\265\320\273\321\214", "Wrecking Ball", "\320\222\321\213\320\261\320\265\320\271\321\202\320\265 50 \321\201\320\276\320\277\320\265\321\200\320\275\320\270\320\272\320\276\320\262", "Take down 50 rivals", 1500},
    {"\320\232\320\276\320\277\320\270\320\273\320\272\320\260", "Piggy Bank", "\320\241\320\276\320\261\320\265\321\200\320\270\321\202\320\265 100 \320\274\320\276\320\275\320\265\321\202", "Collect 100 coins", 500},
    {"\320\221\320\276\320\263\320\260\321\207", "High Roller", "\320\227\320\260\321\200\320\260\320\261\320\276\321\202\320\260\320\271\321\202\320\265 $20 000", "Earn $20,000", 2000},
    {"\320\241\321\203\320\277\320\265\321\200-\321\202\321\203\321\200\320\261\320\276", "Ultra Turbo", "\320\241\320\264\320\265\320\273\320\260\320\271\321\202\320\265 \321\204\320\270\320\276\320\273\320\265\321\202\320\276\320\262\320\276\320\265 \320\274\320\270\320\275\320\270-\321\202\321\203\321\200\320\261\320\276", "Land a purple mini-turbo", 300},
    {"\320\220\320\272\321\200\320\276\320\261\320\260\321\202", "Acrobat", "\320\241\320\264\320\265\320\273\320\260\320\271\321\202\320\265 10 \321\202\321\200\321\216\320\272\320\276\320\262 \320\275\320\260 \321\202\321\200\320\260\320\274\320\277\320\273\320\270\320\275\320\260\321\205", "Do 10 ramp tricks", 600},
    {"\320\237\320\270\320\273\320\276\321\202", "Ace Pilot", "\320\237\321\200\320\276\320\273\320\265\321\202\320\270\321\202\320\265 \321\207\320\265\321\200\320\265\320\267 30 \320\272\320\276\320\273\320\265\321\206", "Fly through 30 rings", 600},
    {"\320\235\320\265\321\203\320\273\320\276\320\262\320\270\320\274\321\213\320\271", "Untouchable", "\320\243\320\271\320\264\320\270\321\202\320\265 \320\276\321\202 \320\277\320\276\320\273\320\270\321\206\320\270\320\270", "Escape a pursuit", 700},
    {"\320\237\320\276\321\201\320\273\320\265\320\264\320\275\320\270\320\271 \320\263\320\265\321\200\320\276\320\271", "Last One Standing", "\320\237\320\276\320\261\320\265\320\264\320\270\321\202\320\265 \320\262 \321\200\320\265\320\266\320\270\320\274\320\265 \320\275\320\260 \320\262\321\213\320\261\321\213\320\262\320\260\320\275\320\270\320\265", "Win an elimination", 700},
    {"\320\232\320\276\321\200\320\276\320\273\321\214 \320\260\321\200\320\265\320\275\321\213", "Arena King", "\320\237\320\276\320\261\320\265\320\264\320\270\321\202\320\265 \320\262 \320\261\320\270\321\202\320\262\320\265", "Win a battle", 700},
    {"\320\241\320\276\320\272\321\200\321\203\321\210\320\270\321\202\320\265\320\273\321\214 \320\261\320\276\321\201\321\201\320\276\320\262", "Boss Slayer", "\320\236\320\261\320\263\320\276\320\275\320\270\321\202\320\265 \320\273\321\216\320\261\320\276\320\263\320\276 \320\261\320\276\321\201\321\201\320\260", "Beat any boss", 1000},
    {"\320\233\320\265\320\263\320\265\320\275\320\264\320\260", "Legend", "\320\236\320\261\320\263\320\276\320\275\320\270\321\202\320\265 \320\262\321\201\320\265\321\205 \321\207\320\265\321\202\321\213\321\200\321\221\321\205 \320\261\320\276\321\201\321\201\320\276\320\262", "Beat all four bosses", 4000},
    {"\320\233\320\265\320\264\320\276\320\272\320\276\320\273", "Icebreaker", "\320\244\320\270\320\275\320\270\321\210\320\270\321\200\321\203\320\271\321\202\320\265 \320\275\320\260 \320\233\320\265\320\264\321\217\320\275\320\276\320\274 \320\277\320\265\321\200\320\265\320\262\320\260\320\273\320\265", "Finish a race on Frost Pass", 400},
    {"\320\236\320\263\320\275\320\265\321\203\320\277\320\276\321\200\320\275\321\213\320\271", "Fireproof", "\320\244\320\270\320\275\320\270\321\210 \320\275\320\260 \320\222\321\203\320\273\320\272\320\260\320\275\320\265 \320\261\320\265\320\267 \320\260\320\262\320\260\321\200\320\270\320\271", "Finish on Volcano without a wreck", 900},
    {"\320\232\320\276\321\201\320\274\320\276\320\275\320\260\320\262\321\202", "Cosmonaut", "\320\244\320\270\320\275\320\270\321\210\320\270\321\200\321\203\320\271\321\202\320\265 \320\275\320\260 \320\236\321\200\320\261\320\270\321\202\320\265", "Finish a race on Orbit Station", 600},
    {"\320\241\320\272\320\262\320\276\320\267\321\214 \321\201\321\202\320\265\320\275\321\203", "Through the Wall", "\320\237\321\200\320\276\320\273\320\276\320\274\320\270\321\202\320\265 \321\201\321\200\320\265\320\267\320\272\321\203 \321\202\320\260\320\275\320\272\320\276\320\274", "Smash a shortcut open with a tank", 500},
    {"\320\237\320\276\320\277\321\203\321\202\320\275\321\213\320\271 \320\262\320\265\321\202\320\265\321\200", "Tailwind", "\320\237\320\276\320\271\320\274\320\260\320\271\321\202\320\265 \321\201\320\273\320\270\320\277\321\201\321\202\321\200\320\270\320\274 20 \321\200\320\260\320\267", "Catch a slipstream 20 times", 600},
    {"\320\232\320\276\320\274\320\260\320\275\320\264\320\275\320\260\321\217 \320\270\320\263\321\200\320\260", "Co-op", "\320\241\321\213\320\263\321\200\320\260\320\271\321\202\320\265 \320\263\320\276\320\275\320\272\321\203 \320\262\320\264\320\262\320\276\321\221\320\274", "Play a 2-player race", 400},
    {"\320\247\320\265\320\274\320\277\320\270\320\276\320\275", "Champion", "\320\237\321\200\320\276\320\271\320\264\320\270\321\202\320\265 \320\262\321\201\321\216 \320\272\320\260\321\200\321\214\320\265\321\200\321\203", "Clear the whole career", 5000},
    {"\320\237\320\276\320\273\320\275\321\213\320\271 \321\202\321\216\320\275\320\270\320\275\320\263", "Fully Tuned", "\320\237\321\200\320\276\320\272\320\260\321\207\320\260\320\271\321\202\320\265 \320\273\321\216\320\261\320\276\320\271 \321\203\320\267\320\265\320\273 \320\264\320\276 \320\274\320\260\320\272\321\201\320\270\320\274\321\203\320\274\320\260", "Max out any upgrade", 800},
    {"\320\241\321\202\320\270\320\273\320\270\321\201\321\202", "Stylist", "\320\235\320\260\320\272\320\273\320\265\320\271\321\202\320\265 \320\275\320\260\320\272\320\273\320\265\320\271\320\272\321\203 \320\262 \320\263\320\260\321\200\320\260\320\266\320\265", "Apply a decal in the garage", 200},
    {"\320\232\320\260\320\266\320\264\321\213\320\271 \320\264\320\265\320\275\321\214", "Daily Driver", "\320\222\321\213\320\277\320\276\320\273\320\275\320\270\321\202\320\265 5 \320\267\320\260\320\264\320\260\320\275\320\270\320\271 \320\264\320\275\321\217", "Complete 5 daily challenges", 1500}};
static bool achDone(int id) { return (P.ach >> id) & 1ull; }
static void achProgress(int id, int& cur, int& target) {
    target = 1; cur = achDone(id) ? 1 : 0;
    int done = 0; for (int i = 0; i < CAREER_N; i++) if (P.career[i]) done++;
    int maxUp = 0; for (int u = 0; u < 4; u++) maxUp = std::max(maxUp, P.upg[u]);
    int bosses = 0; for (int b = 0; b < 4; b++) if (P.bossMask & (1 << b)) bosses++;
    switch (id) {
        case A_FIRST_WIN: cur = P.wins; break;
        case A_HATTRICK: cur = P.wins; target = 3; break;
        case A_TD10: cur = P.totalTakedowns; target = 10; break;
        case A_TD50: cur = P.totalTakedowns; target = 50; break;
        case A_COINS100: cur = P.totalCoins; target = 100; break;
        case A_RICH: cur = P.earned; target = 20000; break;
        case A_ULTRA: cur = P.ultras; break;
        case A_ACRO: cur = P.tricks; target = 10; break;
        case A_PILOT: cur = P.rings; target = 30; break;
        case A_ESCAPE: cur = P.escapes; break;
        case A_ARENA: cur = P.battles; break;
        case A_BOSS1: cur = bosses; break;
        case A_BOSSALL: cur = bosses; target = 4; break;
        case A_WALL: cur = P.walls; break;
        case A_DRAFT: cur = P.drafts; target = 20; break;
        case A_TEAM: cur = P.races2p; break;
        case A_CHAMP: cur = done; target = CAREER_N; break;
        case A_TUNED: cur = maxUp; target = 5; break;
        case A_DAILY5: cur = P.dailyDone; target = 5; break;
        default: break;   // flag achievements: set directly
    }
    if (achDone(id)) cur = target;
    cur = std::min(cur, target);
}
static void unlockAch(int id) {
    if (achDone(id)) return;
    P.ach |= 1ull << id;
    P.money += ACH[id].reward;
    notify(TextFormat("%s %s  +$%d", T("\320\224\320\236\320\241\320\242\320\230\320\226\320\225\320\235\320\230\320\225:", "ACHIEVEMENT:"), T(ACH[id].ru, ACH[id].en), ACH[id].reward), C(255, 215, 70), 4.5f);
    play(sCash, 0.8f, 1.2f);
}
static void checkAchievements() {
    for (int id = 0; id < A_COUNT; id++) {
        if (achDone(id)) continue;
        int c, t; achProgress(id, c, t);
        bool flagOnly = id == A_LASTHERO || id == A_ICE || id == A_FIREPROOF || id == A_COSMO || id == A_STYLE;
        if (!flagOnly && c >= t) unlockAch(id);
    }
}

// ------------------------------ daily challenge ------------------------------
struct Daily { int type, track, base, n, reward; };
static bool dailyActive = false;
static int todayId() {
    time_t t = time(NULL); struct tm* lt = localtime(&t);
    return lt ? (lt->tm_year * 400 + lt->tm_yday) : (int)(t / 86400);
}
static Daily makeDaily(int day) {
    Daily d;
    float h0 = hash1(day * 7 + 11), h1 = hash1(day * 13 + 5), h2 = hash1(day * 29 + 3), h3 = hash1(day * 41 + 17);
    d.type = (int)(h0 * 8) % 8;
    const int tracks[5] = {0, 1, 3, 4, 5};
    d.track = tracks[(int)(h1 * 5) % 5];
    d.base = (int)(h2 * 4) % 4;
    if (d.track == 5 && d.base != B_CAR && d.base != B_KART) d.base = B_KART;
    if (!P.owned[d.base]) d.base = (d.base == B_TANK || d.base == B_ROBOT) ? B_CAR : d.base;
    if (d.type == 6 && d.track == 5) d.track = 0;      // no police in orbit
    d.n = 0;
    switch (d.type) {
        case 2: d.n = 12 + (int)(h3 * 8); break;
        case 3: d.n = 2 + (int)(h3 * 2); break;
        case 4: d.n = 3 + (int)(h3 * 3); break;
        default: break;
    }
    d.reward = 1200 + d.type * 100 + (int)(h3 * 4) * 100;
    return d;
}
static std::string dailyText(const Daily& d) {
    switch (d.type) {
        case 0: return TextFormat(T("\320\222\321\213\320\270\320\263\321\200\320\260\320\271\321\202\320\265 \320\263\320\276\320\275\320\272\321\203 \320\275\320\260 \321\202\321\200\320\260\321\201\321\201\320\265 \302\253%s\302\273", "Win a race on %s"), trackName(d.track));
        case 1: return TextFormat(T("\320\244\320\270\320\275\320\270\321\210 \320\262 \321\202\321\200\320\276\320\271\320\272\320\265 \320\275\320\260 \302\253%s\302\273 (%s)", "Top 3 on %s driving the %s"), trackName(d.track), baseName((Base)d.base));
        case 2: return TextFormat(T("\320\241\320\276\320\261\320\265\321\200\320\270\321\202\320\265 %d \320\274\320\276\320\275\320\265\321\202 \320\267\320\260 \320\276\320\264\320\275\321\203 \320\263\320\276\320\275\320\272\321\203", "Collect %d coins in one race"), d.n);
        case 3: return TextFormat(T("\320\222\321\213\320\261\320\265\320\271\321\202\320\265 %d \321\201\320\276\320\277\320\265\321\200\320\275\320\270\320\272\320\276\320\262 \320\267\320\260 \320\263\320\276\320\275\320\272\321\203", "Take down %d rivals in one race"), d.n);
        case 4: return TextFormat(T("\320\241\320\264\320\265\320\273\320\260\320\271\321\202\320\265 %d \321\201\321\203\320\277\320\265\321\200-\321\202\321\203\321\200\320\261\320\276 \320\267\320\260 \320\263\320\276\320\275\320\272\321\203", "Land %d ultra mini-turbos in one race"), d.n);
        case 5: return TextFormat(T("\320\237\320\276\320\261\320\265\320\264\320\270\321\202\320\265 \320\262 \320\261\320\270\321\202\320\262\320\265 \320\275\320\260 \302\253%s\302\273", "Win a battle on %s"), trackName(d.track));
        case 6: return TextFormat(T("\320\243\320\271\320\264\320\270\321\202\320\265 \320\276\321\202 \320\277\320\276\320\263\320\276\320\275\320\270 \320\275\320\260 \302\253%s\302\273", "Escape the police on %s"), trackName(d.track));
        default: return TextFormat(T("\320\237\320\276\320\261\320\265\320\264\320\270\321\202\320\265 \320\275\320\260 \320\262\321\213\320\261\321\213\320\262\320\260\320\275\320\270\320\265 \320\275\320\260 \302\253%s\302\273", "Win an elimination on %s"), trackName(d.track));
    }
}
// configure the quick race for today's challenge
static void setupDaily(const Daily& d) {
    S.track = d.track; S.laps = 2; S.opponents = 5; S.items = 1; S.traffic = 1; S.players = 1;
    S.mode = d.type == 5 ? GM_BATTLE : d.type == 6 ? GM_PURSUIT : d.type == 7 ? GM_ELIM : GM_RACE;
    if (d.type == 1) S.base = (Base)d.base;
}
static void metaAfterRace(int myPos) {
    const Racer& pl = racers[0];
    bool finishedRace = S.mode == GM_RACE && pl.finished;
    if (S.mode == GM_ELIM && myPos == 1 && resultState == 1) unlockAch(A_LASTHERO);
    if (finishedRace && curTrack == 3) unlockAch(A_ICE);
    if (finishedRace && curTrack == 4 && pl.deaths == 0) unlockAch(A_FIREPROOF);
    if (finishedRace && curTrack == 5) unlockAch(A_COSMO);
    if (dailyActive) {
        int day = todayId();
        Daily d = makeDaily(day);
        bool ok = false;
        switch (d.type) {
            case 0: ok = S.mode == GM_RACE && curTrack == d.track && myPos == 1 && pl.finished; break;
            case 1: ok = S.mode == GM_RACE && curTrack == d.track && myPos <= 3 && pl.finished && pl.base == d.base; break;
            case 2: ok = pl.coinsTotal >= d.n; break;
            case 3: ok = pl.takedowns >= d.n; break;
            case 4: ok = pl.ultraTurbos >= d.n; break;
            case 5: ok = S.mode == GM_BATTLE && myPos == 1; break;
            case 6: ok = S.mode == GM_PURSUIT && resultState == 1; break;
            default: ok = S.mode == GM_ELIM && myPos == 1 && resultState == 1; break;
        }
        if (ok && P.dailyDay != day) {
            P.dailyDay = day; P.dailyDone++; P.money += d.reward; P.earned += d.reward;
            notify(TextFormat("%s +$%d", T("\320\227\320\220\320\224\320\220\320\235\320\230\320\225 \320\224\320\235\320\257 \320\222\320\253\320\237\320\236\320\233\320\235\320\225\320\235\320\236!", "DAILY CHALLENGE DONE!"), d.reward), C(120, 255, 160), 5.0f);
            play(sCash, 0.9f, 0.9f);
        }
    }
    checkAchievements();
}

// =====================================================================
//                     RENDER PRIMITIVES
// =====================================================================
static V3 gLightL = {0, 1, 0};
static float gFog = 0, drawR = 900;
static int gAlpha = 255;            // < 255 for the ghost car

static inline V3 rotY(V3 v, float a) { float c = cosf(a), s = sinf(a); return vec(v.x * c + v.z * s, v.y, -v.x * s + v.z * c); }
// world -> screen for the current viewport (works for split screen and scaled render targets)
static Vector2 w2s(V3 p, const Camera3D& cam) {
    Vector2 v = GetWorldToScreenEx(p, cam, vpW, vpH);
    return v2(v.x / hudScale, v.y / hudScale);
}
static void setObj(V3 worldPos, float yaw) { gLightL = rotY(lightDir, -yaw); gFog = fogF(len(sub(worldPos, camPos))); }
static Color litC(Color c, V3 n) {
    float d = std::max(0.0f, dot(n, gLightL));
    float up = 0.5f + 0.5f * n.y;
    float r = c.r * (ambCol.r / 255.0f * (0.75f + 0.35f * up) + sunCol.r / 255.0f * d * 0.75f);
    float g = c.g * (ambCol.g / 255.0f * (0.75f + 0.35f * up) + sunCol.g / 255.0f * d * 0.75f);
    float b = c.b * (ambCol.b / 255.0f * (0.75f + 0.35f * up) + sunCol.b / 255.0f * d * 0.75f);
    Color o = C((int)r, (int)g, (int)b, c.a);
    o = mixC(o, fogCol, gFog); o.a = (unsigned char)(c.a * gAlpha / 255);
    return o;
}
static Color emisC(Color c) { Color o = mixC(c, fogCol, gFog * 0.85f); o.a = (unsigned char)(c.a * gAlpha / 255); return o; }
static inline void vtx(V3 p) { rlVertex3f(p.x, p.y, p.z); }
static void hexa(const V3* p, Color c, bool emissive = false) {
    static const int F[6][4] = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
    V3 ctr = vec(0, 0, 0); for (int i = 0; i < 8; i++) ctr = add(ctr, p[i]); ctr = mul(ctr, 0.125f);
    rlCheckRenderBatchLimit(36);
    rlBegin(RL_TRIANGLES);
    for (int f = 0; f < 6; f++) {
        V3 a = p[F[f][0]], b = p[F[f][1]], cc = p[F[f][2]], d = p[F[f][3]];
        V3 n = norm(cross(sub(b, a), sub(cc, a)));
        if (len(cross(sub(b, a), sub(cc, a))) < 1e-6f) n = norm(cross(sub(cc, a), sub(d, a)));
        V3 fc = mul(add(add(a, b), add(cc, d)), 0.25f);
        if (dot(n, sub(fc, ctr)) < 0) n = mul(n, -1.0f);
        Color k = emissive ? emisC(c) : litC(c, n);
        rlColor4ub(k.r, k.g, k.b, k.a);
        vtx(a); vtx(b); vtx(cc); vtx(a); vtx(cc); vtx(d);
    }
    rlEnd();
}
static void box(float x, float y, float z, float w, float h, float l, Color c, bool em = false) {
    float hx = w / 2, hy = h / 2, hz = l / 2;
    V3 p[8] = {vec(x - hx, y - hy, z - hz), vec(x + hx, y - hy, z - hz), vec(x + hx, y - hy, z + hz), vec(x - hx, y - hy, z + hz),
               vec(x - hx, y + hy, z - hz), vec(x + hx, y + hy, z - hz), vec(x + hx, y + hy, z + hz), vec(x - hx, y + hy, z + hz)};
    hexa(p, c, em);
}
static void tbox(float x, float y0, float z, float w0, float l0, float y1, float w1, float l1, float dz, Color c) {
    V3 p[8] = {vec(x - w0 / 2, y0, z - l0 / 2), vec(x + w0 / 2, y0, z - l0 / 2), vec(x + w0 / 2, y0, z + l0 / 2), vec(x - w0 / 2, y0, z + l0 / 2),
               vec(x - w1 / 2, y1, z + dz - l1 / 2), vec(x + w1 / 2, y1, z + dz - l1 / 2), vec(x + w1 / 2, y1, z + dz + l1 / 2), vec(x - w1 / 2, y1, z + dz + l1 / 2)};
    hexa(p, c);
}
static void cone(V3 b, float r, float h, int sl, Color c, Color c2 = BLANK) {
    rlCheckRenderBatchLimit(sl * 3);
    rlBegin(RL_TRIANGLES);
    V3 top = add(b, vec(0, h, 0));
    for (int i = 0; i < sl; i++) {
        float a0 = 2 * PI_F * i / sl, a1 = 2 * PI_F * (i + 1) / sl, am = (a0 + a1) / 2;
        V3 n = norm(vec(cosf(am) * fabsf(h), r, sinf(am) * fabsf(h)));
        Color k = litC((c2.a > 0 && (i & 1)) ? c2 : c, n); rlColor4ub(k.r, k.g, k.b, k.a);
        vtx(add(b, vec(cosf(a0) * r, 0, sinf(a0) * r))); vtx(top); vtx(add(b, vec(cosf(a1) * r, 0, sinf(a1) * r)));
    }
    rlEnd();
}
static void ball(V3 c, float r, Color col, Color col2 = BLANK, int rings = 5, int segs = 8) {
    rlCheckRenderBatchLimit(rings * segs * 6);
    rlBegin(RL_TRIANGLES);
    for (int i = 0; i < rings; i++) {
        float t0 = PI_F * i / rings - PI_F / 2, t1 = PI_F * (i + 1) / rings - PI_F / 2;
        for (int j = 0; j < segs; j++) {
            float p0 = 2 * PI_F * j / segs, p1 = 2 * PI_F * (j + 1) / segs;
            V3 a = vec(cosf(t0) * cosf(p0), sinf(t0), cosf(t0) * sinf(p0)), b = vec(cosf(t0) * cosf(p1), sinf(t0), cosf(t0) * sinf(p1));
            V3 d = vec(cosf(t1) * cosf(p0), sinf(t1), cosf(t1) * sinf(p0)), e = vec(cosf(t1) * cosf(p1), sinf(t1), cosf(t1) * sinf(p1));
            V3 n = norm(add(add(a, b), add(d, e)));
            Color k = litC((col2.a > 0 && (j & 1)) ? col2 : col, n); rlColor4ub(k.r, k.g, k.b, k.a);
            V3 A = add(c, mul(a, r)), B = add(c, mul(b, r)), D = add(c, mul(d, r)), E = add(c, mul(e, r));
            vtx(A); vtx(B); vtx(E); vtx(A); vtx(E); vtx(D);
        }
    }
    rlEnd();
}
static void blob(V3 c, float rx, float ry, int sl, Color col, float seed) {
    rlCheckRenderBatchLimit(sl * 6);
    rlBegin(RL_TRIANGLES);
    V3 top = add(c, vec(0, ry, 0)), bot = add(c, vec(0, -ry * 0.7f, 0));
    for (int i = 0; i < sl; i++) {
        float a0 = 2 * PI_F * i / sl + seed, a1 = 2 * PI_F * (i + 1) / sl + seed;
        float r0 = rx * (0.85f + 0.3f * hash1((int)(seed * 100) + i)), r1 = rx * (0.85f + 0.3f * hash1((int)(seed * 100) + (i + 1) % sl));
        V3 p0 = add(c, vec(cosf(a0) * r0, 0, sinf(a0) * r0)), p1 = add(c, vec(cosf(a1) * r1, 0, sinf(a1) * r1));
        V3 n1 = norm(cross(sub(p1, top), sub(p0, top))); if (n1.y < 0) n1 = mul(n1, -1);
        Color k = litC(col, n1); rlColor4ub(k.r, k.g, k.b, k.a); vtx(p0); vtx(top); vtx(p1);
        V3 n2 = norm(cross(sub(p0, bot), sub(p1, bot))); if (n2.y > 0) n2 = mul(n2, -1);
        k = litC(col, n2); rlColor4ub(k.r, k.g, k.b, k.a); vtx(p0); vtx(p1); vtx(bot);
    }
    rlEnd();
}
static void tri2(V3 a, V3 b, V3 c, Color col) {   // lit two-sided triangle
    V3 n = norm(cross(sub(b, a), sub(c, a))); if (n.y < 0) n = mul(n, -1);
    Color k = litC(col, n);
    rlCheckRenderBatchLimit(3); rlBegin(RL_TRIANGLES); rlColor4ub(k.r, k.g, k.b, k.a); vtx(a); vtx(b); vtx(c); rlEnd();
}
static void texQuad(const Texture2D& t, V3 a, V3 b, V3 c, V3 d, Vector2 ta, Vector2 tb, Vector2 tc, Vector2 td, Color ca, Color cb, Color cc, Color cd) {
    rlCheckRenderBatchLimit(4);
    rlSetTexture(t.id);
    rlBegin(RL_QUADS);
    rlColor4ub(ca.r, ca.g, ca.b, ca.a); rlTexCoord2f(ta.x, ta.y); vtx(a);
    rlColor4ub(cb.r, cb.g, cb.b, cb.a); rlTexCoord2f(tb.x, tb.y); vtx(b);
    rlColor4ub(cc.r, cc.g, cc.b, cc.a); rlTexCoord2f(tc.x, tc.y); vtx(c);
    rlColor4ub(cd.r, cd.g, cd.b, cd.a); rlTexCoord2f(td.x, td.y); vtx(d);
    rlEnd();
    rlSetTexture(0);
}
static void texQuad1(const Texture2D& t, V3 a, V3 b, V3 c, V3 d, Vector2 ta, Vector2 tc, Color col) {
    texQuad(t, a, b, c, d, ta, v2(tc.x, ta.y), tc, v2(ta.x, tc.y), col, col, col, col);
}
static void groundGlow(V3 c, V3 axisA, V3 axisB, Color col) {
    V3 a = sub(sub(c, axisA), axisB), b = sub(add(c, axisA), axisB), cc = add(add(c, axisA), axisB), d = add(sub(c, axisA), axisB);
    texQuad1(texGlow, a, b, cc, d, v2(0, 0), v2(1, 1), col);
}
static void glow(Camera3D& cam, V3 p, float size, Color c) {
    float d = len(sub(p, camPos));
    if (d > drawR * 1.2f) return;
    if (dot(sub(p, camPos), camFwd) < -size) return;
    Color k = c; float f = fogF(d) * 0.75f;
    k.r = (unsigned char)(k.r * (1 - f)); k.g = (unsigned char)(k.g * (1 - f)); k.b = (unsigned char)(k.b * (1 - f));
    DrawBillboard(cam, texGlow, p, size, k);
}
static bool visible(V3 p, float radius, float maxD) {
    V3 d = sub(p, camPos); float dl = len(d);
    if (dl > maxD + radius) return false;
    if (dl > radius + 15 && dot(d, camFwd) < -radius) return false;
    return true;
}
// textured box in local space (base at y0), lit per face
static void texBoxLocal(const Texture2D& t, float hx, float hz, float y0, float y1, Color tint, float uW, float vH, float bright, bool roof) {
    struct F { V3 a, b, c, d, n; float w; };
    F f[4] = {{vec(hx, y0, -hz), vec(hx, y0, hz), vec(hx, y1, hz), vec(hx, y1, -hz), vec(1, 0, 0), 2 * hz},
              {vec(-hx, y0, hz), vec(-hx, y0, -hz), vec(-hx, y1, -hz), vec(-hx, y1, hz), vec(-1, 0, 0), 2 * hz},
              {vec(hx, y0, hz), vec(-hx, y0, hz), vec(-hx, y1, hz), vec(hx, y1, hz), vec(0, 0, 1), 2 * hx},
              {vec(-hx, y0, -hz), vec(hx, y0, -hz), vec(hx, y1, -hz), vec(-hx, y1, -hz), vec(0, 0, -1), 2 * hx}};
    float vh = (y1 - y0) / vH;
    for (int k = 0; k < 4; k++) {
        Color c = litC(tint, f[k].n);
        if (bright > 0) c = mixC(c, emisC(tint), bright);
        float uw = uW > 0 ? f[k].w / uW : 1.0f;
        texQuad(t, f[k].a, f[k].b, f[k].c, f[k].d, v2(0, vh), v2(uw, vh), v2(uw, 0), v2(0, 0), c, c, c, c);
    }
    if (roof) {
        Color rc = litC(C(70, 72, 78), vec(0, 1, 0));
        texQuad1(texWhite, vec(-hx, y1, -hz), vec(hx, y1, -hz), vec(hx, y1, hz), vec(-hx, y1, hz), v2(0, 0), v2(1, 1), rc);
    }
}

// =====================================================================
//                              SKY (2D)
// =====================================================================
struct Star { float az, el, b; };
static std::vector<Star> stars;
static std::vector<float> skyline;
static void initSky() {
    for (int i = 0; i < 260; i++) { Star s; s.az = frand(0, 2 * PI_F); s.el = asinf(frand(0.05f, 1.0f)); s.b = frand(0.3f, 1.0f); stars.push_back(s); }
    for (int i = 0; i < 180; i++) skyline.push_back(hash1(i * 17 + 3) < 0.15f ? frand(160, 320) : frand(40, 150));
}
static bool proj(Camera3D& cam, V3 dir, Vector2& out) {
    if (dot(dir, camFwd) < 0.15f) return false;
    out = w2s(add(camPos, mul(dir, 1000.0f)), cam);
    return out.x > -200 && out.x < SW + 200 && out.y > -200 && out.y < SH + 200;
}
static bool projAny(Camera3D& cam, V3 dir, Vector2& out) {   // like proj() but for directions below the horizon too
    if (dot(dir, camFwd) < 0.05f) return false;
    out = w2s(add(camPos, mul(dir, 1000.0f)), cam);
    return out.x > -600 && out.x < SW + 600 && out.y > -600 && out.y < SH + 600;
}
static void drawSpaceSky(Camera3D& cam) {
    DrawRectangleGradientV(0, 0, SW, SH, C(3, 4, 14), C(10, 8, 28));
    // nebula clouds
    const Color neb[3] = {C(120, 40, 160), C(30, 90, 170), C(170, 50, 90)};
    for (int i = 0; i < 18; i++) {
        float az = hash1(i * 31 + 7) * 2 * PI_F, el = (hash1(i * 17 + 3) - 0.35f) * 1.2f;
        V3 d = vec(cosf(el) * sinf(az), sinf(el), cosf(el) * cosf(az));
        Vector2 p; if (!projAny(cam, d, p)) continue;
        float w = 260 + 420 * hash1(i * 5);
        DrawTexturePro(texGlow, Rectangle{0, 0, 64, 64}, Rectangle{p.x - w / 2, p.y - w * 0.3f, w, w * 0.6f}, v2(0, 0), 0, alpha(neb[i % 3], 0.22f));
    }
    for (const Star& s : stars) for (int hemi = 0; hemi < 2; hemi++) {
        V3 d = vec(cosf(s.el) * sinf(s.az + hemi * 1.3f), sinf(s.el) * (hemi ? -1.0f : 1.0f), cosf(s.el) * cosf(s.az + hemi * 1.3f));
        Vector2 p; if (!projAny(cam, d, p)) continue;
        float tw = 0.75f + 0.25f * sinf(gTime * 2 + s.az * 40);
        DrawRectangle((int)p.x, (int)p.y, s.b > 0.85f ? 2 : 1, s.b > 0.85f ? 2 : 1, alpha(s.b > 0.95f ? C(200, 220, 255) : WHITE, s.b * tw));
    }
    // big ringed planet below the station and a distant moon
    {
        V3 pd = norm(vec(0.55f, -0.45f, 0.7f)); Vector2 p;
        if (projAny(cam, pd, p)) {
            Vector2 e = w2s(add(camPos, mul(norm(add(pd, vec(0.18f, 0, 0))), 1000.0f)), cam);
            float r = fabsf(e.x - p.x) + fabsf(e.y - p.y) * 0.5f + 40;
            DrawTexturePro(texGlow, Rectangle{0, 0, 64, 64}, Rectangle{p.x - r * 1.6f, p.y - r * 1.6f, r * 3.2f, r * 3.2f}, v2(0, 0), 0, alpha(C(80, 150, 255), 0.35f));
            DrawCircleV(p, r, C(40, 90, 150));
            DrawCircleV(v2(p.x - r * 0.18f, p.y - r * 0.18f), r * 0.86f, C(60, 130, 190));
            for (int k = 0; k < 6; k++) DrawRectangle((int)(p.x - r * 0.8f), (int)(p.y - r * 0.6f + k * r * 0.22f), (int)(r * 1.5f), (int)(r * 0.06f), C(90, 160, 210, 120));
            DrawEllipseLines((int)p.x, (int)p.y, r * 1.9f, r * 0.42f, C(210, 200, 170, 200));
            DrawEllipseLines((int)p.x, (int)p.y, r * 1.75f, r * 0.37f, C(190, 180, 150, 150));
            DrawEllipseLines((int)p.x, (int)p.y, r * 2.05f, r * 0.47f, C(170, 160, 130, 110));
        }
        V3 md = norm(vec(-0.7f, 0.35f, -0.3f));
        if (projAny(cam, md, p)) { DrawCircleV(p, 26, C(200, 195, 185)); DrawCircleV(v2(p.x + 8, p.y - 5), 7, C(170, 165, 160)); DrawCircleV(v2(p.x - 9, p.y + 6), 5, C(165, 160, 155)); }
        if (projAny(cam, lightDir, p)) {
            DrawTexturePro(texGlow, Rectangle{0, 0, 64, 64}, Rectangle{p.x - 160, p.y - 160, 320, 320}, v2(0, 0), 0, alpha(C(255, 245, 220), 0.6f));
            DrawCircleV(p, 18, WHITE);
        }
    }
}
static void drawSky(Camera3D& cam) {
    if (indoorTrack()) { DrawRectangle(0, 0, SW, SH, fogCol); return; }
    if (spaceTrack()) { drawSpaceSky(cam); return; }
    V3 hf = norm(vec(camFwd.x, 0, camFwd.z));
    Vector2 h = w2s(add(camPos, mul(hf, 3000.0f)), cam);
    int hy = (int)clampf(h.y, -50.0f, (float)SH + 50);
    if (hy > 0) DrawRectangleGradientV(0, 0, SW, hy, skyTop, skyHor);
    DrawRectangle(0, std::max(0, hy), SW, SH, fogCol);
    float clearK = (curWeather == W_CLEAR ? 1.0f : 1.0f - wI);
    if (nightK > 0.6f && clearK > 0.05f) {
        for (const Star& s : stars) {
            V3 d = vec(cosf(s.el) * sinf(s.az), sinf(s.el), cosf(s.el) * cosf(s.az));
            Vector2 p; if (!proj(cam, d, p)) continue;
            float tw = 0.7f + 0.3f * sinf(gTime * 3 + s.az * 40);
            DrawRectangle((int)p.x, (int)p.y, s.b > 0.85f ? 2 : 1, s.b > 0.85f ? 2 : 1, alpha(WHITE, s.b * tw * clearK));
        }
    }
    {
        Vector2 p;
        if (proj(cam, lightDir, p)) {
            float k = 0.4f + 0.6f * clearK;
            if (S.tod == TD_NIGHT) {
                DrawTexturePro(texGlow, Rectangle{0, 0, 64, 64}, Rectangle{p.x - 90, p.y - 90, 180, 180}, v2(0, 0), 0, alpha(C(170, 190, 255), 0.35f * k));
                DrawCircleV(p, 16, alpha(C(235, 240, 255), k));
                DrawCircleV(v2(p.x + 5, p.y - 3), 4, alpha(C(205, 210, 230), k));
            } else {
                Color sc = S.tod == TD_SUNSET ? C(255, 170, 90) : C(255, 250, 225);
                float sz = S.tod == TD_SUNSET ? 420.0f : 260.0f;
                DrawTexturePro(texGlow, Rectangle{0, 0, 64, 64}, Rectangle{p.x - sz / 2, p.y - sz / 2, sz, sz}, v2(0, 0), 0, alpha(sc, 0.55f * k));
                DrawCircleV(p, S.tod == TD_SUNSET ? 34.0f : 22.0f, alpha(mixC(sc, WHITE, 0.5f), k));
            }
        }
    }
    if (curTrack == 4) {   // the volcano on the horizon: cone, glowing crater and an ash plume
        V3 vd = norm(vec(0.3f, 0, -0.95f));
        V3 b0 = add(vec(camPos.x, 0, camPos.z), mul(vd, 4000.0f));
        if (dot(vd, camFwd) > 0.1f) {
            V3 sd = vec(vd.z, 0, -vd.x);
            Vector2 l = w2s(add(b0, mul(sd, -900)), cam), r = w2s(add(b0, mul(sd, 900)), cam), t = w2s(add(b0, vec(0, 600, 0)), cam);
            float tw = (r.x - l.x) * 0.12f;
            Color vc = S.tod == TD_NIGHT ? C(25, 12, 10) : C(90, 45, 38);
            DrawTriangle(v2(t.x - tw, t.y), l, r, vc); DrawTriangle(v2(t.x - tw, t.y), r, v2(t.x + tw, t.y), vc);
            DrawTexturePro(texGlow, Rectangle{0, 0, 64, 64}, Rectangle{t.x - tw * 3, t.y - tw * 1.5f, tw * 6, tw * 3}, v2(0, 0), 0, alpha(C(255, 120, 40), 0.8f));
            for (int k = 0; k < 7; k++) {
                float rise = fmodf(gTime * 0.03f + k / 7.0f, 1.0f), sz = tw * (2 + rise * 8);
                DrawTexturePro(texGlow, Rectangle{0, 0, 64, 64}, Rectangle{t.x - sz / 2 + rise * tw * 4, t.y - rise * tw * 12 - sz / 2, sz, sz}, v2(0, 0), 0, alpha(C(70, 55, 55), 0.5f * (1 - rise)));
            }
        }
    }
    if (nightK < 0.9f || curWeather != W_CLEAR) {
        Color cc = mixC(mixC(C(255, 255, 255), C(255, 180, 150), S.tod == TD_SUNSET ? 0.6f : 0.0f), C(90, 95, 105), curWeather == W_STORM || curWeather == W_RAIN ? wI * 0.8f : 0.0f);
        if (nightK > 0.9f) cc = C(60, 60, 80);
        for (int i = 0; i < 26; i++) {
            float az = hash1(i * 13) * 2 * PI_F + gTime * 0.004f, el = 0.06f + hash1(i * 29) * 0.3f;
            V3 d = vec(cosf(el) * sinf(az), sinf(el), cosf(el) * cosf(az));
            Vector2 p; if (!proj(cam, d, p)) continue;
            float w = 160 + 220 * hash1(i * 7), hh = w * 0.28f;
            DrawTexturePro(texGlow, Rectangle{0, 0, 64, 64}, Rectangle{p.x - w / 2, p.y - hh / 2, w, hh}, v2(0, 0), 0, alpha(cc, 0.45f));
            DrawTexturePro(texGlow, Rectangle{0, 0, 64, 64}, Rectangle{p.x - w * 0.3f, p.y - hh * 0.7f, w * 0.6f, hh}, v2(0, 0), 0, alpha(cc, 0.35f));
        }
    }
    // distant skyline (city) or islands (beach)
    {
        Color sil = S.tod == TD_NIGHT ? C(12, 12, 24) : S.tod == TD_SUNSET ? C(70, 46, 70) : C(140, 160, 190);
        if (curTrack == 1) sil = S.tod == TD_NIGHT ? C(10, 18, 20) : S.tod == TD_SUNSET ? C(60, 50, 60) : C(90, 140, 120);
        if (curTrack == 3) sil = S.tod == TD_NIGHT ? C(30, 36, 60) : S.tod == TD_SUNSET ? C(110, 90, 120) : C(120, 135, 165);
        if (curTrack == 4) sil = S.tod == TD_NIGHT ? C(30, 14, 12) : S.tod == TD_SUNSET ? C(110, 50, 40) : C(150, 90, 70);
        sil = mixC(sil, skyHor, 0.35f + 0.5f * (curWeather == W_FOG ? wI : 0.0f));
        int M = (int)skyline.size();
        for (int i = 0; i < M; i++) {
            float a0 = 2 * PI_F * i / M, a1 = 2 * PI_F * (i + 0.8f) / M;
            V3 d0 = vec(sinf(a0), 0, cosf(a0)), d1 = vec(sinf(a1), 0, cosf(a1));
            if (dot(d0, camFwd) < 0.2f && dot(d1, camFwd) < 0.2f) continue;
            V3 p0 = add(vec(camPos.x, 0, camPos.z), mul(d0, 4000.0f)), p1 = add(vec(camPos.x, 0, camPos.z), mul(d1, 4000.0f));
            Vector2 s0 = w2s(p0, cam), s1 = w2s(p1, cam);
            float hgt = curTrack == 1 ? skyline[i] * 0.35f * (0.5f + 0.5f * sinf(i * 0.37f)) : skyline[i];
            Vector2 t0 = w2s(add(p0, vec(0, hgt, 0)), cam);
            if (curTrack == 3 || curTrack == 4) {   // mountain ranges / mesas instead of a city skyline
                float x0 = std::min(s0.x, s1.x), x1 = std::max(s0.x, s1.x);
                if (x1 < -200 || x0 > SW + 200 || x1 - x0 > SW) continue;
                float base = std::max(s0.y, s1.y) + 2, w = x1 - x0;
                if (curTrack == 3) {
                    float ph = (t0.y - base) * 2.2f * (0.6f + 0.6f * hash1(i * 3));
                    Vector2 a = v2(x0 - w * 1.5f, base), b = v2(x1 + w * 1.5f, base), tip = v2((x0 + x1) / 2, base + ph);
                    DrawTriangle(tip, a, b, sil);
                    Color cap = mixC(C(240, 245, 255), skyHor, 0.3f + 0.4f * nightK);
                    DrawTriangle(tip, v2(tip.x - w * 0.55f, tip.y - ph * 0.28f), v2(tip.x + w * 0.55f, tip.y - ph * 0.28f), cap);
                } else {
                    float mh = (t0.y - base) * 0.6f * (0.4f + 0.6f * hash1(i * 7));
                    if (hash1(i * 11) < 0.5f) DrawRectangle((int)(x0 - w * 0.6f), (int)(base + mh), (int)(w * 2.2f), (int)(-mh) + 1, sil);
                }
                continue;
            }
            float x0 = std::min(s0.x, s1.x), x1 = std::max(s0.x, s1.x);
            if (x1 < 0 || x0 > SW || x1 - x0 > SW) continue;
            float top = t0.y, base = std::max(s0.y, s1.y) + 2;
            DrawRectangle((int)x0, (int)top, (int)(x1 - x0) + 1, (int)(base - top), sil);
            if (S.tod == TD_NIGHT && curWeather != W_FOG && curTrack == 0) {
                int rows = (int)((base - top) / 4), cols = (int)((x1 - x0) / 4);
                for (int ry = 1; ry < rows; ry++) for (int cx = 0; cx < cols; cx++)
                    if (hash1(i * 997 + ry * 31 + cx * 7) < 0.12f) DrawRectangle((int)(x0 + cx * 4 + 1), (int)(top + ry * 4), 2, 1, C(255, 210, 140, 160));
            }
        }
    }
}

// =====================================================================
//                       GROUND PASS (no depth)
// =====================================================================
static Color gcol(Color c, V3 p) { Color o = mixC(c, fogCol, fogF(len(sub(p, camPos)))); o.a = c.a; return o; }
static Color dayTint(Color c) {
    if (indoorTrack()) return mulC(c, C(235, 225, 190));
    float k = 0.22f + 0.78f * (1.0f - nightK);
    Color a = C((int)(c.r * (k + ambCol.r / 255.0f * 0.25f)), (int)(c.g * (k + ambCol.g / 255.0f * 0.25f)), (int)(c.b * (k + ambCol.b / 255.0f * 0.25f)), c.a);
    if (S.tod == TD_SUNSET) a = mulC(a, C(255, 215, 200));
    return a;
}
static const float AUTO_Y = -9999.0f;
static void ribbon(int i, float s0, float s1, const Texture2D& t, float uScale, float vScale, float vOff, Color col, float yFix = AUTO_Y) {
    int j = (i + 1) % N;
    V3 a = add(track[i], mul(norv[i], s0)), b = add(track[i], mul(norv[i], s1));
    V3 c = add(track[j], mul(norv[j], s1)), d = add(track[j], mul(norv[j], s0));
    if (yFix == AUTO_Y) { a.y = hAtLat(i, s0); b.y = hAtLat(i, s1); c.y = hAtLat(j, s1); d.y = hAtLat(j, s0); }
    else a.y = b.y = c.y = d.y = yFix;
    float v0 = sArc[i] / vScale + vOff, v1 = (j == 0 ? trackLen : sArc[j]) / vScale + vOff;
    texQuad(t, a, b, c, d, v2(s0 / uScale, v0), v2(s1 / uScale, v0), v2(s1 / uScale, v1), v2(s0 / uScale, v1), gcol(col, a), gcol(col, b), gcol(col, c), gcol(col, d));
}
static Color gateColor(Mode m) { return m == M_WATER ? C(40, 170, 255) : m == M_AIR ? C(190, 90, 255) : C(255, 160, 30); }
static float lightsK() {
    if (indoorTrack()) return 1.0f;
    return clampf(nightK + (curWeather == W_STORM || curWeather == W_FOG || curWeather == W_RAIN ? wI * 0.45f : 0.0f), 0, 1);
}
static bool lampOn(const Lamp& L) {
    if (L.type == 4) {   // flickering backrooms fluorescents
        if (hash1(L.id * 7 + 1) < 0.06f) return false;
        if (hash1(L.id * 13 + 5) < 0.18f) return hash1((int)(gTime * 9) + L.id * 31) > 0.25f;
    }
    if (L.type == 6) return nightK > 0.3f;
    return true;
}

static void drawGround(Camera3D& cam) {
    (void)cam;
    Color grass = dayTint(mixC(C(150, 190, 120), C(245, 248, 255), snowK * 0.9f));
    grass = mixC(grass, shade(grass, 0.7f), wetK * 0.5f);
    Color concrete = dayTint(mixC(C(200, 200, 205), C(240, 242, 250), snowK * 0.7f));
    Color sand = dayTint(mixC(C(222, 196, 150), C(240, 242, 250), snowK * 0.7f));
    Color water = dayTint(mixC(C(150, 200, 230), C(110, 140, 170), wetK));
    if (curTrack == 1) water = dayTint(C(90, 210, 230));
    if (S.tod == TD_NIGHT && !indoorTrack()) water = C(40, 60, 90);
    Color poolWater = C(110, 200, 220);
    Color asph = dayTint(mixC(C(255, 255, 255), C(150, 150, 160), wetK * 0.6f));
    asph = mixC(asph, C(250, 252, 255), snowK * 0.25f);
    Color sandRoad = dayTint(mixC(C(225, 212, 195), C(200, 190, 175), wetK * 0.5f));
    Color dirt = dayTint(C(220, 200, 170));
    Color carpet = dayTint(C(255, 245, 215));
    float wv = gTime * 0.04f;
    Color snowC = dayTint(C(238, 244, 252)), iceC = dayTint(C(225, 240, 252)), desertC = dayTint(C(236, 196, 140));
    Color basaltC = dayTint(C(165, 155, 150)), plateC = C(150, 158, 175);
    float lavaPulse = 0.85f + 0.15f * sinf(gTime * 2.0f);
    Color lavaC = C((int)(255 * lavaPulse), (int)(150 * lavaPulse), 70);
    // ground tiles: grass / sea / dark carpet / snow / desert sand (none in space)
    const Texture2D& gt = curTrack == 1 ? texWater : curTrack == 2 ? texCarpet : curTrack == 3 ? texSnow : curTrack == 4 ? texSand : texGrass;
    Color gc = curTrack == 1 ? water : curTrack == 2 ? C(60, 52, 30) : curTrack == 3 ? snowC : curTrack == 4 ? desertC : grass;
    const float TS = 100.0f; int cx = (int)floorf(camPos.x / TS), cz = (int)floorf(camPos.z / TS);
    int R = spaceTrack() ? -1 : (int)(drawR / TS) + 1;
    float gofs = curTrack == 1 ? wv * 4 : 0;
    for (int gx = cx - R; gx <= cx + R && R >= 0; gx++)
        for (int gz = cz - R; gz <= cz + R; gz++) {
            V3 a = vec(gx * TS, -0.05f, gz * TS), b = vec((gx + 1) * TS, -0.05f, gz * TS), c = vec((gx + 1) * TS, -0.05f, (gz + 1) * TS), d = vec(gx * TS, -0.05f, (gz + 1) * TS);
            if (dist2D(mul(add(a, c), 0.5f), camPos) > drawR + 80) continue;
            float u = gx * 8.0f, v = gz * 8.0f + gofs;
            texQuad(gt, a, b, c, d, v2(u, v), v2(u + 8, v), v2(u + 8, v + 8), v2(u, v + 8), gcol(gc, a), gcol(gc, b), gcol(gc, c), gcol(gc, d));
        }
    float R2 = drawR * drawR;
    std::vector<int> vis; vis.reserve(800);
    for (int i = 0; i < N; i++) {
        float dx = track[i].x - camPos.x, dz = track[i].z - camPos.z;
        float rr = drawR + outerW[i];
        if (dx * dx + dz * dz < std::max(R2, rr * rr)) vis.push_back(i);
    }
    // outer ribbons grouped by texture: water, sand, grass, concrete
    for (int pass = 0; pass < 4; pass++)
        for (int i : vis) {
            float ow = outerW[i]; if (ow < 1) continue;
            Theme t = theme[i]; float L = lim[i];
            switch (pass) {
                case 0: if (t == TH_BAY) ribbon(i, -ow, ow, texWater, 18, 18, wv, water, 0.0f);
                        if (t == TH_LAGOON) { ribbon(i, L, L + 45, texWater, 16, 16, wv, water, 0.0f); ribbon(i, -L - 45, -L, texWater, 16, 16, wv, water, 0.0f); } break;
                case 1: if (t == TH_CANYON || t == TH_BEACH) { ribbon(i, L, ow, texSand, 10, 10, 0, sand); ribbon(i, -ow, -L, texSand, 10, 10, 0, sand); }
                        if (t == TH_LAGOON) { ribbon(i, L + 45, ow, texSand, 10, 10, 0, sand, 0.0f); ribbon(i, -ow, -L - 45, texSand, 10, 10, 0, sand, 0.0f); } break;
                case 2: if (t == TH_COUNTRY) { ribbon(i, L, ow, texGrass, 10, 10, 0, grass); ribbon(i, -ow, -L, texGrass, 10, 10, 0, grass); } break;
                default: if (t == TH_CITY || t == TH_HARBOR || t == TH_RIVER) { ribbon(i, L, ow, texConcrete, 10, 10, 0, shade(concrete, 0.8f)); ribbon(i, -ow, -L, texConcrete, 10, 10, 0, shade(concrete, 0.8f)); } break;
            }
        }
    std::vector<int> nearI; nearI.reserve(vis.size());
    for (int i : vis) { float dx = track[i].x - camPos.x, dz = track[i].z - camPos.z; if (dx * dx + dz * dz <= R2) nearI.push_back(i); }
    // embankments under raised roads
    for (int i : nearI) {
        if (th[i] < 0.05f && th[(i + 1) % N] < 0.05f) continue;
        if (outerW[i] > 1 || spaceTrack()) continue;
        float L = lim[i];
        const Texture2D& et = curTrack == 3 ? texSnow : curTrack == 4 ? texSand : texGrass;
        Color ec = curTrack == 3 ? snowC : curTrack == 4 ? desertC : grass;
        ribbon(i, L, L + 42, et, 10, 10, 0, ec); ribbon(i, -L - 42, -L, et, 10, 10, 0, ec);
    }
    // frozen lake, fjord water, oasis pools, lava fields and the crater lake around the road
    for (int i : nearI) {
        Theme t = theme[i]; float L = lim[i];
        switch (t) {
            case TH_ICE: ribbon(i, L, L + 60, texIce, 14, 14, 0, iceC, 0.0f); ribbon(i, -L - 60, -L, texIce, 14, 14, 0, iceC, 0.0f); break;
            case TH_FJORD: ribbon(i, L, L + 50, texWater, 16, 16, wv, dayTint(C(70, 120, 150)), 0.0f); ribbon(i, -L - 50, -L, texWater, 16, 16, wv, dayTint(C(70, 120, 150)), 0.0f); break;
            case TH_OASIS: ribbon(i, L, L + 38, texWater, 16, 16, wv, dayTint(C(80, 220, 210)), 0.0f); ribbon(i, -L - 38, -L, texWater, 16, 16, wv, dayTint(C(80, 220, 210)), 0.0f); break;
            case TH_VOLCANO: ribbon(i, L + 2, L + 32, texLava, 12, 12, -gTime * 0.05f, lavaC, 0.0f); ribbon(i, -L - 32, -L - 2, texLava, 12, 12, -gTime * 0.05f, lavaC, 0.0f);
                             ribbon(i, L, L + 2, texBasalt, 4, 4, 0, basaltC); ribbon(i, -L - 2, -L, texBasalt, 4, 4, 0, basaltC); break;
            case TH_CRATER: ribbon(i, -L - 140, L + 140, texLava, 20, 20, -gTime * 0.03f, lavaC, 0.0f); break;
            default: break;
        }
    }
    for (int pass = 0; pass < 7; pass++)
        for (int i : nearI) {
            Theme t = theme[i]; Mode m = zmode[i]; float h = hw[i], L = lim[i];
            bool wat = (m == M_WATER || t == TH_BAY);
            switch (pass) {
                case 0: if (wat) {
                            Color wc = t == TH_POOL ? poolWater : t == TH_FJORD ? dayTint(C(70, 120, 150)) : t == TH_OASIS ? dayTint(C(80, 220, 210))
                                     : t == TH_COOLANT ? C(140, 90, 255) : water;
                            ribbon(i, -std::max(L, h), std::max(L, h), texWater, 16, 16, t == TH_COOLANT ? wv * 3 : wv, wc, 0.0f);
                        } break;
                case 1: if (!wat && (t == TH_CITY || t == TH_TUNNEL)) { ribbon(i, h, L, texConcrete, 4, 4, 0, concrete); ribbon(i, -L, -h, texConcrete, 4, 4, 0, concrete); } break;
                case 2: if (!wat && t == TH_COUNTRY) { ribbon(i, h, h + 4, curTrack == 1 ? texSand : texDirt, 4, 4, 0, curTrack == 1 ? sand : dirt); ribbon(i, -h - 4, -h, curTrack == 1 ? texSand : texDirt, 4, 4, 0, curTrack == 1 ? sand : dirt); } break;
                case 3: if (!wat && (t == TH_CANYON || t == TH_BEACH || t == TH_DESERT)) { ribbon(i, h, L, texSand, 10, 10, 0, t == TH_DESERT ? desertC : sand); ribbon(i, -L, -h, texSand, 10, 10, 0, t == TH_DESERT ? desertC : sand); }
                        if (!wat && t == TH_SNOW) { ribbon(i, h, L, texSnow, 6, 6, 0, snowC); ribbon(i, -L, -h, texSnow, 6, 6, 0, snowC); }
                        if (!wat && t == TH_ICE) ribbon(i, -L, L, texIce, 14, 14, 0, iceC);
                        if (!wat && t == TH_VOLCANO) { ribbon(i, h, L, texBasalt, 6, 6, 0, shade(basaltC, 0.8f)); ribbon(i, -L, -h, texBasalt, 6, 6, 0, shade(basaltC, 0.8f)); }
                        if (!wat && t == TH_STATION) { ribbon(i, h, L + 0.5f, texPlate, 4, 4, 0, plateC); ribbon(i, -L - 0.5f, -h, texPlate, 4, 4, 0, plateC); } break;
                case 4: if (!wat && t == TH_HIGHWAY) { ribbon(i, h, L + 1.5f, texAsphalt, 30, 24, 0.3f, shade(asph, 0.75f)); ribbon(i, -L - 1.5f, -h, texAsphalt, 30, 24, 0.3f, shade(asph, 0.75f)); } break;
                case 5: if (!wat && t == TH_BACKROOMS) ribbon(i, -L - 0.5f, L + 0.5f, texCarpet, 6, 6, 0, carpet); break;
                default: if (!wat && t != TH_BACKROOMS && t != TH_ICE && t != TH_VOID && t != TH_CRATER && t != TH_PEAKS) {
                            if (curTrack == 1) ribbon(i, -h, h, texSandRoad, 2 * h, 24, 0.5f, sandRoad);
                            else if (t == TH_VOLCANO) ribbon(i, -h, h, texBasalt, 2 * h, 24, 0.5f, basaltC);
                            else if (t == TH_STATION) ribbon(i, -h, h, texDeckRoad, 2 * h, 30, 0.5f, C(235, 240, 255));
                            else if (t == TH_SNOW) ribbon(i, -h, h, texAsphalt, 2 * h, 24, 0.5f, mixC(asph, snowC, 0.3f));
                            else if (t == TH_DESERT) ribbon(i, -h, h, texAsphalt, 2 * h, 24, 0.5f, mulC(asph, C(255, 235, 210)));
                            else ribbon(i, -h, h, texAsphalt, 2 * h, 24, 0.5f, asph);
                        } break;
            }
        }
    // tank shortcut paths
    for (const Shortcut& sc : shortcuts) {
        if (dist2D(add(sc.pa, mul(sc.dir, sc.len / 2)), camPos) > drawR + sc.len) continue;
        const Texture2D& pt = curTrack == 3 ? texSnow : curTrack == 4 || curTrack == 1 ? texSandRoad : texDirt;
        Color pc = curTrack == 3 ? shade(snowC, 0.92f) : curTrack == 4 || curTrack == 1 ? sandRoad : dirt;
        int steps = (int)(sc.len / 8) + 1;
        for (int k = 0; k < steps; k++) {
            float u0 = sc.len * k / steps, u1 = sc.len * (k + 1) / steps;
            V3 c0 = add(sc.pa, mul(sc.dir, u0)), c1 = add(sc.pa, mul(sc.dir, u1));
            V3 a = add(c0, mul(sc.nrm, -sc.w / 2)), b = add(c0, mul(sc.nrm, sc.w / 2)), c = add(c1, mul(sc.nrm, sc.w / 2)), d = add(c1, mul(sc.nrm, -sc.w / 2));
            int ia = sc.a + (int)((float)((sc.b - sc.a + N) % N) * k / steps);
            a.y = b.y = roadHeight(c0, ia % N) + 0.02f; c.y = d.y = roadHeight(c1, ia % N) + 0.02f;
            texQuad(pt, a, b, c, d, v2(0, u0 / 10), v2(1, u0 / 10), v2(1, u1 / 10), v2(0, u1 / 10), gcol(pc, a), gcol(pc, b), gcol(pc, c), gcol(pc, d));
        }
    }
    // start line
    rlCheckRenderBatchLimit(400);
    {
        V3 c = track[0], t = tanv[0], n = norv[0]; float w = hw[0];
        for (int k = 0; k < 16; k++) for (int row = 0; row < 3; row++) {
            float s0 = -w + 2 * w * k / 16.0f, s1 = -w + 2 * w * (k + 1) / 16.0f;
            V3 a = add(add(c, mul(n, s0)), mul(t, (row - 1.5f) * 1.6f)), b = add(add(c, mul(n, s1)), mul(t, (row - 1.5f) * 1.6f));
            a.y = b.y = 0.01f;
            V3 d = add(a, mul(t, 1.6f)), e = add(b, mul(t, 1.6f));
            Color col = gcol(dayTint(((k + row) & 1) ? C(20, 20, 20) : C(250, 250, 250)), a);
            texQuad1(texWhite, a, b, e, d, v2(0, 0), v2(1, 1), col);
        }
    }
    BeginBlendMode(BLEND_ALPHA);
    for (size_t g = 0; g < gateIdx.size(); g++) {
        int gi = gateIdx[g]; if (dist2D(track[gi], camPos) > drawR) continue;
        Mode nm = zmode[(gi + 3) % N];
        Color gcc = alpha(gateColor(nm), 0.35f + 0.15f * sinf(gTime * 5.0f));
        for (int k = 0; k < 6; k++) {
            int i = (gi + k) % N, j = (i + 1) % N; float w = std::max(hw[i], 14.0f);
            V3 a = add(track[i], mul(norv[i], -w)), b = add(track[i], mul(norv[i], w)), c = add(track[j], mul(norv[j], w)), d = add(track[j], mul(norv[j], -w));
            a.y = b.y = th[i] + 0.03f; c.y = d.y = th[j] + 0.03f;
            texQuad1(texWhite, a, b, c, d, v2(0, 0), v2(1, 1), gcc);
        }
    }
    for (const Pad& p : pads) {
        if (dist2D(p.p, camPos) > 500) continue;
        V3 f = fwdV(p.yaw), r = rgtV(p.yaw);
        auto P = [&](float s, float l) { return add(vec(p.p.x, p.p.y + 0.03f, p.p.z), add(mul(r, s), mul(f, l))); };
        float pulse = 0.6f + 0.4f * sinf(gTime * 8);
        texQuad1(texWhite, P(-3.4f, -5), P(3.4f, -5), P(3.4f, 5), P(-3.4f, 5), v2(0, 0), v2(1, 1), C(20, 20, 30, 230));
        for (int k = -1; k <= 1; k++) {
            float l = k * 3.0f; Color cc = alpha(C(0, 200, 255), pulse);
            texQuad1(texWhite, P(-2.6f, l - 1.0f), P(0, l + 0.4f), P(0, l + 1.4f), P(-2.6f, l), v2(0, 0), v2(1, 1), cc);
            texQuad1(texWhite, P(0, l + 0.4f), P(2.6f, l - 1.0f), P(2.6f, l), P(0, l + 1.4f), v2(0, 0), v2(1, 1), cc);
        }
    }
    for (const Hazard& h : hazards) {      // oil slicks and boss ice patches
        if ((h.type != 0 && h.type != 2) || dist2D(h.p, camPos) > 300) continue;
        float k = clampf(h.life / 3.0f, 0, 1);
        if (h.type == 0) groundGlow(add(h.p, vec(0, 0.05f, 0)), vec(4.2f, 0, 0), vec(0, 0, 3.6f), C(5, 5, 10, (int)(235 * k)));
        else groundGlow(add(h.p, vec(0, 0.05f, 0)), vec(5.0f, 0, 0), vec(0, 0, 4.4f), C(200, 240, 255, (int)(220 * k)));
    }
    for (const Racer& r : racers) {
        if (r.dead || r.out || dist2D(r.pos, camPos) > 400) continue;
        float gy = r.mode == M_WATER ? 0.0f : (r.mode == M_AIR ? (zmode[r.lastIdx] == M_AIR ? 0.0f : th[r.lastIdx]) : roadHeight(r.pos, r.lastIdx) + rampHeight(r.pos));
        float alt = std::max(0.0f, r.pos.y - gy), sc = 1.0f / (1.0f + alt * 0.04f);
        V3 f = fwdV(r.yaw), rt = rgtV(r.yaw);
        float rad = radiusOf(r);
        V3 c = vec(r.pos.x - lightDir.x * alt * 0.3f, gy + 0.04f, r.pos.z - lightDir.z * alt * 0.3f);
        groundGlow(c, mul(rt, rad * 0.9f * sc + 0.4f), mul(f, rad * 1.5f * sc + 0.4f), C(0, 0, 0, (int)(170 * sc)));
    }
    EndBlendMode();
    BeginBlendMode(BLEND_ADDITIVE);
    for (const Hazard& h : hazards) if (h.type == 0 && dist2D(h.p, camPos) < 200)   // rainbow sheen on oil
        groundGlow(add(h.p, vec(0.6f, 0.06f, 0.4f)), vec(2.0f, 0, 0), vec(0, 0, 1.6f), C((int)(40 + 30 * sinf(gTime * 2)), 20, (int)(50 + 30 * cosf(gTime * 2))));
    for (const Hazard& h : hazards) if (h.type == 3 && dist2D(h.p, camPos) < 250) {  // burning trail
        float fl = 0.7f + 0.3f * sinf(gTime * 20 + h.p.x);
        groundGlow(add(h.p, vec(0, 0.06f, 0)), vec(3.0f, 0, 0), vec(0, 0, 3.0f), shade(C(255, 110, 20), fl * clampf(h.life, 0, 1)));
    }
    for (const LavaBomb& b : lavaBombs) {   // impact warning rings
        float k = clampf(1.0f - b.t / 2.2f, 0, 1), pulse = 0.5f + 0.5f * sinf(gTime * (8 + 16 * k));
        groundGlow(add(b.p, vec(0, 0.07f, 0)), vec(6.5f, 0, 0), vec(0, 0, 6.5f), shade(C(255, 60, 20), (0.3f + 0.7f * k) * pulse));
    }
    // lava and plasma light up their surroundings
    for (int i : nearI) {
        if (i % 3) continue;
        Theme t = theme[i];
        if (t == TH_VOLCANO) for (int s = -1; s <= 1; s += 2) groundGlow(add(add(track[i], mul(norv[i], s * (lim[i] + 6))), vec(0, 0.08f, 0)), vec(14, 0, 0), vec(0, 0, 14), shade(C(255, 110, 30), 0.5f));
        if (t == TH_STATION) for (int s = -1; s <= 1; s += 2) groundGlow(add(add(track[i], mul(norv[i], s * hw[i])), vec(0, th[i] + 0.06f, 0)), vec(0.9f, 0, 0), vec(0, 0, 0.9f), ((i / 3) % 2) ? C(0, 160, 220) : C(170, 40, 220));
        if (t == TH_COOLANT && i % 6 == 0) groundGlow(add(track[i], vec(sinf(gTime * 0.7f + i) * 6, 0.1f, cosf(gTime * 0.5f + i * 1.7f) * 6)), vec(6, 0, 0), vec(0, 0, 6), shade(C(150, 80, 255), 0.25f));
    }
    float lampK = lightsK();
    float reflK = clampf(wetK + nightK * 0.25f + (indoorTrack() ? 0.2f : 0.0f), 0, 1);
    if (lampK > 0.05f) {
        for (const Lamp& L : lamps) {
            float d = dist2D(L.head, camPos); if (d > 420 || !lampOn(L)) continue;
            float k = lampK * (1.0f - d / 420.0f);
            float gy = L.type >= 4 && L.type <= 5 ? (L.type == 5 ? 0.05f : 0.05f) : L.base.y + 0.05f;
            V3 g = vec(L.head.x, gy, L.head.z);
            float r = L.type == 3 ? 9.0f : L.type == 6 ? 6.0f : L.type >= 4 ? 7.0f : 13.0f;
            float inten = L.type >= 4 && L.type <= 5 ? 0.3f : 0.55f;
            groundGlow(g, vec(r, 0, 0), vec(0, 0, r), alpha(shade(L.c, inten * k), 1));
            if (reflK > 0.05f && L.type != 6) {
                V3 toC = norm(vec(camPos.x - g.x, 0, camPos.z - g.z));
                V3 side = vec(toC.z, 0, -toC.x);
                groundGlow(add(g, mul(toC, 14.0f)), mul(side, 1.6f), mul(toC, 16.0f), shade(L.c, 0.4f * k * reflK));
            }
        }
        for (const Bld& b : blds) {
            if (!b.neon || reflK < 0.1f) continue;
            float d = dist2D(b.p, camPos); if (d > 300) continue;
            V3 fwdB = fwdV(b.yaw), sideB = rgtV(b.yaw);
            for (int s = -1; s <= 1; s += 2) {
                V3 face = add(b.p, mul(sideB, s * b.d / 2));
                if (dot(sub(camPos, face), mul(sideB, (float)s)) < 0) continue;
                V3 g = add(face, mul(sideB, s * 6.0f)); g.y = 0.05f;
                groundGlow(g, mul(fwdB, b.w * 0.25f), mul(sideB, 8.0f), shade(b.neonC, 0.35f * reflK * (1 - d / 300.0f) * nightK));
            }
        }
    }
    float headK = indoorTrack() ? 0.25f : clampf(nightK + wetK * 0.4f + (curWeather == W_FOG ? wI * 0.6f : 0.0f), 0, 1);
    for (const Racer& r : racers) {
        if (r.dead || r.out || r.mode == M_AIR || dist2D(r.pos, camPos) > 300 || headK < 0.05f) continue;
        V3 f = fwdV(r.yaw), rt = rgtV(r.yaw);
        float gy = r.mode == M_WATER ? 0.06f : roadHeight(r.pos, r.lastIdx) + 0.06f;
        V3 o = vec(r.pos.x, gy, r.pos.z);
        float near = radiusOf(r);
        V3 a = add(o, add(mul(f, near - 0.5f), mul(rt, -2.0f))), b = add(o, add(mul(f, near - 0.5f), mul(rt, 2.0f)));
        V3 c = add(o, add(mul(f, near + 36), mul(rt, 11.0f))), d = add(o, add(mul(f, near + 36), mul(rt, -11.0f)));
        Color hc = shade(C(255, 236, 200), 0.42f * headK);
        texQuad1(texBeam, a, b, c, d, v2(0, 0), v2(1, 1), hc);
        if ((r.base == B_CAR || r.base == B_KART) && nightK > 0.3f && !r.isCop) groundGlow(o, mul(rt, 2.2f), mul(f, 3.4f), shade(r.color, 0.45f * nightK));
        if (r.isCop) { bool ph = fmodf(gTime * 3, 1.0f) < 0.5f; groundGlow(o, vec(7, 0, 0), vec(0, 0, 7), ph ? C(90, 10, 10) : C(10, 20, 100)); }
        if (reflK > 0.1f) groundGlow(add(o, mul(f, -near - 2.0f)), mul(rt, 1.4f), mul(f, 4.0f), shade(C(255, 30, 30), 0.35f * reflK));
        if (r.boosting || r.boostT > 0) groundGlow(add(o, mul(f, -near - 1)), mul(rt, 2.0f), mul(f, 3.0f), shade(r.boostT > 0 && !r.boosting ? C(255, 140, 40) : C(60, 140, 255), 0.6f));
        if (r.driftCharge > 0.8f) groundGlow(add(o, mul(f, -1.3f)), mul(rt, 2.0f), mul(f, 1.6f), shade(driftColor(driftStage(r)), 0.5f));
    }
    for (int i = 0; i < N; i += 3) {
        if (theme[i] != TH_CANYON) continue;
        float d = dist2D(track[i], camPos); if (d > 600) continue;
        Color lc = ((i / 3) % 2) ? C(80, 200, 255) : C(200, 120, 255);
        float k = clampf(0.4f + nightK, 0, 1) * (0.6f + 0.4f * sinf(gTime * 6 - i * 0.3f));
        for (int s = -1; s <= 1; s += 2) groundGlow(add(add(track[i], mul(norv[i], s * (hw[i] + 0.5f))), vec(0, 0.06f, 0)), vec(1.2f, 0, 0), vec(0, 0, 1.2f), shade(lc, k));
    }
    EndBlendMode();
}

// =====================================================================
//                       WORLD OBJECTS (3D)
// =====================================================================
struct GlowPt { V3 p; float s; Color c; };
static std::vector<GlowPt> glows;
static inline void addGlow(V3 p, float s, Color c) { glows.push_back({p, s, c}); }
static Texture2D texQ;
static void genItemTex() {
    Image img = GenImageColor(64, 64, C(255, 255, 255, 70));
    ImageDrawRectangle(&img, 0, 0, 64, 5, WHITE); ImageDrawRectangle(&img, 0, 59, 64, 5, WHITE);
    ImageDrawRectangle(&img, 0, 0, 5, 64, WHITE); ImageDrawRectangle(&img, 59, 0, 5, 64, WHITE);
    ImageDrawText(&img, "?", 21, 8, 50, WHITE);
    texQ = finishTex(img, false);
}
static Color rainbow(float t) {
    float r = 0.5f + 0.5f * sinf(t), g = 0.5f + 0.5f * sinf(t + 2.09f), b = 0.5f + 0.5f * sinf(t + 4.19f);
    return C((int)(80 + 175 * r), (int)(80 + 175 * g), (int)(80 + 175 * b));
}

static std::vector<int> visBld;
static void drawBuildings() {
    float lk = lightsK();
    visBld.clear();
    for (size_t k = 0; k < blds.size(); k++) {
        const Bld& b = blds[k];
        float rad = std::max(b.w, b.d) * 0.7f;
        if (visible(add(b.p, vec(0, b.h * 0.3f, 0)), rad + b.h * 0.3f, drawR)) visBld.push_back((int)k);
    }
    for (int tx = 0; tx < 3; tx++)
        for (int k : visBld) {
            const Bld& b = blds[k]; if (b.tex != tx) continue;
            setObj(add(b.p, vec(0, b.h * 0.5f, 0)), b.yaw);
            rlPushMatrix(); rlTranslatef(b.p.x, 0, b.p.z); rlRotatef(b.yaw * RAD2DEG, 0, 1, 0);
            texBoxLocal(texFacade[b.tex], b.d / 2, b.w / 2, 0, b.h, b.tint, 24, 48, nightK * 0.9f, false);
            rlPopMatrix();
        }
    for (int k : visBld) {
        const Bld& b = blds[k];
        setObj(add(b.p, vec(0, b.h * 0.5f, 0)), b.yaw);
        rlPushMatrix(); rlTranslatef(b.p.x, 0, b.p.z); rlRotatef(b.yaw * RAD2DEG, 0, 1, 0);
        float hx = b.d / 2, hz = b.w / 2;
        box(0, b.h + 0.15f, 0, b.d - 0.1f, 0.3f, b.w - 0.1f, C(70, 72, 78));
        box(hx * 0.3f, b.h + 1.2f, -hz * 0.3f, hx * 0.6f, 2.4f, hz * 0.5f, C(90, 92, 98));
        float fx = -(float)b.side * (hx + 0.35f);
        if (b.neon) {
            Color nc = shade(b.neonC, 0.55f + 0.45f * lk);
            float flick = (hash1((int)(gTime * 8) + (int)b.p.x) < 0.03f) ? 0.3f : 1.0f;
            if (b.neon == 1) {
                float y0 = std::min(b.h * 0.25f, 12.0f), y1 = std::min(b.h * 0.8f, y0 + 20.0f);
                box(fx, (y0 + y1) / 2, hz * 0.55f, 0.6f, y1 - y0, 2.6f, shade(nc, flick), true);
                if (lk > 0.2f) for (float y = y0 + 2; y < y1; y += 4) addGlow(add(b.p, rotY(vec(fx * 1.05f, y, hz * 0.55f), b.yaw)), 9.0f, shade(b.neonC, 0.5f * lk * flick));
            } else {
                float y = std::min(b.h * 0.18f, 6.0f) + 3.0f;
                box(fx, y, 0, 0.5f, 0.9f, b.w * 0.85f, shade(nc, flick), true);
                if (lk > 0.2f) for (float z = -b.w * 0.35f; z <= b.w * 0.36f; z += 5) addGlow(add(b.p, rotY(vec(fx * 1.05f, y, z), b.yaw)), 7.0f, shade(b.neonC, 0.4f * lk * flick));
            }
        }
        if (b.antenna) {
            box(-hx * 0.3f, b.h + 6.0f, hz * 0.3f, 0.4f, 12.0f, 0.4f, C(120, 120, 125));
            if (fmodf(gTime + b.p.x * 0.01f, 1.4f) < 0.7f) addGlow(add(b.p, rotY(vec(-hx * 0.3f, b.h + 12.5f, hz * 0.3f), b.yaw)), 6.0f, C(255, 30, 30));
        }
        rlPopMatrix();
    }
}
static void drawLamps() {
    float lk = lightsK();
    for (const Lamp& L : lamps) {
        if (!visible(L.head, 6, std::min(drawR, 650.0f))) continue;
        setObj(L.head, 0);
        bool on = lampOn(L);
        Color pole = C(70, 74, 82);
        if (L.type <= 2) {
            float h = L.head.y - L.base.y;
            box(L.base.x, L.base.y + h / 2, L.base.z, 0.35f, h, 0.35f, pole);
            V3 top = vec(L.base.x, L.head.y + 0.2f, L.base.z), dir = sub(vec(L.head.x, top.y, L.head.z), top);
            float al = len(dir); float ay = atan2f(dir.x, dir.z);
            rlPushMatrix(); rlTranslatef(top.x + dir.x / 2, top.y, top.z + dir.z / 2); rlRotatef(ay * RAD2DEG, 0, 1, 0);
            box(0, 0, 0, 0.22f, 0.22f, al, pole);
            box(0, -0.15f, al / 2 - 0.4f, 0.6f, 0.2f, 1.4f, shade(L.c, 0.4f + 0.6f * lk), true);
            rlPopMatrix();
        } else if (L.type == 3 || L.type == 4 || L.type == 5) {
            rlPushMatrix(); rlTranslatef(L.head.x, L.head.y, L.head.z); rlRotatef(L.armYaw * RAD2DEG, 0, 1, 0);
            if (L.type == 3) box(0, 0, 0, 0.8f, 0.2f, 3.0f, L.c, true);
            else box(0, 0, 0, 1.5f, 0.08f, 2.8f, on ? L.c : C(90, 88, 80), true);
            rlPopMatrix();
        } else if (L.type == 6) {
            box(L.base.x, L.base.y + 1.5f, L.base.z, 0.16f, 3.0f, 0.16f, C(150, 115, 60));
            box(L.base.x, L.base.y + 3.0f, L.base.z, 0.35f, 0.35f, 0.35f, C(60, 45, 30));
            if (on) {
                float fl = 0.8f + 0.2f * sinf(gTime * 23 + L.id) + 0.1f * sinf(gTime * 41 + L.id * 3);
                addGlow(add(L.head, vec(0, 0.25f, 0)), 2.6f * fl, C(255, 170, 60));
                addGlow(add(L.head, vec(0, 0.2f, 0)), 7.0f * fl, C(140, 70, 20));
                if (GetRandomValue(0, 100) < 6) emit(add(L.head, vec(0, 0.4f, 0)), vec(frand(-0.3f, 0.3f), 2.0f, frand(-0.3f, 0.3f)), 0.5f, 0.25f, C(255, 160, 50), false, true);
            }
            continue;
        }
        if (lk > 0.05f && on) addGlow(add(L.head, vec(0, -0.3f, 0)), L.type == 3 ? 7.0f : L.type >= 4 ? 5.0f : 9.0f, shade(L.c, (L.type >= 4 ? 0.5f : 0.8f) * lk));
    }
}
static void drawPalm(const Tree& t) {
    float s = t.s;
    float lean = (hash1((int)(t.p.x * 10)) - 0.5f) * 1.2f, dir = hash1((int)(t.p.z * 10)) * 6.28f;
    V3 ld = vec(cosf(dir), 0, sinf(dir));
    V3 prev = t.p;
    Color trunk = C(150, 115, 75);
    for (int k = 0; k < 6; k++) {
        float f = (k + 1) / 6.0f;
        V3 nxt = add(t.p, add(vec(0, 7.5f * s * f, 0), mul(ld, lean * 3.0f * f * f * s)));
        V3 mid = mul(add(prev, nxt), 0.5f);
        box(mid.x, mid.y, mid.z, 0.55f * s * (1.1f - f * 0.3f), 7.5f * s / 6.0f + 0.1f, 0.55f * s * (1.1f - f * 0.3f), (k & 1) ? trunk : shade(trunk, 0.85f));
        prev = nxt;
    }
    Color leaf = mixC(C(50, 150, 60), C(240, 245, 250), snowK * 0.7f);
    for (int k = 0; k < 7; k++) {
        float a = k * 2 * PI_F / 7 + t.p.x;
        V3 d = vec(cosf(a), 0, sinf(a)), side = vec(-d.z, 0, d.x);
        V3 m = add(prev, add(mul(d, 2.6f * s), vec(0, 0.5f * s, 0)));
        V3 tip = add(prev, add(mul(d, 4.6f * s), vec(0, -1.4f * s, 0)));
        tri2(prev, add(m, mul(side, 0.9f * s)), m, leaf);
        tri2(prev, m, add(m, mul(side, -0.9f * s)), shade(leaf, 0.9f));
        tri2(m, add(m, mul(side, 0.9f * s)), tip, shade(leaf, 0.95f));
        tri2(m, tip, add(m, mul(side, -0.9f * s)), shade(leaf, 0.85f));
    }
    ball(add(prev, vec(0.3f, -0.4f, 0.2f)), 0.3f * s, C(110, 80, 40), BLANK, 3, 5);
    ball(add(prev, vec(-0.3f, -0.45f, 0.1f)), 0.3f * s, C(110, 80, 40), BLANK, 3, 5);
}
static void drawTrees() {
    float R = std::min(drawR, 650.0f);
    Color leaf0 = mixC(C(70, 140, 55), C(240, 245, 252), snowK * 0.8f), leaf1 = mixC(C(35, 95, 50), C(235, 240, 250), snowK * 0.8f);
    if (S.tod == TD_SUNSET) { leaf0 = mulC(leaf0, C(255, 220, 190)); leaf1 = mulC(leaf1, C(255, 220, 190)); }
    for (const Tree& t : trees) {
        if (!visible(t.p, 8, t.kind == 3 ? 220.0f : t.kind == 2 ? std::min(R, 480.0f) : R)) continue;
        setObj(t.p, 0);
        float s = t.s; float y = t.p.y;
        if (t.kind == 3) {
            rlPushMatrix(); rlTranslatef(t.p.x, 0, t.p.z);
            texBoxLocal(texWallpaper, 0.8f, 0.8f, 0, 6.0f, C(255, 250, 230), 4, 4, 0, false);
            rlPopMatrix();
            continue;
        }
        if (t.kind == 2) { drawPalm(t); continue; }
        if (t.kind == 4) {   // saguaro cactus
            Color cc = S.tod == TD_SUNSET ? C(90, 130, 70) : C(70, 140, 70);
            box(t.p.x, y + 2.4f * s, t.p.z, 0.7f * s, 4.8f * s, 0.7f * s, cc);
            box(t.p.x + 0.8f * s, y + 2.2f * s, t.p.z, 0.9f * s, 0.45f * s, 0.45f * s, cc); box(t.p.x + 1.2f * s, y + 3.0f * s, t.p.z, 0.45f * s, 1.6f * s, 0.45f * s, cc);
            box(t.p.x - 0.7f * s, y + 2.9f * s, t.p.z, 0.8f * s, 0.4f * s, 0.4f * s, cc); box(t.p.x - 1.05f * s, y + 3.5f * s, t.p.z, 0.4f * s, 1.3f * s, 0.4f * s, cc);
            continue;
        }
        if (t.kind == 5) {   // pine under snow
            Color dk = C(30, 80, 55), sn = C(240, 245, 252);
            box(t.p.x, y + 1.2f * s, t.p.z, 0.6f * s, 2.4f * s, 0.6f * s, C(90, 62, 42));
            cone(vec(t.p.x, y + 1.6f * s, t.p.z), 3.6f * s, 4.6f * s, 7, dk, sn);
            cone(vec(t.p.x, y + 3.9f * s, t.p.z), 2.8f * s, 4.2f * s, 7, sn, dk);
            cone(vec(t.p.x, y + 6.0f * s, t.p.z), 1.8f * s, 3.4f * s, 7, sn);
            continue;
        }
        box(t.p.x, y + 1.6f * s, t.p.z, 0.6f * s, 3.2f * s, 0.6f * s, C(95, 65, 42));
        if (t.kind) {
            cone(vec(t.p.x, y + 2.0f * s, t.p.z), 3.4f * s, 5.5f * s, 7, leaf1);
            cone(vec(t.p.x, y + 4.6f * s, t.p.z), 2.6f * s, 5.0f * s, 7, leaf1);
        } else blob(vec(t.p.x, y + 5.0f * s, t.p.z), 3.4f * s, 3.0f * s, 7, leaf0, t.p.x * 0.37f);
    }
    for (const Rock& r : rocks) {
        if (!visible(r.p, std::max(r.w, r.h), drawR)) continue;
        setObj(add(r.p, vec(0, r.h / 2, 0)), 0);
        V3 p[8];
        float c = cosf(r.yaw), s = sinf(r.yaw);
        float tw = 0.55f + 0.3f * hash1(r.seed), td = 0.55f + 0.3f * hash1(r.seed + 1);
        float bx[4] = {-1, 1, 1, -1}, bz[4] = {-1, -1, 1, 1};
        for (int k = 0; k < 4; k++) {
            float x = bx[k] * r.w / 2 * (0.85f + 0.3f * hash1(r.seed + k * 3)), z = bz[k] * r.d / 2 * (0.85f + 0.3f * hash1(r.seed + k * 5));
            p[k] = vec(r.p.x + x * c + z * s, -1, r.p.z - x * s + z * c);
            float x2 = x * tw, z2 = z * td;
            p[k + 4] = vec(r.p.x + x2 * c + z2 * s + (hash1(r.seed + 9) - 0.5f) * r.w * 0.2f, r.h * (0.85f + 0.3f * hash1(r.seed + k * 7)), r.p.z - x2 * s + z2 * c);
        }
        Color rc = mixC(r.c, C(240, 242, 250), snowK * 0.5f);
        hexa(p, rc);
        if (r.h > 25) {
            V3 q[8];
            for (int k = 0; k < 4; k++) { q[k] = p[k + 4]; q[k + 4] = add(r.p, mul(sub(p[k + 4], r.p), curTrack == 3 ? 0.35f : 0.92f)); q[k + 4].y = p[k + 4].y + r.h * (curTrack == 3 ? 0.45f : 0.12f); }
            hexa(q, curTrack == 3 ? C(245, 248, 255) : shade(rc, 1.1f));   // snow-capped peaks on the frost pass
            if (curTrack == 1) { V3 top = vec(r.p.x, p[4].y + r.h * 0.12f, r.p.z); blob(top, r.w * 0.35f, 3.0f, 7, C(70, 150, 70), r.p.x); }
        }
    }
}
static void drawDecos() {
    for (const Deco& d : decos) {
        if (!visible(d.p, d.type == 5 || d.type == 8 ? d.s * 12 : 6, d.type == 5 || d.type == 8 ? drawR : 420)) continue;
        setObj(d.p, d.yaw);
        rlPushMatrix(); rlTranslatef(d.p.x, d.p.y, d.p.z); rlRotatef(d.yaw * RAD2DEG, 0, 1, 0); rlScalef(d.s, d.s, d.s);
        switch (d.type) {
            case 0:   // umbrella + towel
                box(0, 1.3f, 0, 0.1f, 2.6f, 0.1f, C(230, 230, 230));
                cone(vec(0, 2.2f, 0), 2.3f, 0.7f, 10, d.c, WHITE);
                box(1.4f, 0.03f, 0.5f, 1.0f, 0.04f, 2.0f, mixC(d.c, WHITE, 0.4f));
                break;
            case 1:   // hut
                rlPopMatrix();
                rlPushMatrix(); rlTranslatef(d.p.x, d.p.y, d.p.z); rlRotatef(d.yaw * RAD2DEG, 0, 1, 0);
                texBoxLocal(texWood, 2.5f, 3.0f, 0, 2.8f, C(255, 240, 220), 4, 4, 0, false);
                box(0, 1.1f, 3.02f, 1.2f, 2.0f, 0.05f, C(60, 40, 25));
                cone(vec(0, 2.8f, 0), 4.6f, 2.6f, 4, C(215, 180, 100));
                break;
            case 2:   // surfboards
                for (int k = 0; k < 3; k++) {
                    rlPushMatrix(); rlTranslatef(k * 0.8f - 0.8f, 1.2f, 0); rlRotatef(8.0f * (k - 1), 0, 0, 1);
                    box(0, 0, 0, 0.55f, 2.4f, 0.08f, k == 1 ? WHITE : d.c);
                    box(0, 0, -0.05f, 0.1f, 2.3f, 0.02f, k == 1 ? d.c : WHITE);
                    rlPopMatrix();
                }
                break;
            case 3:   // lifeguard tower
                for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) box(sx * 1.0f, 1.5f, sz * 1.0f, 0.15f, 3.0f, 0.15f, C(240, 240, 240));
                box(0, 3.05f, 0, 2.6f, 0.15f, 2.6f, C(230, 230, 230));
                box(0, 4.0f, 0, 2.2f, 1.8f, 2.2f, C(230, 50, 40));
                box(0, 4.0f, 1.12f, 1.6f, 0.8f, 0.02f, C(40, 60, 80));
                tbox(0, 4.9f, 0, 2.8f, 2.8f, 5.6f, 0.2f, 0.2f, 0, C(240, 240, 240));
                box(1.2f, 6.2f, 0, 0.05f, 2.4f, 0.05f, C(80, 80, 80));
                box(1.6f, 7.1f, 0, 0.8f, 0.5f, 0.02f, C(255, 200, 0), true);
                break;
            case 5: {   // asteroid
                rlPopMatrix();
                blob(d.p, d.s, d.s * 0.8f, 7, C(105, 98, 92), d.p.x * 0.13f);
                blob(add(d.p, vec(d.s * 0.4f, d.s * 0.3f, 0)), d.s * 0.5f, d.s * 0.4f, 6, C(85, 80, 76), d.p.z * 0.21f);
                rlPushMatrix();
            } break;
            case 6: box(0, 0.15f, 0, 2.4f, 0.3f, 1.8f, C(235, 245, 255)); box(0.3f, 0.32f, 0.2f, 1.4f, 0.12f, 1.0f, C(250, 252, 255)); break;   // ice floe
            case 7:   // snowman
                ball(vec(0, 0.9f, 0), 0.95f, C(248, 250, 255)); ball(vec(0, 2.25f, 0), 0.65f, C(248, 250, 255)); ball(vec(0, 3.2f, 0), 0.45f, C(248, 250, 255));
                box(0, 3.2f, 0.5f, 0.12f, 0.12f, 0.45f, C(255, 130, 20));
                box(0, 3.75f, 0, 0.7f, 0.1f, 0.7f, C(30, 30, 34)); box(0, 4.05f, 0, 0.45f, 0.55f, 0.45f, C(30, 30, 34));
                box(0, 2.75f, 0, 1.05f, 0.18f, 1.05f, C(220, 40, 40));
                break;
            case 8: {   // station module: hull, solar panels, blinking lights
                texBoxLocal(texPlate, 4, 9, -3, 3, C(220, 225, 235), 4, 4, 0, true);
                box(0, 0, 0, 0.6f, 0.6f, 26, C(120, 125, 135));
                for (int s2 = -1; s2 <= 1; s2 += 2) { box(s2 * 9, 0, 0, 10, 0.15f, 5, C(40, 60, 140)); box(s2 * 9, 0.09f, 0, 10.2f, 0.02f, 0.2f, C(200, 200, 210)); }
                box(0, 3.4f, 4, 1.4f, 0.8f, 1.4f, C(200, 205, 215));
                for (int w = -2; w <= 2; w++) box(4.05f, 0.8f, w * 3.0f, 0.06f, 0.8f, 1.2f, C(255, 220, 140), true);
                if (fmodf(gTime + d.p.x * 0.01f, 1.5f) < 0.2f) addGlow(add(d.p, rotY(vec(0, 3.9f * d.s, 4 * d.s), d.yaw)), 4.0f, C(255, 60, 60));
            } break;
            case 9: {   // lava vent
                cone(vec(0, 0, 0), 2.6f, 2.2f, 8, C(55, 45, 42));
                addGlow(add(d.p, vec(0, 2.4f * d.s, 0)), 5.0f * d.s, C(255, 120, 30));
                if (GetRandomValue(0, 100) < 25) emit(add(d.p, vec(0, 2.3f * d.s, 0)), vec(frand(-2, 2), frand(6, 12), frand(-2, 2)), 1.0f, 0.5f, C(255, 150, 40), true, true);
                if (GetRandomValue(0, 100) < 8) emit(add(d.p, vec(0, 3.0f * d.s, 0)), vec(frand(-1, 1), 3, frand(-1, 1)), 3.0f, 4.0f, C(60, 55, 55, 150));
            } break;
            case 10:   // bleached rock arch
                box(-2.5f, 2.0f, 0, 1.4f, 4.0f, 1.6f, C(225, 200, 160)); box(2.5f, 2.0f, 0, 1.4f, 4.0f, 1.6f, C(225, 200, 160));
                box(0, 4.4f, 0, 6.6f, 1.2f, 1.8f, C(230, 205, 165));
                break;
            default: ball(vec(0, 0.45f, 0), 0.45f, d.c, WHITE, 4, 8); break;
        }
        rlPopMatrix();
    }
}

// ------------------------------ trackside ----------------------------
static void wallQuad(int i, int j, float s, float y0, float y1, const Texture2D& t, Color col, float uScale, float vScale) {
    V3 a = add(track[i], mul(norv[i], s * (lim[i] + 0.3f))), b = add(track[j], mul(norv[j], s * (lim[j] + 0.3f)));
    float u0 = sArc[i] / uScale, u1 = (j == 0 ? trackLen : sArc[j]) / uScale;
    V3 A = vec(a.x, y0, a.z), B = vec(b.x, y0, b.z), Cc = vec(b.x, y1, b.z), D = vec(a.x, y1, a.z);
    Color c0 = mixC(col, fogCol, fogF(len(sub(A, camPos)))), c1 = mixC(shade(col, 0.85f), fogCol, fogF(len(sub(D, camPos))));
    texQuad(t, A, B, Cc, D, v2(u0, (y1 - y0) / vScale), v2(u1, (y1 - y0) / vScale), v2(u1, 0), v2(u0, 0), c0, c0, c1, c1);
}
static void drawTrackside() {
    float lk = lightsK();
    float R2 = std::min(drawR, 500.0f); R2 *= R2;
    Color rail = C(185, 190, 198), barrier = C(175, 172, 165);
    for (int i = 0; i < N; i++) {
        float dx = track[i].x - camPos.x, dz = track[i].z - camPos.z;
        if (dx * dx + dz * dz > R2) continue;
        if ((dx * camFwd.x + dz * camFwd.z) < -60) continue;
        int j = (i + 1) % N; Theme thm = theme[i];
        setObj(track[i], 0);
        float hi = th[i], hj = th[j];
        if (thm == TH_HIGHWAY || thm == TH_CITY) {
            for (int s = -1; s <= 1; s += 2) {
                float o0 = s * (lim[i] + 0.3f), o1 = s * (lim[j] + 0.3f);
                V3 a = add(track[i], mul(norv[i], o0)), b = add(track[j], mul(norv[j], o1));
                a.y = hi; b.y = hj;
                if (!shortcuts.empty() && shortcutAt(a, 1.0f) >= 0) continue;     // gap in the rail at a shortcut
                if (thm == TH_HIGHWAY) {
                    V3 p[8] = {add(a, vec(0, 0.55f, 0)), add(b, vec(0, 0.55f, 0)), add(add(b, mul(norv[j], s * 0.12f)), vec(0, 0.55f, 0)), add(add(a, mul(norv[i], s * 0.12f)), vec(0, 0.55f, 0)),
                               add(a, vec(0, 0.9f, 0)), add(b, vec(0, 0.9f, 0)), add(add(b, mul(norv[j], s * 0.12f)), vec(0, 0.9f, 0)), add(add(a, mul(norv[i], s * 0.12f)), vec(0, 0.9f, 0))};
                    hexa(p, rail);
                    if (i % 3 == 0) box(a.x, a.y + 0.45f, a.z, 0.15f, 0.9f, 0.15f, C(110, 110, 115));
                    if (i % 6 == 0 && lk > 0.2f) addGlow(add(a, vec(0, 0.75f, 0)), 0.9f, shade(C(255, 150, 40), lk));
                } else {
                    float t0 = 0.6f * s;
                    V3 p[8] = {a, b, add(b, mul(norv[j], t0)), add(a, mul(norv[i], t0)),
                               add(a, vec(0, 0.95f, 0)), add(b, vec(0, 0.95f, 0)), add(add(b, mul(norv[j], t0 * 0.5f)), vec(0, 0.95f, 0)), add(add(a, mul(norv[i], t0 * 0.5f)), vec(0, 0.95f, 0))};
                    hexa(p, ((i / 5) & 1) ? barrier : C(200, 40, 40));
                }
            }
            if (i % 8 == 0 && curv[i] > 0.0075f) {
                V3 a = tanv[i], b = tanv[(i + 6) % N];
                float turn = a.x * b.z - a.z * b.x, s = turn > 0 ? 1.0f : -1.0f;
                V3 P = add(track[i], mul(norv[i], s * (lim[i] + 0.6f)));
                float yaw = atan2f(norv[i].x, norv[i].z);
                rlPushMatrix(); rlTranslatef(P.x, hi + 1.6f, P.z); rlRotatef(yaw * RAD2DEG, 0, 1, 0);
                box(0, 0, 0, 2.4f, 1.3f, 0.12f, shade(C(255, 210, 0), 0.6f + 0.4f * lk), true);
                for (int k = -1; k <= 1; k++) box(k * 0.7f, 0, -0.08f, 0.22f, 0.9f, 0.05f, C(20, 20, 20), true);
                rlPopMatrix();
            }
        } else if (thm == TH_SNOW || thm == TH_ICE || thm == TH_FJORD) {
            for (int s = -1; s <= 1; s += 2) {
                V3 a = add(track[i], mul(norv[i], s * (lim[i] + 0.4f))), b = add(track[j], mul(norv[j], s * (lim[j] + 0.4f)));
                a.y = hi; b.y = hj;
                if (!shortcuts.empty() && shortcutAt(a, 1.0f) >= 0) continue;
                if (thm == TH_SNOW) {   // plowed snow bank
                    V3 p[8] = {a, b, add(b, mul(norv[j], s * 2.2f)), add(a, mul(norv[i], s * 2.2f)),
                               add(add(a, mul(norv[i], s * 0.5f)), vec(0, 0.9f + 0.3f * hash1(i * 3 + s), 0)), add(add(b, mul(norv[j], s * 0.5f)), vec(0, 0.9f + 0.3f * hash1(j * 3 + s), 0)),
                               add(add(b, mul(norv[j], s * 1.6f)), vec(0, 0.7f, 0)), add(add(a, mul(norv[i], s * 1.6f)), vec(0, 0.7f, 0))};
                    hexa(p, C(240, 245, 252));
                }
                if (thm == TH_FJORD) {  // ice shelf along the water channel
                    V3 p[8] = {add(a, vec(0, -0.8f, 0)), add(b, vec(0, -0.8f, 0)), add(add(b, mul(norv[j], s * 3.0f)), vec(0, -0.8f, 0)), add(add(a, mul(norv[i], s * 3.0f)), vec(0, -0.8f, 0)),
                               add(a, vec(0, 0.7f, 0)), add(b, vec(0, 0.7f, 0)), add(add(b, mul(norv[j], s * 3.0f)), vec(0, 0.7f, 0)), add(add(a, mul(norv[i], s * 3.0f)), vec(0, 0.7f, 0))};
                    hexa(p, C(215, 235, 250));
                }
                if (i % 6 == 0 && thm != TH_FJORD) {   // reflective marker poles
                    bool icey = thm == TH_ICE;
                    box(a.x, a.y + 1.0f, a.z, 0.16f, 2.0f, 0.16f, icey ? C(60, 140, 255) : C(255, 120, 20));
                    box(a.x, a.y + 1.75f, a.z, 0.18f, 0.3f, 0.18f, C(25, 25, 25));
                    if (lk > 0.2f) addGlow(add(a, vec(0, 1.6f, 0)), 0.8f, shade(icey ? C(80, 160, 255) : C(255, 150, 40), lk));
                }
            }
        } else if (thm == TH_DESERT) {
            if (i % 3 == 0) for (int s = -1; s <= 1; s += 2) {   // weathered fence posts with a rope
                V3 a = add(track[i], mul(norv[i], s * (lim[i] + 0.4f))); a.y = hAtLat(i, lim[i] + 0.4f);
                if (!shortcuts.empty() && shortcutAt(a, 1.0f) >= 0) continue;
                box(a.x, a.y + 0.6f, a.z, 0.25f, 1.2f, 0.25f, C(130, 95, 60));
                int j3 = (i + 3) % N; V3 b = add(track[j3], mul(norv[j3], s * (lim[j3] + 0.4f))); b.y = hAtLat(j3, lim[j3] + 0.4f);
                V3 m = mul(add(a, b), 0.5f); float l = dist2D(a, b), ya = atan2f(b.x - a.x, b.z - a.z);
                rlPushMatrix(); rlTranslatef(m.x, m.y + 1.0f, m.z); rlRotatef(ya * RAD2DEG, 0, 1, 0); box(0, 0, 0, 0.06f, 0.06f, l, C(200, 170, 120)); rlPopMatrix();
            }
        } else if (thm == TH_VOLCANO) {
            for (int s = -1; s <= 1; s += 2) {   // jagged basalt wall holding back the lava
                V3 a = add(track[i], mul(norv[i], s * (lim[i] + 0.3f))), b = add(track[j], mul(norv[j], s * (lim[j] + 0.3f)));
                float h0 = 1.0f + 0.9f * hash1(i * 5 + s), h1 = 1.0f + 0.9f * hash1(j * 5 + s);
                V3 p[8] = {a, b, add(b, mul(norv[j], s * 1.6f)), add(a, mul(norv[i], s * 1.6f)),
                           add(a, vec(0, h0, 0)), add(b, vec(0, h1, 0)), add(add(b, mul(norv[j], s * 1.2f)), vec(0, h1 * 0.8f, 0)), add(add(a, mul(norv[i], s * 1.2f)), vec(0, h0 * 0.8f, 0))};
                hexa(p, C(45, 40, 40));
                if (i % 4 == 0) addGlow(add(add(a, mul(norv[i], s * 2.5f)), vec(0, 0.6f, 0)), 3.5f, C(160, 60, 15));
            }
        } else if (thm == TH_STATION) {
            for (int s = -1; s <= 1; s += 2) {   // glass rail with a neon strip
                V3 a = add(track[i], mul(norv[i], s * (lim[i] + 0.3f))), b = add(track[j], mul(norv[j], s * (lim[j] + 0.3f)));
                a.y = hi; b.y = hj;
                V3 p[8] = {a, b, add(b, mul(norv[j], s * 0.2f)), add(a, mul(norv[i], s * 0.2f)),
                           add(a, vec(0, 0.25f, 0)), add(b, vec(0, 0.25f, 0)), add(add(b, mul(norv[j], s * 0.2f)), vec(0, 0.25f, 0)), add(add(a, mul(norv[i], s * 0.2f)), vec(0, 0.25f, 0))};
                hexa(p, ((i / 4) % 2) ? C(0, 200, 255) : C(200, 60, 255), true);
                Color gl = alpha(mixC(C(120, 200, 255), fogCol, gFog), 0.22f);
                texQuad1(texWhite, add(a, vec(0, 0.25f, 0)), add(b, vec(0, 0.25f, 0)), add(b, vec(0, 1.6f, 0)), add(a, vec(0, 1.6f, 0)), v2(0, 0), v2(1, 1), gl);
                if (i % 4 == 0) box(a.x, a.y + 0.9f, a.z, 0.12f, 1.7f, 0.12f, C(180, 185, 200));
                // support struts below the deck
                if (i % 24 == 0) box(a.x - norv[i].x * s * 2, a.y - 20, a.z - norv[i].z * s * 2, 0.8f, 40, 0.8f, C(90, 95, 110));
            }
            if (i % 40 == 0) {   // light arch over the deck
                float yaw = atan2f(tanv[i].x, tanv[i].z), w = lim[i] + 0.5f;
                rlPushMatrix(); rlTranslatef(track[i].x, hi, track[i].z); rlRotatef(yaw * RAD2DEG, 0, 1, 0);
                gLightL = rotY(lightDir, -yaw);
                box(w, 5, 0, 0.8f, 10, 1.2f, C(150, 155, 170)); box(-w, 5, 0, 0.8f, 10, 1.2f, C(150, 155, 170));
                box(0, 10.2f, 0, 2 * w + 0.8f, 0.8f, 1.2f, C(150, 155, 170));
                box(0, 9.7f, -0.62f, 2 * w - 1, 0.25f, 0.05f, C(0, 220, 255), true);
                rlPopMatrix();
                for (int k = -3; k <= 3; k++) addGlow(add(track[i], rotY(vec(k * w / 3.5f, hi + 9.7f, -0.7f), yaw)), 3.0f, C(0, 170, 230));
            }
        } else if (thm == TH_COOLANT) {
            for (int s = -1; s <= 1; s += 2) {
                wallQuad(i, j, (float)s, -1.0f, 3.5f, texPlate, C(170, 175, 190), 4, 4);
                V3 a = add(track[i], mul(norv[i], s * (lim[i] + 0.2f)));
                if (i % 2 == 0) box(a.x, 3.4f, a.z, 0.3f, 0.2f, 2.0f, ((i / 6) % 2) ? C(150, 80, 255) : C(0, 220, 255), true);
            }
        } else if (thm == TH_HARBOR || thm == TH_RIVER || thm == TH_LAGOON) {
            if (thm == TH_LAGOON) continue;
            for (int s = -1; s <= 1; s += 2) {
                V3 a = add(track[i], mul(norv[i], s * lim[i])), b = add(track[j], mul(norv[j], s * lim[j]));
                V3 p[8] = {add(a, vec(0, -1.5f, 0)), add(b, vec(0, -1.5f, 0)), add(add(b, mul(norv[j], s * 1.0f)), vec(0, -1.5f, 0)), add(add(a, mul(norv[i], s * 1.0f)), vec(0, -1.5f, 0)),
                           add(a, vec(0, 0.6f, 0)), add(b, vec(0, 0.6f, 0)), add(add(b, mul(norv[j], s * 1.0f)), vec(0, 0.6f, 0)), add(add(a, mul(norv[i], s * 1.0f)), vec(0, 0.6f, 0))};
                hexa(p, C(150, 148, 140));
                if (i % 10 == 0) box(a.x + norv[i].x * s * 0.8f, 0.9f, a.z + norv[i].z * s * 0.8f, 0.5f, 0.7f, 0.5f, C(60, 60, 64));
            }
        } else if (thm == TH_TUNNEL) {
            Color wall = shade(C(170, 150, 120), 0.75f);
            for (int s = -1; s <= 1; s += 2) {
                wallQuad(i, j, (float)s, 0, 7.5f, texConcrete, wall, 4, 4);
                V3 a = add(track[i], mul(norv[i], s * (lim[i] + 0.3f)));
                if (i % 2 == 0) box(a.x - norv[i].x * s * 0.1f, 1.2f, a.z - norv[i].z * s * 0.1f, 0.3f, 0.25f, 3.0f, C(255, 140, 40), true);
            }
            V3 a = add(track[i], vec(-norv[i].x * (lim[i] + 0.3f), 8.0f, -norv[i].z * (lim[i] + 0.3f))), b = add(track[i], vec(norv[i].x * (lim[i] + 0.3f), 8.0f, norv[i].z * (lim[i] + 0.3f)));
            V3 c = add(track[j], vec(norv[j].x * (lim[j] + 0.3f), 8.0f, norv[j].z * (lim[j] + 0.3f))), d = add(track[j], vec(-norv[j].x * (lim[j] + 0.3f), 8.0f, -norv[j].z * (lim[j] + 0.3f)));
            Color cc = shade(wall, 0.5f);
            texQuad(texConcrete, a, b, c, d, v2(0, 0), v2(4, 0), v2(4, 1), v2(0, 1), cc, cc, cc, cc);
            if (i % 10 == 0) {
                V3 P = track[i]; float yaw = atan2f(tanv[i].x, tanv[i].z);
                rlPushMatrix(); rlTranslatef(P.x, 0, P.z); rlRotatef(yaw * RAD2DEG, 0, 1, 0);
                gLightL = rotY(lightDir, -yaw);
                tbox(0, 8.05f, 0, lim[i] * 2 + 90, 48, 22, lim[i] * 2 + 10, 40, 0, mixC(C(90, 120, 70), C(235, 240, 248), snowK * 0.8f));
                rlPopMatrix();
            }
            if (theme[(i + N - 1) % N] != TH_TUNNEL || theme[j] != TH_TUNNEL) {
                float yaw = atan2f(tanv[i].x, tanv[i].z);
                rlPushMatrix(); rlTranslatef(track[i].x, 0, track[i].z); rlRotatef(yaw * RAD2DEG, 0, 1, 0);
                gLightL = rotY(lightDir, -yaw);
                float w = lim[i] + 0.3f;
                box(w + 1.5f, 6, 0, 3, 12, 2.5f, C(140, 138, 132)); box(-w - 1.5f, 6, 0, 3, 12, 2.5f, C(140, 138, 132));
                box(0, 10, 0, 2 * w + 6, 4, 2.5f, C(140, 138, 132));
                box(0, 9.0f, -1.3f, 2 * w - 2, 0.6f, 0.2f, C(255, 160, 40), true);
                rlPopMatrix();
            }
        } else if (thm == TH_BACKROOMS || thm == TH_POOL) {
            bool pool = thm == TH_POOL;
            const Texture2D& wt = pool ? texTiles : texWallpaper;
            Color wc = pool ? C(170, 190, 198) : C(255, 248, 225);
            for (int s = -1; s <= 1; s += 2) {
                wallQuad(i, j, (float)s, pool ? -1.5f : 0.0f, 6.0f, wt, wc, pool ? 3.0f : 4.0f, pool ? 3.0f : 4.0f);
                // fake doorways into the endless rooms
                if (!pool && hash1(i * 7 + s * 3) < 0.035f) {
                    V3 a = add(track[i], mul(norv[i], s * (lim[i] + 0.25f))), b = add(track[(i + 1) % N], mul(norv[(i + 1) % N], s * (lim[(i + 1) % N] + 0.25f)));
                    V3 m = mul(add(a, b), 0.5f), t = norm(sub(b, a));
                    V3 A = add(m, mul(t, -1.1f)), B = add(m, mul(t, 1.1f));
                    Color dk = mixC(C(26, 22, 10), fogCol, fogF(len(sub(m, camPos))));
                    texQuad1(texWhite, vec(A.x, 0, A.z), vec(B.x, 0, B.z), vec(B.x, 3.0f, B.z), vec(A.x, 3.0f, A.z), v2(0, 0), v2(1, 1), dk);
                }
            }
            V3 a = add(track[i], vec(-norv[i].x * (lim[i] + 0.3f), 6.0f, -norv[i].z * (lim[i] + 0.3f))), b = add(track[i], vec(norv[i].x * (lim[i] + 0.3f), 6.0f, norv[i].z * (lim[i] + 0.3f)));
            V3 c = add(track[j], vec(norv[j].x * (lim[j] + 0.3f), 6.0f, norv[j].z * (lim[j] + 0.3f))), d = add(track[j], vec(-norv[j].x * (lim[j] + 0.3f), 6.0f, -norv[j].z * (lim[j] + 0.3f)));
            float v0 = sArc[i] / 6.0f, v1 = (j == 0 ? trackLen : sArc[j]) / 6.0f;
            Color cc = mixC(pool ? C(150, 168, 175) : C(225, 218, 190), fogCol, fogF(len(sub(a, camPos))));
            texQuad(pool ? texTiles : texCeiling, a, b, c, d, v2(0, v0), v2((lim[i] * 2) / 6.0f, v0), v2((lim[j] * 2) / 6.0f, v1), v2(0, v1), cc, cc, cc, cc);
        }
    }
}
static void drawHarborAndBoards() {
    float lk = lightsK();
    for (const Box& b : boxes) {
        if (!visible(b.p, 7, std::min(drawR, 600.0f))) continue;
        setObj(b.p, b.yaw);
        rlPushMatrix(); rlTranslatef(b.p.x, b.p.y - b.h / 2, b.p.z); rlRotatef(b.yaw * RAD2DEG, 0, 1, 0);
        texBoxLocal(texContainer, b.w / 2, b.d / 2, 0, b.h, b.c, 0, 1, 0, true);
        rlPopMatrix();
    }
    for (const Crane& c : cranes) {
        if (!visible(add(c.p, vec(0, c.h / 2, 0)), c.h, drawR)) continue;
        setObj(add(c.p, vec(0, c.h / 2, 0)), c.yaw);
        rlPushMatrix(); rlTranslatef(c.p.x, 0, c.p.z); rlRotatef(c.yaw * RAD2DEG, 0, 1, 0);
        Color cc = C(230, 120, 30);
        for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) box(sx * 5.0f, c.h / 2, sz * 4.0f, 1.0f, c.h, 1.0f, cc);
        box(0, c.h, 0, 11, 2.2f, 10, cc);
        box(-14, c.h + 2.0f, 0, 50, 1.6f, 2.2f, cc);
        box(8, c.h + 4, 0, 6, 4, 4, C(200, 200, 205));
        box(-30, c.h - 6, 0, 0.2f, 12, 0.2f, C(40, 40, 40));
        rlPopMatrix();
        if (fmodf(gTime, 1.6f) < 0.8f) addGlow(add(c.p, rotY(vec(-38, c.h + 3, 0), c.yaw)), 5, C(255, 40, 40));
        if (lk > 0.2f) addGlow(add(c.p, rotY(vec(0, c.h + 1.5f, 0), c.yaw)), 10, shade(C(255, 200, 120), lk * 0.7f));
    }
    for (const Board& b : boards) {
        if (!visible(add(b.p, vec(0, 12, 0)), 14, drawR)) continue;
        setObj(add(b.p, vec(0, 12, 0)), b.yaw);
        rlPushMatrix(); rlTranslatef(b.p.x, b.p.y, b.p.z); rlRotatef(b.yaw * RAD2DEG, 0, 1, 0);
        box(-6, 5, 0, 0.6f, 10, 0.6f, C(90, 90, 95)); box(6, 5, 0, 0.6f, 10, 0.6f, C(90, 90, 95));
        box(0, 14, 0.15f, 21, 9, 0.3f, C(50, 50, 55));
        Color bc = mixC(litC(WHITE, vec(0, 0, -1)), WHITE, 0.4f + 0.6f * lk); bc = emisC(bc);
        texQuad(texBoard[b.seed], vec(10, 10, -0.05f), vec(-10, 10, -0.05f), vec(-10, 18, -0.05f), vec(10, 18, -0.05f), v2(0, 1), v2(1, 1), v2(1, 0), v2(0, 0), bc, bc, bc, bc);
        texQuad(texBoard[(b.seed + 1) % 3], vec(-10, 10, 0.35f), vec(10, 10, 0.35f), vec(10, 18, 0.35f), vec(-10, 18, 0.35f), v2(0, 1), v2(1, 1), v2(1, 0), v2(0, 0), bc, bc, bc, bc);
        rlPopMatrix();
        if (lk > 0.2f) for (int k = -1; k <= 1; k++) addGlow(add(b.p, rotY(vec(k * 7.0f, 18.8f, -1.0f), b.yaw)), 5, shade(C(255, 250, 230), lk * 0.7f));
    }
}
static void drawGatesRings() {
    for (int gi : gateIdx) {
        if (!visible(track[gi], 30, drawR)) continue;
        Mode nm = zmode[(gi + 3) % N];
        Color gcc = gateColor(nm);
        float w = std::max(hw[gi], 14.0f) + 3.0f, yaw = atan2f(tanv[gi].x, tanv[gi].z);
        float hgt = indoorTrack() ? 5.0f : 16.0f;
        setObj(track[gi], yaw);
        rlPushMatrix(); rlTranslatef(track[gi].x, 0, track[gi].z); rlRotatef(yaw * RAD2DEG, 0, 1, 0);
        float ww = indoorTrack() ? lim[gi] - 0.5f : w;
        for (int s = -1; s <= 1; s += 2) { box(s * ww, hgt / 2, 0, 1.6f, hgt, 1.6f, C(50, 52, 60)); box(s * (ww - 0.9f), hgt / 2, 0, 0.25f, hgt - 1, 1.0f, gcc, true); }
        box(0, hgt + 0.6f, 0, 2 * ww + 1.6f, 1.2f, 1.6f, C(50, 52, 60));
        box(0, hgt - 0.5f, -0.85f, 2 * ww - 2.0f, 0.5f, 0.1f, gcc, true);
        if (nm == M_WATER) for (int k = -3; k <= 3; k++) box(k * 1.4f, hgt + 0.9f + ((k & 1) ? 0.25f : -0.25f), -0.9f, 1.2f, 0.3f, 0.1f, WHITE, true);
        else if (nm == M_AIR) { box(0, hgt + 1.0f, -0.9f, 6, 0.4f, 0.1f, WHITE, true); box(0, hgt + 1.0f, -0.9f, 0.6f, 1.4f, 0.1f, WHITE, true); }
        else box(0, hgt + 1.0f, -0.9f, 1.2f, 1.2f, 0.1f, WHITE, true);
        rlPopMatrix();
        for (int k = -4; k <= 4; k++) addGlow(add(track[gi], rotY(vec(k * ww / 4.5f, hgt + 0.6f, 0), yaw)), 6, shade(gcc, 0.7f));
    }
    if (visible(track[0], 30, drawR)) {
        float yaw = atan2f(tanv[0].x, tanv[0].z), w = hw[0] + 3;
        float hgt = indoorTrack() ? 4.6f : 11.6f;
        setObj(track[0], yaw);
        rlPushMatrix(); rlTranslatef(track[0].x, 0, track[0].z); rlRotatef(yaw * RAD2DEG, 0, 1, 0);
        if (indoorTrack()) w = lim[0] - 0.4f;
        box(w, hgt / 2, 0, 1.4f, hgt, 1.4f, C(60, 60, 66)); box(-w, hgt / 2, 0, 1.4f, hgt, 1.4f, C(60, 60, 66));
        for (int k = 0; k < 16; k++) for (int r = 0; r < 2; r++)
            box(-w + (k + 0.5f) * (2 * w / 16), hgt + r * 1.2f, -0.75f, 2 * w / 16, 1.2f, 0.1f, ((k + r) & 1) ? C(15, 15, 15) : WHITE, true);
        box(0, hgt + 0.6f, 0, 2 * w + 1.4f, 2.6f, 1.4f, C(60, 60, 66));
        rlPopMatrix();
        bool go = countdown <= 1.0f;
        for (int k = 0; k < 5; k++) {
            Color lc = go ? C(40, 255, 80) : (countdown - 1.0f < (5 - k) * 0.6f ? C(255, 40, 30) : C(60, 20, 20));
            addGlow(add(track[0], rotY(vec(-3.0f + k * 1.5f, hgt + 2.6f, -0.8f), yaw)), 3.5f, lc);
        }
    }
    const Racer* pl = vpPlayer < (int)racers.size() ? &racers[vpPlayer] : nullptr;
    for (size_t k = 0; k < rings.size(); k++) {
        const Ring& rg = rings[k];
        if (!visible(rg.p, 12, drawR)) continue;
        bool hit = pl && k < pl->ringHit.size() && pl->ringHit[k];
        Color rc = hit ? C(80, 255, 140, 120) : C(255, 200, 40);
        V3 n = vec(rg.tang.z, 0, -rg.tang.x), up = vec(0, 1, 0);
        float R0 = 9.5f, R1 = 11.5f, spin = gTime * 0.6f;
        gFog = fogF(len(sub(rg.p, camPos)));
        Color ec = emisC(rc), dk = emisC(shade(rc, 0.6f));
        rlCheckRenderBatchLimit(28 * 6);
        rlBegin(RL_TRIANGLES);
        for (int a = 0; a < 28; a++) {
            float t0 = 2 * PI_F * a / 28 + spin, t1 = 2 * PI_F * (a + 1) / 28 + spin;
            V3 d0 = add(mul(n, cosf(t0)), mul(up, sinf(t0))), d1 = add(mul(n, cosf(t1)), mul(up, sinf(t1)));
            V3 i0 = add(rg.p, mul(d0, R0)), o0 = add(rg.p, mul(d0, R1)), i1 = add(rg.p, mul(d1, R0)), o1 = add(rg.p, mul(d1, R1));
            Color cc = (a % 2) ? dk : ec; rlColor4ub(cc.r, cc.g, cc.b, cc.a);
            vtx(i0); vtx(o0); vtx(o1); vtx(i0); vtx(o1); vtx(i1);
        }
        rlEnd();
        if (!hit) for (int a = 0; a < 8; a++) { float t = 2 * PI_F * a / 8 + spin; addGlow(add(rg.p, add(mul(n, cosf(t) * 10.5f), mul(up, sinf(t) * 10.5f))), 5, C(255, 170, 30)); }
    }
}
// tank shortcut barricades and signs
static void drawShortcuts() {
    for (const Shortcut& sc : shortcuts) {
        if (!visible(sc.wallP, 20, 500)) continue;
        setObj(sc.wallP, 0);
        float yaw = atan2f(sc.wdir.x, sc.wdir.z);
        float gy = 0; { int best = sc.a; float bd = 1e9f; for (int k = sc.a; k < sc.a + 40; k++) { float d = dist2D(track[k % N], sc.wallP); if (d < bd) { bd = d; best = k % N; } } gy = th[best]; }
        rlPushMatrix(); rlTranslatef(sc.wallP.x, gy, sc.wallP.z); rlRotatef(yaw * RAD2DEG, 0, 1, 0);
        gLightL = rotY(lightDir, -yaw);
        float xs = dot(vec(cosf(yaw), 0, -sinf(yaw)), sc.wn) > 0 ? 1.0f : -1.0f;   // local x pointing off the road
        if (!sc.open) {
            for (float z = -sc.whl; z < sc.whl - 0.5f; z += 1.6f) {
                int rows = 2 + (hash1((int)(z * 10) + sc.a) > 0.6f ? 1 : 0);
                for (int r = 0; r < rows; r++) {
                    rlPushMatrix(); rlTranslatef(0, r * 1.5f, z + 0.8f); rlRotatef((hash1((int)(z * 7) + r) - 0.5f) * 12, 0, 1, 0);
                    texBoxLocal(texWood, 0.75f, 0.75f, 0, 1.5f, C(255, 235, 200), 1.5f, 1.5f, 0, true);
                    rlPopMatrix();
                }
            }
            box(0, 3.4f, 0, 0.1f, 0.5f, sc.whl * 2, C(255, 200, 0), true);
        } else {
            for (int k = 0; k < 5; k++) {   // smashed crates pushed aside
                float z = (k < 3 ? -sc.whl - 1.5f : sc.whl + 1.5f) + (k % 3) * 0.6f;
                rlPushMatrix(); rlTranslatef(xs * (2.5f + k * 0.5f), 0.4f, z); rlRotatef(30.0f + k * 47, 0.3f, 1, 0.5f);
                texBoxLocal(texWood, 0.75f, 0.75f, -0.75f, 0.75f, C(200, 180, 150), 1.5f, 1.5f, 0, true);
                rlPopMatrix();
            }
        }
        // sign on the road side: a tank silhouette
        rlTranslatef(xs * 1.5f, 0, sc.whl + 2.5f);
        box(0, 1.6f, 0, 0.2f, 3.2f, 0.2f, C(90, 90, 95));
        rlPopMatrix();
        V3 sp = add(add(sc.wallP, mul(sc.wn, 1.5f)), mul(sc.wdir, sc.whl + 2.5f)); sp.y = gy + 3.4f;
        V3 r = mul(sc.wdir, 1.3f), u = vec(0, 1.3f, 0);
        Color sc2 = emisC(C(255, 255, 255));
        texQuad(texTankSign, sub(sub(sp, r), u), add(sub(sp, u), r), add(add(sp, r), u), add(sub(sp, r), u), v2(0, 1), v2(1, 1), v2(1, 0), v2(0, 0), sc2, sc2, sc2, sc2);
        if (!sc.open && lightsK() > 0.2f && fmodf(gTime, 1.0f) < 0.5f) addGlow(add(sc.wallP, vec(0, gy + 3.8f, 0)), 3.0f, C(255, 180, 30));
    }
}
// item boxes, coins, mines, rockets, ramps, destructibles
static void drawKartLayer() {
    for (size_t k = 0; k < itemBoxes.size(); k++) {
        const ItemBox& b = itemBoxes[k];
        float sc = b.t > 0 ? (b.t < 0.5f ? 1.0f - b.t / 0.5f : 0.0f) : 1.0f;
        if (!itemsOn() || sc <= 0.01f || !visible(b.p, 3, 400)) continue;
        setObj(b.p, 0);
        float bob = sinf(gTime * 2.5f + k) * 0.25f;
        Color rc = rainbow(gTime * 2.0f + k * 0.7f);
        rlPushMatrix(); rlTranslatef(b.p.x, b.p.y + bob - 0.9f * sc, b.p.z); rlRotatef(gTime * 70 + k * 30, 0, 1, 0); rlRotatef(20, 1, 0, 1);
        rlScalef(sc, sc, sc);
        texBoxLocal(texQ, 0.9f, 0.9f, 0, 1.8f, rc, 0, 1, 1.0f, false);
        texQuad1(texQ, vec(-0.9f, 1.8f, -0.9f), vec(0.9f, 1.8f, -0.9f), vec(0.9f, 1.8f, 0.9f), vec(-0.9f, 1.8f, 0.9f), v2(0, 0), v2(1, 1), emisC(rc));
        texQuad1(texQ, vec(-0.9f, 0, -0.9f), vec(0.9f, 0, -0.9f), vec(0.9f, 0, 0.9f), vec(-0.9f, 0, 0.9f), v2(0, 0), v2(1, 1), emisC(rc));
        rlPopMatrix();
        addGlow(add(b.p, vec(0, bob, 0)), 4.5f * sc, shade(rc, 0.45f));
    }
    for (size_t k = 0; k < coinsW.size(); k++) {
        const Coin& c = coinsW[k];
        if (c.t > 0 || !visible(c.p, 2, 260)) continue;
        setObj(c.p, 0);
        rlPushMatrix(); rlTranslatef(c.p.x, c.p.y + sinf(gTime * 3 + k) * 0.15f, c.p.z); rlRotatef(gTime * 180 + k * 20, 0, 1, 0); rlRotatef(90, 1, 0, 0);
        Color gold = litC(C(255, 200, 30), vec(0, 0, 1));
        DrawCylinder(vec(0, -0.08f, 0), 0.7f, 0.7f, 0.16f, 12, gold);
        DrawCylinder(vec(0, -0.1f, 0), 0.45f, 0.45f, 0.2f, 10, shade(gold, 1.2f));
        rlPopMatrix();
        addGlow(c.p, 2.0f, C(120, 90, 10));
    }
    for (const Hazard& h : hazards) {
        if (h.type != 1 || !visible(h.p, 2, 300)) continue;
        setObj(h.p, 0);
        ball(add(h.p, vec(0, 0.55f, 0)), 0.55f, C(40, 40, 46));
        for (int k = 0; k < 6; k++) { float a = k * 1.047f; box(h.p.x + cosf(a) * 0.6f, h.p.y + 0.55f, h.p.z + sinf(a) * 0.6f, 0.18f, 0.18f, 0.18f, C(150, 150, 155)); }
        if (fmodf(gTime * 3, 1.0f) < 0.5f) addGlow(add(h.p, vec(0, 1.2f, 0)), 1.6f, C(255, 30, 30));
    }
    for (const Proj& p : rocketsP) {
        if (!visible(p.p, 3, 500)) continue;
        V3 d = norm(p.v);
        float yaw = atan2f(d.x, d.z), pitch = asinf(clampf(d.y, -1, 1));
        setObj(p.p, yaw);
        rlPushMatrix(); rlTranslatef(p.p.x, p.p.y, p.p.z); rlRotatef(yaw * RAD2DEG, 0, 1, 0); rlRotatef(-pitch * RAD2DEG, 1, 0, 0); rlRotatef(gTime * 400, 0, 0, 1);
        box(0, 0, 0, 0.4f, 0.4f, 1.8f, C(220, 40, 40));
        tbox(0, -0.2f, 1.1f, 0.4f, 0.4f, 0.2f, 0.05f, 0.05f, 0.25f, WHITE);
        box(0, 0, -0.8f, 1.0f, 0.08f, 0.4f, C(240, 240, 240)); box(0, 0, -0.8f, 0.08f, 1.0f, 0.4f, C(240, 240, 240));
        rlPopMatrix();
        addGlow(sub(p.p, mul(d, 1.2f)), 2.6f, C(255, 150, 40));
    }
    for (const Ramp& r : ramps) {
        if (!visible(r.p, r.len, 500)) continue;
        setObj(r.p, r.yaw);
        rlPushMatrix(); rlTranslatef(r.p.x, r.p.y, r.p.z); rlRotatef(r.yaw * RAD2DEG, 0, 1, 0);
        float hw2 = r.w / 2, hl = r.len / 2;
        Color top = litC(C(255, 255, 255), norm(vec(0, r.len, -r.h)));
        texQuad(texStripes, vec(-hw2, 0.02f, -hl), vec(hw2, 0.02f, -hl), vec(hw2, r.h, hl), vec(-hw2, r.h, hl), v2(0, 0), v2(r.w / 3, 0), v2(r.w / 3, r.len / 3), v2(0, r.len / 3), top, top, top, top);
        Color side = litC(C(90, 92, 100), vec(1, 0, 0));
        rlCheckRenderBatchLimit(12);
        rlBegin(RL_TRIANGLES);
        rlColor4ub(side.r, side.g, side.b, side.a);
        vtx(vec(hw2, 0, -hl)); vtx(vec(hw2, 0, hl)); vtx(vec(hw2, r.h, hl));
        vtx(vec(-hw2, 0, -hl)); vtx(vec(-hw2, r.h, hl)); vtx(vec(-hw2, 0, hl));
        rlEnd();
        box(0, r.h / 2, hl, r.w, r.h, 0.15f, C(60, 62, 70));
        rlPopMatrix();
        if (lightsK() > 0.2f) { addGlow(add(r.p, rotY(vec(hw2, r.h + 0.2f, hl), r.yaw)), 1.5f, C(255, 200, 40)); addGlow(add(r.p, rotY(vec(-hw2, r.h + 0.2f, hl), r.yaw)), 1.5f, C(255, 200, 40)); }
    }
    for (const Dyn& d : dyns) {
        if (!visible(d.p, 2, 300)) continue;
        setObj(d.p, d.yaw);
        rlPushMatrix(); rlTranslatef(d.p.x, d.p.y, d.p.z); rlRotatef(d.yaw * RAD2DEG, 0, 1, 0); rlRotatef(d.rot * RAD2DEG, 1, 0, 0.3f);
        switch (d.type) {
            case 0: box(0, 0.05f, 0, 0.8f, 0.1f, 0.8f, C(255, 120, 20)); cone(vec(0, 0.1f, 0), 0.35f, 0.9f, 8, C(255, 120, 20));
                    box(0, 0.5f, 0, 0.42f, 0.12f, 0.42f, WHITE); break;
            case 1: {
                Color bc = litC(d.c, vec(1, 0, 0));
                DrawCylinder(vec(0, 0, 0), 0.45f, 0.45f, 1.2f, 10, bc);
                DrawCylinder(vec(0, 0.35f, 0), 0.47f, 0.47f, 0.08f, 10, shade(bc, 0.6f));
                DrawCylinder(vec(0, 0.8f, 0), 0.47f, 0.47f, 0.08f, 10, shade(bc, 0.6f));
            } break;
            case 2: texBoxLocal(texWood, 0.55f, 0.55f, 0, 1.1f, C(255, 235, 200), 1.1f, 1.1f, 0, true); break;
            case 3: ball(vec(0, 0.6f, 0), 0.6f, d.c, WHITE, 4, 8); break;
            default: box(0, 0.4f, 0, 1.0f, 0.8f, 0.8f, C(175, 140, 95)); box(0, 0.81f, 0, 1.02f, 0.02f, 0.15f, C(200, 180, 120)); break;
        }
        rlPopMatrix();
    }
}
static void drawParticles(Camera3D& cam, bool additive) {
    for (const Particle& p : parts) {
        if (p.add != additive) continue;
        float k = p.life / p.maxLife;
        float sz = p.size * (additive ? (0.4f + 0.8f * k) : (0.6f + 1.4f * (1.0f - k)));
        Color c = p.c;
        if (additive) c = shade(c, k);
        else { c = mixC(c, fogCol, fogF(len(sub(p.p, camPos)))); c.a = (unsigned char)(p.c.a * k); }
        DrawBillboard(cam, texGlow, p.p, sz, c);
    }
    if (additive) for (const Shell& s : shells) DrawBillboard(cam, texGlow, s.p, 2.5f, C(255, 200, 90));
}
static void drawLightCones() {
    if (indoorTrack()) return;
    float k = lightsK() * (0.25f + 0.75f * clampf(wetK + (curWeather == W_FOG ? wI : 0) + (curWeather == W_SNOW ? wI * 0.6f : 0), 0, 1));
    if (k < 0.05f) return;
    for (const Lamp& L : lamps) {
        float d = len(sub(L.head, camPos)); if (d > 260 || L.type >= 3) continue;
        float f = k * (1.0f - d / 260.0f) * 0.14f;
        Color top = shade(L.c, f), bot = C(0, 0, 0);
        int sl = 10; float r = 6.0f, h = L.head.y - L.base.y - 0.3f;
        rlCheckRenderBatchLimit(sl * 3);
        rlBegin(RL_TRIANGLES);
        for (int i = 0; i < sl; i++) {
            float a0 = 2 * PI_F * i / sl, a1 = 2 * PI_F * (i + 1) / sl;
            rlColor4ub(top.r, top.g, top.b, 255); vtx(add(L.head, vec(0, -0.3f, 0)));
            rlColor4ub(bot.r, bot.g, bot.b, 255); vtx(vec(L.head.x + cosf(a0) * r, L.head.y - h, L.head.z + sinf(a0) * r));
            vtx(vec(L.head.x + cosf(a1) * r, L.head.y - h, L.head.z + sinf(a1) * r));
        }
        rlEnd();
    }
}

// =====================================================================
//                         VEHICLE MODELS
// =====================================================================
struct Bubble { V3 p; float r; };
static std::vector<Bubble> bubbles;
static V3 worldOf(const Racer& r, V3 local) {
    float cr = cosf(r.roll), sr = sinf(r.roll), cp = cosf(-r.pitch), sp = sinf(-r.pitch);
    V3 a = vec(local.x * cr - local.y * sr, local.x * sr + local.y * cr, local.z);
    V3 b = vec(a.x, a.y * cp - a.z * sp, a.y * sp + a.z * cp);
    return add(r.pos, rotY(b, r.yaw + r.spinAng));
}
// space: glowing anti-grav pads instead of wheels
static void hoverPad(float x, float y, float z, float rad) {
    box(x, y, z, rad * 1.6f, 0.22f, rad * 2.0f, C(70, 75, 90));
    box(x, y - 0.13f, z, rad * 1.3f, 0.05f, rad * 1.7f, shade(C(0, 220, 255), 0.75f + 0.25f * sinf(gTime * 9 + z)), true);
}
static void wheelAt(float x, float y, float z, float rad, float w, float spin) {
    if (rad < 0.05f) return;
    if (spaceNames) { hoverPad(x, y, z, std::max(rad, 0.3f)); return; }
    Color tire = litC(C(28, 28, 30), vec(1, 0, 0)), rim = litC(C(190, 195, 205), vec(1, 0, 0));
    rlPushMatrix(); rlTranslatef(x, y, z); rlRotatef(90, 0, 0, 1); rlRotatef(spin * RAD2DEG, 0, 1, 0);
    DrawCylinder(vec(0, -w / 2, 0), rad, rad, w, 12, tire);
    DrawCylinder(vec(0, -w / 2 - 0.01f, 0), rad * 0.62f, rad * 0.62f, w + 0.02f, 10, rim);
    DrawCube(vec(0, 0, 0), rad * 1.1f, w + 0.04f, rad * 0.14f, shade(rim, 0.6f));
    DrawCube(vec(0, 0, 0), rad * 0.14f, w + 0.04f, rad * 1.1f, shade(rim, 0.6f));
    rlPopMatrix();
}
static void propeller(float x, float y, float z, float size, float m) {
    if (m < 0.3f) return;
    if (spaceNames) {   // starship: twin ion thrusters instead of a propeller
        for (int s = -1; s <= 1; s += 2) {
            box(x + s * 0.45f * size * 0.5f, y, z - 4.2f, 0.45f, 0.45f, 0.9f, C(90, 95, 110));
            box(x + s * 0.45f * size * 0.5f, y, z - 4.7f, 0.32f, 0.32f, 0.1f, C(120, 200, 255), true);
        }
        box(x, y + 0.1f, z + 0.1f, 0.5f, 0.3f, 0.6f, C(120, 200, 255), true);
        return;
    }
    rlPushMatrix(); rlTranslatef(x, y, z); rlRotatef(gTime * 2200.0f, 0, 0, 1);
    box(0, 0, 0, 0.14f, size * m, 0.05f, C(40, 40, 44)); box(0, 0, 0, size * m, 0.14f, 0.05f, C(40, 40, 44));
    rlPopMatrix();
    box(x, y, z + 0.12f, 0.35f, 0.35f, 0.3f, C(220, 220, 225));
}
static void wingSet(float m, float span, float chord, float y, float z, float tailZ, Color wing, Color dark) {
    if (m < 0.02f) return;
    float w = span * m;
    for (int s = -1; s <= 1; s += 2) {
        tbox(s * (1.0f + w * 0.5f), y - 0.07f, z, w, chord, y + 0.07f, w, chord * 0.8f, -0.1f, wing);
        box(s * (1.0f + w), y + 0.25f * m, z - 0.1f, 0.1f, 0.5f * m, chord * 0.6f, dark);
        box(s * (0.4f + 0.7f * m), y + 0.35f * m, tailZ, 1.4f * m, 0.08f, 0.8f * m, wing);
    }
    tbox(0, y, tailZ, 0.12f, 1.2f * m, y + 1.3f * m, 0.1f, 0.6f * m, -0.3f * m, dark);
}
static void driverHead(float x, float y, float z, Color helmet, Color suit, int style = 0, bool crown = false) {
    box(x, y - 0.45f, z, 0.6f, 0.55f, 0.4f, suit);
    switch (style) {
        case 1:   // fox: helmet with ears
            ball(vec(x, y + 0.05f, z), 0.38f, helmet, BLANK, 5, 8);
            for (int s = -1; s <= 1; s += 2) { cone(vec(x + s * 0.2f, y + 0.3f, z), 0.13f, 0.32f, 4, helmet); cone(vec(x + s * 0.2f, y + 0.32f, z + 0.02f), 0.07f, 0.2f, 4, WHITE); }
            box(x, y + 0.05f, z + 0.33f, 0.5f, 0.17f, 0.08f, C(30, 50, 80));
            break;
        case 2:   // robot head
            box(x, y + 0.05f, z, 0.62f, 0.55f, 0.6f, helmet);
            box(x, y + 0.08f, z + 0.31f, 0.46f, 0.12f, 0.03f, C(255, 60, 60), true);
            box(x, y + 0.45f, z, 0.05f, 0.3f, 0.05f, C(200, 200, 210)); ball(vec(x, y + 0.62f, z), 0.07f, C(255, 220, 60), BLANK, 3, 5);
            break;
        case 3:   // helmet + ponytail
            ball(vec(x, y + 0.05f, z), 0.38f, helmet, BLANK, 5, 8);
            box(x, y - 0.05f, z - 0.48f, 0.16f, 0.45f, 0.16f, C(120, 70, 40));
            box(x, y + 0.05f, z + 0.33f, 0.5f, 0.17f, 0.08f, C(30, 50, 80));
            break;
        case 4:   // viking horns
            ball(vec(x, y + 0.05f, z), 0.38f, helmet, BLANK, 5, 8);
            for (int s = -1; s <= 1; s += 2) { rlPushMatrix(); rlTranslatef(x + s * 0.33f, y + 0.2f, z); rlRotatef(-s * 35.0f, 0, 0, 1); cone(vec(0, 0, 0), 0.08f, 0.42f, 5, C(240, 235, 215)); rlPopMatrix(); }
            box(x, y + 0.05f, z + 0.33f, 0.5f, 0.17f, 0.08f, C(30, 50, 80));
            break;
        case 5:   // alien
            ball(vec(x, y + 0.08f, z), 0.4f, C(120, 220, 110), BLANK, 5, 8);
            box(x - 0.13f, y + 0.12f, z + 0.34f, 0.16f, 0.1f, 0.04f, C(20, 20, 20)); box(x + 0.13f, y + 0.12f, z + 0.34f, 0.16f, 0.1f, 0.04f, C(20, 20, 20));
            for (int s = -1; s <= 1; s += 2) { box(x + s * 0.15f, y + 0.5f, z, 0.04f, 0.3f, 0.04f, C(120, 220, 110)); ball(vec(x + s * 0.15f, y + 0.68f, z), 0.07f, helmet, BLANK, 3, 5); }
            break;
        default:
            ball(vec(x, y + 0.05f, z), 0.38f, helmet, BLANK, 5, 8);
            box(x, y + 0.05f, z + 0.33f, 0.5f, 0.17f, 0.08f, C(30, 50, 80));
            box(x, y + 0.3f, z, 0.08f, 0.1f, 0.7f, WHITE);
            break;
    }
    if (crown) {
        Color g = C(255, 200, 40);
        box(x, y + 0.48f, z, 0.5f, 0.12f, 0.5f, g);
        for (int k = 0; k < 4; k++) { float a = k * PI_F / 2 + PI_F / 4; cone(vec(x + cosf(a) * 0.2f, y + 0.54f, z + sinf(a) * 0.2f), 0.08f, 0.22f, 4, g); }
        box(x, y + 0.5f, z + 0.26f, 0.08f, 0.08f, 0.02f, C(255, 40, 60), true);
    }
}
// ------------------------------ decals ------------------------------
// seven-segment digit drawn as thin boxes on a plane facing +x (s = +1) or -x (s = -1), or +z (s = 0)
static void segDigit(float x, float y, float z, int d, float h, int s, Color c) {
    static const unsigned char SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    unsigned char m = SEG[d % 10];
    float w = h * 0.55f, t = h * 0.13f;
    struct Sg { float u, v, lu, lv; } sg[7] = {{0, h / 2, w, t}, {w / 2, h / 4, t, h / 2}, {w / 2, -h / 4, t, h / 2}, {0, -h / 2, w, t}, {-w / 2, -h / 4, t, h / 2}, {-w / 2, h / 4, t, h / 2}, {0, 0, w, t}};
    for (int k = 0; k < 7; k++) {
        if (!(m & (1 << k))) continue;
        if (s == 0) box(x + sg[k].u, y + sg[k].v, z, sg[k].lu, sg[k].lv, 0.02f, c, true);
        else box(x, y + sg[k].v, z - s * sg[k].u, 0.02f, sg[k].lv, sg[k].lu, c, true);
    }
}
static void numberPlate(float x, float y, float z, int num, float h, int s) {
    num = ((num % 100) + 100) % 100;
    if (s == 0) box(x, y, z - 0.01f, h * 1.6f, h * 1.3f, 0.02f, WHITE, true);
    else box(x - s * 0.01f, y, z, 0.02f, h * 1.3f, h * 1.6f, WHITE, true);
    if (num >= 10) { segDigit(x + (s == 0 ? -h * 0.38f : 0), y, z + (s == 0 ? 0.01f : s * h * 0.38f), num / 10, h * 0.85f, s, C(20, 20, 24)); segDigit(x + (s == 0 ? h * 0.38f : 0), y, z + (s == 0 ? 0.01f : -s * h * 0.38f), num % 10, h * 0.85f, s, C(20, 20, 24)); }
    else segDigit(x, y, z + (s == 0 ? 0.01f : 0), num, h * 0.85f, s, C(20, 20, 24));
}
// flame / bolt / magma side graphics on a panel at x = xs facing outwards
static void sideArt(float xs, float y0, float z0, float len, float hgt, int kind, Color c2) {
    float sx = xs + sgn(xs) * 0.012f;
    if (kind == 2 || kind == 6) {
        bool magma = kind == 6;
        float pulse = 0.75f + 0.25f * sinf(gTime * 6 + xs);
        Color c0 = magma ? shade(C(255, 90, 20), pulse) : C(255, 120, 20), c1 = magma ? shade(C(255, 220, 80), pulse) : C(255, 215, 40);
        for (int k = 0; k < 4; k++) {
            float yy = y0 + hgt * (0.15f + 0.22f * k), tl = len * (0.55f + 0.45f * hash1(k * 7 + 3));
            V3 a = vec(sx, yy - hgt * 0.12f, z0), b = vec(sx, yy + hgt * 0.12f, z0), tip = vec(sx, yy + hgt * 0.08f * (k % 2 ? 1 : -1), z0 - tl);
            if (magma) {
                rlCheckRenderBatchLimit(3); rlBegin(RL_TRIANGLES); Color k1 = emisC(k % 2 ? c0 : c1); rlColor4ub(k1.r, k1.g, k1.b, k1.a); vtx(a); vtx(b); vtx(tip); rlEnd();
            } else tri2(a, b, tip, k % 2 ? c0 : c1);
        }
    } else if (kind == 5) {   // lightning bolt
        float zc = z0 - len * 0.5f, yc = y0 + hgt * 0.5f;
        tri2(vec(sx, yc + hgt * 0.45f, zc + len * 0.25f), vec(sx, yc - hgt * 0.05f, zc - len * 0.05f), vec(sx, yc + hgt * 0.1f, zc + len * 0.1f), c2);
        tri2(vec(sx, yc + hgt * 0.1f, zc - len * 0.15f), vec(sx, yc - hgt * 0.45f, zc - len * 0.3f), vec(sx, yc - hgt * 0.05f, zc + len * 0.05f), c2);
        tri2(vec(sx, yc - hgt * 0.05f, zc - len * 0.05f), vec(sx, yc + hgt * 0.1f, zc - len * 0.15f), vec(sx, yc + hgt * 0.1f, zc + len * 0.1f), c2);
    }
}
static void drawCarDecal(const Racer& r) {
    Color c2 = r.color2;
    switch (r.decal) {
        case 1:   // racing stripes over hood, roof and trunk
            for (int s = -1; s <= 1; s += 2) {
                box(s * 0.22f, 0.965f, 1.35f, 0.24f, 0.02f, 1.45f, c2); box(s * 0.22f, 1.435f, -0.5f, 0.24f, 0.02f, 1.15f, c2);
                box(s * 0.22f, 1.012f, -1.75f, 0.24f, 0.02f, 0.85f, c2);
            }
            break;
        case 2: case 5: case 6: for (int s = -1; s <= 1; s += 2) sideArt(s * 0.995f, 0.32f, 1.95f, 2.4f, 0.45f, r.decal, c2); break;
        case 3: for (int s = -1; s <= 1; s += 2) numberPlate(s * 1.0f, 0.56f, -0.15f, r.number, 0.42f, s); box(0, 0.97f, 1.6f, 0.9f, 0.02f, 0.5f, c2); break;
        case 4: for (int a = 0; a < 6; a++) for (int b = 0; b < 4; b++) if ((a + b) & 1) box(-0.75f + a * 0.3f, 0.965f, 0.75f + b * 0.3f, 0.3f, 0.02f, 0.3f, c2); break;
        case 7: {
            Color g = C(255, 200, 40);
            box(0, 1.5f, -0.5f, 0.9f, 0.14f, 0.9f, g);
            for (int k = 0; k < 5; k++) { float a = k * 2 * PI_F / 5; cone(vec(cosf(a) * 0.38f, 1.56f, -0.5f + sinf(a) * 0.38f), 0.12f, 0.38f, 4, g); }
            box(0, 1.55f, -0.04f, 0.14f, 0.14f, 0.04f, C(255, 40, 60), true);
        } break;
        default: break;
    }
}
static void drawCar(const Racer& r) {
    bool cop = r.isCop;
    Color col = cop ? C(22, 24, 30) : r.color, dark = C(25, 26, 30), glass = C(25, 35, 50);
    float ma = r.morphAir, mw = r.morphWater, mm = std::max(ma, mw);
    tbox(0, 0.28f, 0, 2.0f, 4.5f, 0.82f, 1.98f, 4.4f, 0, col);
    tbox(0, 0.82f, 1.35f, 1.9f, 1.75f, 0.95f, 1.75f, 1.5f, -0.1f, col);
    tbox(0, 0.82f, -0.3f, 1.8f, 2.4f, 1.38f, 1.38f, 1.2f, -0.2f, glass);
    tbox(0, 1.36f, -0.5f, 1.4f, 1.24f, 1.42f, 1.26f, 1.0f, 0, cop ? WHITE : col);
    tbox(0, 0.82f, -1.75f, 1.92f, 1.0f, 1.0f, 1.8f, 0.85f, 0.05f, col);
    box(0, 0.24f, 0, 2.06f, 0.18f, 4.3f, dark);
    if (r.hp > 60) box(0, 0.35f, 2.2f, 1.95f, 0.3f, 0.25f, dark);
    box(0, 0.4f, -2.2f, 1.9f, 0.35f, 0.2f, dark);
    if (r.hp > 35 && !cop) {
        box(0, 1.3f, -2.15f, 2.0f, 0.08f, 0.45f, dark);
        box(-0.65f, 1.1f, -2.1f, 0.08f, 0.35f, 0.15f, dark); box(0.65f, 1.1f, -2.1f, 0.08f, 0.35f, 0.15f, dark);
        box(0, 0.9f, 2.0f, 0.12f, 0.02f, 0.9f, shade(col, 0.6f));
    }
    if (cop) {
        box(1.0f, 0.62f, 0.0f, 0.06f, 0.5f, 1.8f, WHITE); box(-1.0f, 0.62f, 0.0f, 0.06f, 0.5f, 1.8f, WHITE);
        box(0, 1.5f, -0.45f, 1.2f, 0.12f, 0.35f, dark);
        bool ph = fmodf(gTime * 3, 1.0f) < 0.5f;
        box(0.35f, 1.6f, -0.45f, 0.45f, 0.14f, 0.3f, ph ? C(255, 40, 40) : C(90, 10, 10), true);
        box(-0.35f, 1.6f, -0.45f, 0.45f, 0.14f, 0.3f, ph ? C(20, 30, 100) : C(40, 90, 255), true);
    }
    if (r.hp < 60) { box(0.4f, 0.97f, 1.4f, 0.6f, 0.02f, 0.5f, C(25, 25, 25)); box(-0.7f, 0.6f, -1.0f, 0.05f, 0.3f, 0.7f, C(30, 30, 30)); }
    box(-0.68f, 0.7f, 2.23f, 0.45f, 0.14f, 0.06f, C(255, 250, 230), true);
    box(0.68f, 0.7f, 2.23f, 0.45f, 0.14f, 0.06f, C(255, 250, 230), true);
    Color tl = r.in.throttle < 0 ? C(255, 60, 60) : C(200, 20, 25);
    box(0, 0.78f, -2.26f, 1.7f, 0.1f, 0.05f, tl, true);
    float wr = 0.40f * (1.0f - 0.85f * mm), wy = 0.40f + 0.5f * mm;
    float st = r.steerV * 0.45f;
    for (int sx = -1; sx <= 1; sx += 2) for (int sz = -1; sz <= 1; sz += 2) {
        if (sz > 0) { rlPushMatrix(); rlTranslatef(sx * 0.95f, wy, 1.45f); rlRotatef(-st * RAD2DEG, 0, 1, 0); wheelAt(0, 0, 0, wr, 0.32f, r.wheelRot); rlPopMatrix(); }
        else wheelAt(sx * 0.95f, wy, -1.4f, wr, 0.34f, r.wheelRot);
    }
    if (mw > 0.02f) {
        tbox(0, -0.05f - 0.1f * (1 - mw), 0, 1.6f * mw + 1.9f * (1 - mw), 4.6f, 0.32f, 2.3f * mw + 1.9f * (1 - mw), 4.9f, 0.15f, shade(col, 0.7f));
        tbox(0, 0.0f, 2.6f, 0.4f, 0.4f, 0.32f, 1.6f * mw, 0.9f * mw, -0.2f, shade(col, 0.7f));
        box(0, 0.4f, -2.5f, 0.35f, 0.9f * mw, 0.3f, dark);
    }
    if (!cop) drawCarDecal(r);
    wingSet(ma, 3.8f, 1.4f, 0.92f, -0.2f, -2.1f, mixC(col, WHITE, 0.6f), dark);
    propeller(0, 0.72f, 2.35f, 2.0f, ma);
}
static void drawKart(const Racer& r) {
    Color col = r.color, dark = C(30, 30, 34), metal = C(170, 172, 180);
    float ma = r.morphAir, mw = r.morphWater, mm = std::max(ma, mw);
    tbox(0, 0.16f, 0, 1.5f, 2.6f, 0.42f, 1.3f, 2.3f, 0, col);
    tbox(0, 0.18f, 1.45f, 1.1f, 0.7f, 0.42f, 0.55f, 0.5f, 0.15f, col);
    box(0, 0.25f, 1.82f, 1.5f, 0.16f, 0.16f, dark);
    box(0, 0.3f, -1.45f, 1.7f, 0.22f, 0.2f, dark);
    for (int s = -1; s <= 1; s += 2) box(s * 0.85f, 0.32f, 0.05f, 0.32f, 0.32f, 1.4f, shade(col, 0.8f));
    box(0, 0.6f, -0.45f, 0.72f, 0.45f, 0.62f, dark);
    box(0, 0.98f, -0.78f, 0.7f, 0.65f, 0.14f, dark);
    box(0, 0.62f, -1.15f, 0.8f, 0.5f, 0.5f, C(70, 70, 76));
    for (int s = -1; s <= 1; s += 2) box(s * 0.25f, 0.78f, -1.48f, 0.12f, 0.12f, 0.45f, metal);
    box(0, 0.75f, 0.45f, 0.08f, 0.08f, 0.6f, dark);
    rlPushMatrix(); rlTranslatef(0, 0.98f, 0.18f); rlRotatef(-25, 1, 0, 0); rlRotatef(r.steerV * 50, 0, 0, 1);
    box(0, 0, 0, 0.55f, 0.07f, 0.07f, dark); box(0, 0, 0, 0.07f, 0.4f, 0.07f, dark);
    rlPopMatrix();
    numberPlate(0, 0.45f, 1.86f, r.number, 0.24f, 0);
    driverHead(0, 1.55f, -0.38f, r.helmet, r.color, r.driverStyle, r.decal == 7);
    if (r.decal == 1) for (int s = -1; s <= 1; s += 2) box(s * 0.18f, 0.43f, 0.6f, 0.16f, 0.02f, 2.0f, r.color2);
    if (r.decal == 2 || r.decal == 5 || r.decal == 6) for (int s = -1; s <= 1; s += 2) sideArt(s * 1.01f, 0.18f, 0.7f, 1.3f, 0.3f, r.decal, r.color2);
    if (r.decal == 4) for (int a = 0; a < 4; a++) for (int b = 0; b < 3; b++) if ((a + b) & 1) box(-0.39f + a * 0.26f, 0.43f, -0.1f + b * 0.26f, 0.26f, 0.02f, 0.26f, r.color2);
    for (int s = -1; s <= 1; s += 2) { rlPushMatrix(); rlTranslatef(s * 0.3f, 1.25f, -0.25f); rlRotatef(28, 1, 0, 0); rlRotatef(-s * 12.0f, 0, 1, 0); box(0, 0, 0.3f, 0.15f, 0.15f, 0.62f, r.color); rlPopMatrix(); }
    float fw = 0.30f * (1 - 0.8f * mm), rw = 0.37f * (1 - 0.8f * mm);
    for (int s = -1; s <= 1; s += 2) {
        rlPushMatrix(); rlTranslatef(s * 0.88f, 0.3f + 0.3f * mm, 1.05f); rlRotatef(-r.steerV * 25, 0, 1, 0); wheelAt(0, 0, 0, fw, 0.3f, r.wheelRot); rlPopMatrix();
        wheelAt(s * 0.92f, 0.37f + 0.3f * mm, -1.0f, rw, 0.42f, r.wheelRot);
    }
    if (mw > 0.02f) tbox(0, -0.2f, 0.1f, 0.9f * mw, 2.4f, 0.18f, 1.6f * mw, 3.0f, 0.3f, shade(col, 0.75f));
    wingSet(ma, 3.0f, 1.1f, 0.62f, -0.1f, -1.4f, mixC(col, WHITE, 0.6f), dark);
    propeller(0, 0.45f, 1.95f, 1.6f, ma);
}
static void drawTank(const Racer& r) {
    Color col = mixC(r.color, C(90, 100, 70), 0.35f), dark = shade(col, 0.55f), light = shade(col, 1.15f);
    float ma = r.morphAir, mw = r.morphWater;
    for (int s = -1; s <= 1; s += 2) {
        tbox(s * 1.75f, 0.1f, 0, 0.95f, 5.0f, 1.15f, 0.95f, 5.7f, 0, C(40, 41, 44));
        for (int k = -2; k <= 2; k++) wheelAt(s * 2.25f, 0.55f, k * 1.05f, 0.45f, 0.18f, r.wheelRot);
        box(s * 1.75f, 1.2f, 0, 1.1f, 0.12f, 5.6f, dark);
    }
    tbox(0, 0.7f, 0, 2.5f, 5.0f, 1.55f, 2.3f, 4.2f, -0.2f, col);
    tbox(0, 1.1f, 2.4f, 2.4f, 0.3f, 1.55f, 2.2f, 0.9f, -0.35f, light);
    tbox(0, 1.55f, -0.3f, 2.1f, 2.4f, 2.35f, 1.7f, 1.8f, -0.1f, light);
    box(0.55f, 2.5f, -0.7f, 0.5f, 0.3f, 0.5f, dark);
    driverHead(-0.45f, 2.75f, -0.9f, r.helmet, C(60, 70, 50), r.driverStyle, r.decal == 7);
    if (r.decal == 3) for (int s = -1; s <= 1; s += 2) numberPlate(s * 1.0f, 1.85f, -0.3f, r.number, 0.5f, s);
    if (r.decal == 1 || r.decal == 4) box(0, 1.56f, 1.2f, 0.5f, 0.02f, 2.0f, r.color2);
    if (r.decal == 2 || r.decal == 5 || r.decal == 6) for (int s = -1; s <= 1; s += 2) sideArt(s * 1.26f, 0.75f, 2.2f, 2.8f, 0.7f, r.decal, r.color2);
    float recoil = clampf((r.specialCd - 0.7f) * 2.0f, 0, 1) * 0.6f;
    box(0, 2.0f, 2.3f - recoil, 0.28f, 0.28f, 3.4f, dark);
    box(0, 2.0f, 4.1f - recoil, 0.42f, 0.42f, 0.35f, C(30, 30, 30));
    box(-0.9f, 1.35f, 2.55f, 0.35f, 0.2f, 0.1f, C(255, 245, 220), true); box(0.9f, 1.35f, 2.55f, 0.35f, 0.2f, 0.1f, C(255, 245, 220), true);
    if (mw > 0.02f) for (int s = -1; s <= 1; s += 2) tbox(s * 2.2f, -0.3f, 0, 0.6f * mw, 5.0f, 0.4f, 0.9f * mw, 5.6f, 0.2f, shade(col, 0.6f));
    wingSet(ma, 5.5f, 2.2f, 1.4f, -0.2f, -2.8f, shade(col, 1.1f), dark);
    if (ma > 0.3f) for (int s = -1; s <= 1; s += 2) propeller(s * (1.8f + 1.4f * ma), 1.4f, 1.3f, 2.4f, ma);
}
static void drawRobot(const Racer& r) {
    Color col = r.color, dark = C(45, 48, 56), light = mixC(col, WHITE, 0.3f);
    float ma = r.morphAir, mw = r.morphWater;
    float spd = clampf(len(vec(r.vel.x, 0, r.vel.z)) / 18.0f, 0, 1);
    float sw = sinf(r.walk * 1.6f) * 38.0f * spd;
    if (!r.grounded && r.mode == M_LAND) sw = 35.0f;
    sw = lerpf(sw, -55.0f, ma);
    rlPushMatrix();
    rlTranslatef(0, 0.25f * mw - 1.6f * ma, 0);
    tbox(0, 2.0f, 0, 1.2f, 0.9f, 3.4f, 1.7f, 1.1f, 0.05f, col);
    box(0, 2.9f, 0.56f, 1.1f, 0.6f, 0.08f, light);
    box(0, 2.9f, 0.62f, 0.5f, 0.25f, 0.05f, C(80, 220, 255), true);
    if (r.decal == 3) numberPlate(0, 2.35f, 0.58f, r.number, 0.4f, 0);
    else if (r.decal != 0 && r.decal != 7) box(0, 2.35f, 0.57f, 1.15f, 0.18f, 0.03f, r.decal == 6 ? shade(C(255, 110, 20), 0.8f + 0.2f * sinf(gTime * 6)) : r.color2, r.decal == 6);
    if (r.decal == 7) { Color g = C(255, 200, 40); box(0, 4.05f, 0.1f, 0.7f, 0.14f, 0.7f, g); for (int k = 0; k < 4; k++) { float a = k * PI_F / 2 + PI_F / 4; cone(vec(cosf(a) * 0.28f, 4.1f, 0.1f + sinf(a) * 0.28f), 0.1f, 0.3f, 4, g); } }
    box(0, 1.9f, 0, 1.1f, 0.4f, 0.8f, dark);
    tbox(0, 3.4f, 0.1f, 0.8f, 0.8f, 4.0f, 0.7f, 0.75f, 0.05f, dark);
    box(0, 3.72f, 0.5f, 0.66f, 0.18f, 0.05f, C(255, 60, 60), true);
    box(0.3f, 4.25f, 0, 0.06f, 0.5f, 0.06f, C(200, 200, 200));
    box(0, 2.8f, -0.7f, 1.0f, 1.1f, 0.4f, dark);
    for (int s = -1; s <= 1; s += 2) {
        box(s * 1.05f, 3.2f, 0, 0.7f, 0.6f, 0.75f, col);
        rlPushMatrix(); rlTranslatef(s * 1.05f, 3.1f, 0); rlRotatef(-sw * s * 0.9f, 1, 0, 0); rlRotatef(s * 88.0f * ma, 0, 0, 1);
        box(0, -0.65f, 0, 0.4f, 1.3f, 0.4f, dark); box(0, -1.4f, 0.05f, 0.5f, 0.45f, 0.5f, col);
        rlPopMatrix();
        rlPushMatrix(); rlTranslatef(s * 0.5f, 1.9f, 0); rlRotatef(sw * s, 1, 0, 0);
        box(0, -0.55f, 0, 0.5f, 1.1f, 0.55f, dark);
        rlTranslatef(0, -1.1f, 0); rlRotatef(fabsf(sw) * 0.5f, 1, 0, 0);
        box(0, -0.5f, 0.05f, 0.46f, 1.0f, 0.5f, col); box(0, -1.0f, 0.25f, 0.6f, 0.2f, 0.9f, dark);
        rlPopMatrix();
    }
    if (mw > 0.02f) tbox(0, -0.1f, 0, 1.2f * mw, 3.0f * mw, 0.15f, 1.9f * mw, 3.5f * mw, 0, shade(col, 0.6f));
    wingSet(ma, 3.4f, 1.1f, 2.7f, -0.1f, -1.6f, mixC(col, WHITE, 0.6f), dark);
    if (ma > 0.3f) propeller(0, 2.6f, 0.9f, 1.8f, ma);
    rlPopMatrix();
}
static void drawVehicle(const Racer& r, bool lights = true) {
    if (r.dead || r.out) return;
    if (r.invuln > 0 && gAlpha == 255 && ((int)(gTime * 12)) % 2 == 0) return;
    if (!visible(r.pos, 8, drawR)) return;
    setObj(r.pos, r.yaw + r.spinAng);
    float trick = r.trickT > 0 ? (1.0f - r.trickT / 0.55f) * 360.0f : 0.0f;
    float sq = r.squashT > 0 ? sinf(r.squashT / 0.3f * PI_F) * 0.25f : 0.0f;
    float sc = (r.shrinkT > 0 ? 0.6f : 1.0f) * (r.boss ? 1.22f : 1.0f);
    float hover = spaceNames && r.mode == M_LAND && (r.base == B_CAR || r.base == B_KART) ? 0.45f + 0.08f * sinf(gTime * 4 + r.id) : 0.0f;
    rlPushMatrix();
    rlTranslatef(r.pos.x, r.pos.y + hover, r.pos.z);
    rlRotatef((r.yaw + r.spinAng) * RAD2DEG, 0, 1, 0); rlRotatef(-r.pitch * RAD2DEG, 1, 0, 0); rlRotatef(r.roll * RAD2DEG + (r.base == B_ROBOT ? 0.0f : trick), 0, 0, 1);
    if (r.base == B_ROBOT && trick > 0) { rlTranslatef(0, 2, 0); rlRotatef(-trick, 1, 0, 0); rlTranslatef(0, -2, 0); }
    rlScalef(sc * (1 + sq * 0.6f), sc * (1 - sq), sc * (1 + sq * 0.6f));
    switch (r.base) { case B_CAR: drawCar(r); break; case B_TANK: drawTank(r); break; case B_ROBOT: drawRobot(r); break; default: drawKart(r); break; }
    rlPopMatrix();
    if (r.shieldT > 0) bubbles.push_back({add(r.pos, vec(0, 1.0f * sc, 0)), (radiusOf(r) + 0.8f)});
    // battle balloons on strings
    if (S.mode == GM_BATTLE && r.balloons > 0 && !r.isCop) {
        float h = r.mode == M_AIR ? 2.5f : r.base == B_TANK ? 4.6f : r.base == B_ROBOT ? 5.8f : r.base == B_KART ? 3.0f : 3.2f;
        V3 attach = add(r.pos, vec(0, h - 1.6f, 0));
        for (int k = 0; k < r.balloons; k++) {
            float a = k * 2.094f + r.yaw + sinf(gTime * 1.3f + r.id) * 0.3f;
            V3 bp = add(r.pos, vec(cosf(a) * 0.9f, h + 0.25f * sinf(gTime * 2.2f + k + r.id), sinf(a) * 0.9f));
            setObj(bp, 0);
            DrawLine3D(attach, bp, C(240, 240, 240, 200));
            ball(bp, 0.6f, balloonCol(k), BLANK, 5, 8);
            box(bp.x, bp.y - 0.62f, bp.z, 0.12f, 0.12f, 0.12f, balloonCol(k));
        }
    }
    if (r.boss) {   // menacing aura
        float p = 0.6f + 0.4f * sinf(gTime * 3);
        addGlow(add(r.pos, vec(0, 1.0f, 0)), 9.0f * p, shade(r.color2, 0.35f));
    }
    if (hover > 0) addGlow(add(r.pos, vec(0, 0.15f, 0)), 2.4f, shade(C(0, 200, 255), 0.22f));
    if (!lights) return;
    float lk = lightsK();
    float hk = 0.35f + 0.65f * lk;
    if (r.isCop) {
        bool ph = fmodf(gTime * 3, 1.0f) < 0.5f;
        addGlow(worldOf(r, vec(0.35f, 1.65f, -0.45f)), ph ? 5.0f : 2.0f, C(255, 40, 40)); addGlow(worldOf(r, vec(-0.35f, 1.65f, -0.45f)), ph ? 2.0f : 5.0f, C(40, 90, 255));
    }
    if (r.mode != M_AIR) {
        if (r.base == B_CAR || r.base == B_KART) {
            float z = r.base == B_CAR ? 2.3f : 1.9f, x = r.base == B_CAR ? 0.68f : 0.4f, y = r.base == B_CAR ? 0.7f : 0.4f;
            addGlow(worldOf(r, vec(x, y, z)), 2.6f * hk, shade(C(255, 245, 220), hk)); addGlow(worldOf(r, vec(-x, y, z)), 2.6f * hk, shade(C(255, 245, 220), hk));
            float bk = r.in.throttle < 0 ? 1.0f : 0.45f;
            float tz = r.base == B_CAR ? -2.3f : -1.55f;
            addGlow(worldOf(r, vec(0.7f, 0.78f, tz)), 1.8f, shade(C(255, 30, 30), bk)); addGlow(worldOf(r, vec(-0.7f, 0.78f, tz)), 1.8f, shade(C(255, 30, 30), bk));
        } else if (r.base == B_TANK) {
            addGlow(worldOf(r, vec(0.9f, 1.35f, 2.6f)), 2.4f * hk, shade(C(255, 245, 220), hk)); addGlow(worldOf(r, vec(-0.9f, 1.35f, 2.6f)), 2.4f * hk, shade(C(255, 245, 220), hk));
        } else {
            addGlow(worldOf(r, vec(0, 3.72f, 0.6f)), 2.0f, C(255, 70, 70)); addGlow(worldOf(r, vec(0, 2.9f, 0.7f)), 1.6f, C(80, 220, 255));
        }
        if (r.boosting || r.boostT > 0) {
            float y = r.base == B_TANK ? 1.3f : r.base == B_ROBOT ? 2.6f : r.base == B_KART ? 0.78f : 0.5f, z = -radiusOf(r) - 0.2f;
            Color fc = r.boostT > 0 && !r.boosting ? C(255, 140, 40) : C(90, 160, 255);
            addGlow(worldOf(r, vec(0.45f, y, z)), 2.5f + frand(0, 1), fc); addGlow(worldOf(r, vec(-0.45f, y, z)), 2.5f + frand(0, 1), fc);
        }
        if (r.driftCharge > 0.8f) {
            Color dc = driftColor(driftStage(r));
            addGlow(worldOf(r, vec(0.9f, 0.3f, -1.3f)), 1.6f + frand(0, 0.6f), dc); addGlow(worldOf(r, vec(-0.9f, 0.3f, -1.3f)), 1.6f + frand(0, 0.6f), dc);
        }
    } else {
        float span = (r.base == B_TANK ? 7.5f : r.base == B_ROBOT ? 4.5f : r.base == B_KART ? 4.0f : 4.8f) * r.morphAir;
        float y = r.base == B_TANK ? 1.4f : r.base == B_ROBOT ? 1.1f : r.base == B_KART ? 0.62f : 0.92f;
        addGlow(worldOf(r, vec(span, y, -0.2f)), 2.0f, C(40, 255, 80)); addGlow(worldOf(r, vec(-span, y, -0.2f)), 2.0f, C(255, 40, 40));
        if (fmodf(gTime + r.id * 0.3f, 1.2f) < 0.08f) addGlow(worldOf(r, vec(0, y + 1.0f, -2.0f)), 4.0f, WHITE);
        if (boostK(r) > 0) addGlow(worldOf(r, vec(0, y, -radiusOf(r))), 4.0f + frand(0, 1.5f), C(255, 150, 60));
    }
    if (r.shrinkT > 0 && fmodf(gTime * 4, 1.0f) < 0.15f) addGlow(add(r.pos, vec(0, 1.5f, 0)), 3.0f, C(255, 240, 120));
}
static void drawTraffic() {
    float lk = lightsK();
    for (const Traffic& t : traffic) {
        if (t.dead || !visible(t.pos, 8, std::min(drawR, 600.0f))) continue;
        setObj(t.pos, t.yaw);
        rlPushMatrix(); rlTranslatef(t.pos.x, t.pos.y, t.pos.z); rlRotatef(t.yaw * RAD2DEG, 0, 1, 0);
        Color c = t.wreck ? shade(t.c, 0.6f) : t.c, glass = C(30, 40, 55), dark = C(30, 30, 34);
        float L = t.len, W = t.wid;
        switch (t.type) {
            case 0:
                tbox(0, 0.3f, 0, W, L, 0.95f, W * 0.97f, L * 0.97f, 0, c);
                tbox(0, 0.95f, -0.2f, W * 0.9f, L * 0.5f, 1.45f, W * 0.75f, L * 0.32f, -0.1f, glass);
                break;
            case 1:
                box(0, 1.25f, -0.3f, W, 2.0f, L - 0.8f, c);
                tbox(0, 0.3f, L / 2 - 0.6f, W, 1.2f, 1.3f, W * 0.95f, 0.9f, -0.15f, c);
                box(0, 1.5f, L / 2 - 0.62f, W * 0.85f, 0.6f, 0.4f, glass);
                break;
            case 2:
                box(0, 1.5f, L / 2 - 1.2f, W, 2.4f, 2.2f, c);
                box(0, 2.0f, L / 2 - 0.12f, W * 0.85f, 0.8f, 0.1f, glass);
                rlPushMatrix(); rlTranslatef(0, 0.6f, -0.9f);
                texBoxLocal(texContainer, W / 2 + 0.05f, (L - 2.6f) / 2, 0, 2.7f, mixC(c, WHITE, 0.5f), 0, 1, 0, true);
                rlPopMatrix();
                break;
            default:
                box(0, 1.75f, 0, W, 2.9f, L, c);
                box(0, 2.3f, 0, W + 0.04f, 0.7f, L - 1.0f, glass);
                box(0, 2.2f, L / 2 + 0.01f, W * 0.85f, 1.1f, 0.05f, glass);
                break;
        }
        float wr = 0.42f;
        for (int s = -1; s <= 1; s += 2) { wheelAt(s * W / 2, wr, L / 2 - 1.0f, wr, 0.3f, 0); wheelAt(s * W / 2, wr, -L / 2 + 1.0f, wr, 0.3f, 0); }
        box(0, 0.35f, 0, W * 0.9f, 0.2f, L * 0.9f, dark);
        rlPopMatrix();
        if (!t.wreck) {
            float hk = 0.3f + 0.7f * lk;
            addGlow(add(t.pos, rotY(vec(W * 0.35f, 0.75f, L / 2 + 0.05f), t.yaw)), 2.2f * hk, shade(C(255, 245, 210), hk));
            addGlow(add(t.pos, rotY(vec(-W * 0.35f, 0.75f, L / 2 + 0.05f), t.yaw)), 2.2f * hk, shade(C(255, 245, 210), hk));
            addGlow(add(t.pos, rotY(vec(W * 0.38f, 0.8f, -L / 2 - 0.05f), t.yaw)), 1.4f, C(200, 20, 20));
            addGlow(add(t.pos, rotY(vec(-W * 0.38f, 0.8f, -L / 2 - 0.05f), t.yaw)), 1.4f, C(200, 20, 20));
        } else if (fmodf(gTime * 2, 1.0f) < 0.5f) {
            addGlow(add(t.pos, rotY(vec(W * 0.45f, 0.8f, L / 2), t.yaw)), 1.8f, C(255, 160, 30));
            addGlow(add(t.pos, rotY(vec(-W * 0.45f, 0.8f, -L / 2), t.yaw)), 1.8f, C(255, 160, 30));
        }
    }
}
static void drawBubbles(Camera3D& cam) {
    for (const Bubble& b : bubbles) {
        gFog = 0; gLightL = lightDir;
        float pulse = 0.85f + 0.15f * sinf(gTime * 6);
        int saved = gAlpha; gAlpha = 255;
        ball(b.p, b.r * pulse, C(90, 200, 255, 60), BLANK, 6, 12);
        gAlpha = saved;
        DrawBillboard(cam, texGlow, b.p, b.r * 2.4f, C(60, 160, 255, 90));
    }
}

// =====================================================================
//                              CAMERA
// =====================================================================
struct CamState { V3 pos{0, 6, -10}; float yaw = 0, fov = 65, dist = 8, hgt = 3; int mode = 0; };
static CamState cs, cs2;      // one camera per human player
// keep cameras inside walls on indoor / walled sections
static V3 clampToCorridor(V3 p, int idx) {
    if (!indoorTrack() && theme[idx] != TH_TUNNEL) return p;
    int best = idx; float bd = 1e18f;
    for (int k = -30; k <= 30; k++) { int i = (idx + k + N) % N; float d = dist2D(track[i], p); if (d < bd) { bd = d; best = i; } }
    V3 d = sub(p, track[best]); float lat = d.x * norv[best].x + d.z * norv[best].z, L = lim[best] - 1.5f;
    if (fabsf(lat) > L) p = sub(p, mul(norv[best], lat - sgn(lat) * L));
    p.y = std::min(p.y, indoorTrack() ? 5.2f : 6.8f);
    return p;
}
static Camera3D updateCamera(CamState& cs, const Racer& pl, float dt, bool rear) {
    Camera3D cam{};
    cam.projection = CAMERA_PERSPECTIVE;
    float spd = speedOf(pl);
    float tf = 62.0f + clampf(spd * 0.25f, 0, 24) + (boostK(pl) > 0 ? 10.0f : 0.0f);
    if (cs.mode == 1) tf += 4.0f;
    cs.fov = lerpf(cs.fov, tf, clampf(3 * dt, 0, 1));
    cam.fovy = cs.fov;
    if (slowmoT > 0 && numHumans < 2 && takedownVictim >= 0 && takedownVictim < (int)racers.size()) {   // takedown cam
        const Racer& v = racers[takedownVictim];
        float a = gTime * 0.8f;
        cam.target = add(v.pos, vec(0, 1.0f, 0));
        cam.position = clampToCorridor(add(v.pos, vec(sinf(a) * 9, 3.5f, cosf(a) * 9)), v.lastIdx);
        cam.up = vec(0, 1, 0); cam.fovy = 55;
    } else if (cs.mode == 1 && !rear) {
        V3 f = fwd3(pl), l = norm(vec(cosf(pl.yaw), 0, -sinf(pl.yaw)));
        V3 u0 = cross(f, l); V3 u = sub(mul(u0, cosf(pl.roll)), mul(l, sinf(pl.roll)));
        float eh = (spaceNames && pl.mode == M_LAND ? 0.45f : 0.0f) + (pl.mode == M_AIR ? (pl.base == B_ROBOT ? 2.9f : pl.base == B_TANK ? 2.2f : 1.4f)
                                    : (pl.base == B_CAR ? 1.15f : pl.base == B_TANK ? 2.7f : pl.base == B_KART ? 1.75f : 3.75f));
        float ef = pl.base == B_CAR ? 0.2f : pl.base == B_TANK ? 1.0f : pl.base == B_KART ? -0.1f : 0.7f;
        V3 eye = add(pl.pos, add(mul(u, eh * (pl.shrinkT > 0 ? 0.6f : 1.0f)), mul(f, ef)));
        cam.position = eye; cam.target = add(eye, mul(f, 50.0f)); cam.up = u;
        cs.pos = eye; cs.yaw = pl.yaw;
    } else {
        float k = cs.mode == 2 ? 1.7f : 1.0f;
        float dd = (pl.mode == M_AIR ? 14.0f : pl.base == B_CAR ? 7.6f : pl.base == B_TANK ? 11.5f : pl.base == B_KART ? 6.6f : 10.0f) * k;
        float hh = (pl.mode == M_AIR ? 3.8f : pl.base == B_CAR ? 2.5f : pl.base == B_TANK ? 4.6f : pl.base == B_KART ? 2.6f : 5.2f) * k;
        dd += clampf(spd * 0.02f, 0, 1.5f);
        cs.dist = lerpf(cs.dist, dd, clampf(3 * dt, 0, 1)); cs.hgt = lerpf(cs.hgt, hh, clampf(3 * dt, 0, 1));
        float targetYaw = pl.yaw + (rear ? PI_F : 0.0f);
        if (rear) cs.yaw = targetYaw;
        else cs.yaw += wrapA(targetYaw - cs.yaw) * clampf((pl.mode == M_AIR ? 3.5f : 7.0f) * dt, 0, 1);
        V3 dir = fwdV(cs.yaw);
        V3 p = sub(pl.pos, mul(dir, cs.dist));
        float gy = pl.mode == M_LAND ? roadHeight(p, pl.lastIdx) : 0.0f;
        p.y = pl.pos.y + cs.hgt - (pl.mode == M_AIR ? sinf(pl.pitch) * cs.dist * 0.7f : 0.0f);
        if (p.y < gy + 0.8f) p.y = gy + 0.8f;
        if (theme[pl.lastIdx] == TH_TUNNEL) p.y = std::min(p.y, 6.8f);
        if (indoorTrack()) p.y = std::min(p.y, 5.2f);
        cs.pos = p; cam.position = p;
        cam.target = add(pl.pos, add(mul(dir, 10.0f), vec(0, 1.3f + (pl.mode == M_AIR ? sinf(pl.pitch) * 12.0f : 0.0f), 0)));
        float rl = pl.mode == M_AIR ? pl.roll * 0.35f : 0.0f;
        cam.up = vec(-sinf(rl) * cosf(cs.yaw), cosf(rl), sinf(rl) * sinf(cs.yaw));
    }
    float shk = pl.pid >= 0 ? shakeP[pl.pid & 1] : shakeAmt;
    if (shk > 0.01f) {
        float s = shk * 0.5f;
        V3 o = vec(frand(-s, s), frand(-s, s), frand(-s, s));
        cam.position = add(cam.position, o); cam.target = add(cam.target, mul(o, 0.5f));
    }
    if (spd > 50 && pl.mode == M_LAND) { float v = (spd - 50) * 0.0015f; cam.position = add(cam.position, vec(frand(-v, v), frand(-v, v), 0)); }
    camPos = cam.position; camFwd = norm(sub(cam.target, cam.position));
    return cam;
}

// =====================================================================
//                         HUD PRIMITIVES
// =====================================================================
static const Color ACC = {255, 176, 0, 255};
static const Color NITRO = {60, 170, 255, 255};
static void panel(float x, float y, float w, float h, float a = 0.55f, float skew = 14) {
    Vector2 p0 = v2(x + skew, y), p1 = v2(x + w + skew, y), p2 = v2(x + w, y + h), p3 = v2(x, y + h);
    Color c = alpha(C(8, 10, 16), a);
    DrawTriangle(p0, p3, p2, c); DrawTriangle(p0, p2, p1, c);
}
static void accentBar(float x, float y, float w, float h, Color c, float skew = 6) {
    DrawTriangle(v2(x + skew, y), v2(x, y + h), v2(x + w, y + h), c); DrawTriangle(v2(x + skew, y), v2(x + w, y + h), v2(x + w + skew, y), c);
}
static float keycap(float x, float y, const char* label, float h = 30) {
    float tw = 0; bool arrow = strlen(label) == 1 && strchr("^v<>", label[0]);
    if (!arrow) tw = txtW(label, h * 0.55f);
    float w = std::max(h, tw + h * 0.6f);
    DrawRectangleRounded(Rectangle{x, y + 3, w, h}, 0.25f, 6, C(60, 64, 72, 230));
    DrawRectangleRounded(Rectangle{x, y, w, h}, 0.25f, 6, C(235, 237, 242, 240));
    if (arrow) {
        float cx = x + w / 2, cy = y + h / 2, s = h * 0.22f;
        Color k = C(30, 32, 38);
        switch (label[0]) {
            case '^': DrawTriangle(v2(cx, cy - s), v2(cx - s, cy + s * 0.7f), v2(cx + s, cy + s * 0.7f), k); break;
            case 'v': DrawTriangle(v2(cx - s, cy - s * 0.7f), v2(cx, cy + s), v2(cx + s, cy - s * 0.7f), k); break;
            case '<': DrawTriangle(v2(cx - s, cy), v2(cx + s * 0.7f, cy + s), v2(cx + s * 0.7f, cy - s), k); break;
            default: DrawTriangle(v2(cx + s, cy), v2(cx - s * 0.7f, cy - s), v2(cx - s * 0.7f, cy + s), k); break;
        }
    } else txt(label, x + (w - tw) / 2, y + h * 0.2f, h * 0.55f, C(30, 32, 38));
    return w;
}
static void iconCar(float cx, float cy, float s, Color c) {
    DrawRectangleRounded(Rectangle{cx - s, cy - s * 0.15f, 2 * s, s * 0.5f}, 0.5f, 6, c);
    DrawTriangle(v2(cx - s * 0.5f, cy - s * 0.15f), v2(cx + s * 0.55f, cy - s * 0.15f), v2(cx + s * 0.25f, cy - s * 0.5f), c);
    DrawTriangle(v2(cx - s * 0.5f, cy - s * 0.15f), v2(cx + s * 0.25f, cy - s * 0.5f), v2(cx - s * 0.3f, cy - s * 0.5f), c);
    DrawCircleV(v2(cx - s * 0.55f, cy + s * 0.38f), s * 0.24f, c); DrawCircleV(v2(cx + s * 0.55f, cy + s * 0.38f), s * 0.24f, c);
}
static void iconKart(float cx, float cy, float s, Color c) {
    DrawRectangleRounded(Rectangle{cx - s, cy, 2 * s, s * 0.3f}, 0.5f, 6, c);
    DrawCircleV(v2(cx - s * 0.1f, cy - s * 0.35f), s * 0.3f, c);
    DrawCircleV(v2(cx - s * 0.65f, cy + s * 0.4f), s * 0.3f, c); DrawCircleV(v2(cx + s * 0.65f, cy + s * 0.4f), s * 0.24f, c);
}
static void iconTank(float cx, float cy, float s, Color c) {
    DrawRectangleRounded(Rectangle{cx - s, cy + s * 0.05f, 2 * s, s * 0.45f}, 0.8f, 6, c);
    DrawRectangle((int)(cx - s * 0.75f), (int)(cy - s * 0.2f), (int)(s * 1.5f), (int)(s * 0.3f), c);
    DrawRectangle((int)(cx - s * 0.35f), (int)(cy - s * 0.5f), (int)(s * 0.75f), (int)(s * 0.32f), c);
    DrawRectangle((int)(cx + s * 0.35f), (int)(cy - s * 0.42f), (int)(s * 0.75f), (int)(s * 0.1f), c);
}
static void iconRobot(float cx, float cy, float s, Color c) {
    DrawRectangle((int)(cx - s * 0.25f), (int)(cy - s), (int)(s * 0.5f), (int)(s * 0.4f), c);
    DrawRectangle((int)(cx - s * 0.45f), (int)(cy - s * 0.55f), (int)(s * 0.9f), (int)(s * 0.75f), c);
    DrawRectangle((int)(cx - s * 0.7f), (int)(cy - s * 0.5f), (int)(s * 0.2f), (int)(s * 0.6f), c);
    DrawRectangle((int)(cx + s * 0.5f), (int)(cy - s * 0.5f), (int)(s * 0.2f), (int)(s * 0.6f), c);
    DrawRectangle((int)(cx - s * 0.4f), (int)(cy + s * 0.25f), (int)(s * 0.3f), (int)(s * 0.75f), c);
    DrawRectangle((int)(cx + s * 0.1f), (int)(cy + s * 0.25f), (int)(s * 0.3f), (int)(s * 0.75f), c);
}
static void iconBoat(float cx, float cy, float s, Color c) {
    DrawTriangle(v2(cx - s, cy - s * 0.1f), v2(cx - s * 0.6f, cy + s * 0.35f), v2(cx + s, cy - s * 0.1f), c);
    DrawTriangle(v2(cx + s, cy - s * 0.1f), v2(cx - s * 0.6f, cy + s * 0.35f), v2(cx + s * 0.7f, cy + s * 0.35f), c);
    DrawRectangle((int)(cx - s * 0.3f), (int)(cy - s * 0.45f), (int)(s * 0.6f), (int)(s * 0.35f), c);
    for (int k = 0; k < 4; k++) DrawCircleSector(v2(cx - s * 0.75f + k * s * 0.5f, cy + s * 0.7f), s * 0.22f, 180, 360, 8, c);
}
static void iconPlane(float cx, float cy, float s, Color c) {
    DrawRectangleRounded(Rectangle{cx - s, cy - s * 0.12f, 2 * s, s * 0.24f}, 1.0f, 6, c);
    DrawTriangle(v2(cx - s * 0.1f, cy - s * 0.1f), v2(cx + s * 0.35f, cy - s * 0.1f), v2(cx - s * 0.35f, cy - s * 0.9f), c);
    DrawTriangle(v2(cx - s * 0.1f, cy + s * 0.1f), v2(cx - s * 0.35f, cy + s * 0.9f), v2(cx + s * 0.35f, cy + s * 0.1f), c);
    DrawTriangle(v2(cx - s, cy - s * 0.05f), v2(cx - s * 0.7f, cy - s * 0.05f), v2(cx - s * 0.95f, cy - s * 0.5f), c);
}
static void iconFor(Base b, Mode m, float cx, float cy, float s, Color c) {
    if (m == M_AIR) iconPlane(cx, cy, s, c); else if (m == M_WATER) iconBoat(cx, cy, s, c);
    else if (b == B_TANK) iconTank(cx, cy, s, c); else if (b == B_ROBOT) iconRobot(cx, cy, s, c); else if (b == B_KART) iconKart(cx, cy, s, c); else iconCar(cx, cy, s, c);
}
static void iconBolt(float cx, float cy, float s, Color c) {
    DrawTriangle(v2(cx + s * 0.3f, cy - s), v2(cx - s * 0.5f, cy + s * 0.15f), v2(cx + s * 0.1f, cy + s * 0.15f), c);
    DrawTriangle(v2(cx - s * 0.1f, cy - s * 0.15f), v2(cx - s * 0.3f, cy + s), v2(cx + s * 0.5f, cy - s * 0.15f), c);
}
static void iconCross(float cx, float cy, float s, Color c) {
    DrawRectangle((int)(cx - s * 0.25f), (int)(cy - s * 0.8f), (int)(s * 0.5f), (int)(s * 1.6f), c);
    DrawRectangle((int)(cx - s * 0.8f), (int)(cy - s * 0.25f), (int)(s * 1.6f), (int)(s * 0.5f), c);
}
static void iconCoin(float cx, float cy, float s) {
    DrawCircleV(v2(cx, cy), s, C(255, 200, 30)); DrawCircleV(v2(cx, cy), s * 0.7f, C(255, 225, 90));
    DrawRectangle((int)(cx - s * 0.12f), (int)(cy - s * 0.45f), (int)(s * 0.24f + 1), (int)(s * 0.9f), C(220, 160, 20));
}
static void iconItem(int it, float cx, float cy, float s) {
    switch (it) {
        case IT_TURBO:
            for (int k = 0; k < 3; k++) { float x = cx - s * 0.6f + k * s * 0.45f; DrawTriangle(v2(x, cy - s * 0.6f), v2(x, cy + s * 0.6f), v2(x + s * 0.55f, cy), k == 2 ? C(255, 220, 80) : C(255, 140, 30)); }
            break;
        case IT_ROCKET:
            DrawRectangleRounded(Rectangle{cx - s * 0.8f, cy - s * 0.22f, s * 1.4f, s * 0.44f}, 0.6f, 6, C(230, 50, 50));
            DrawTriangle(v2(cx + s * 0.6f, cy - s * 0.22f), v2(cx + s * 0.6f, cy + s * 0.22f), v2(cx + s * 0.95f, cy), WHITE);
            DrawTriangle(v2(cx - s * 0.8f, cy - s * 0.2f), v2(cx - s * 0.5f, cy - s * 0.2f), v2(cx - s * 0.9f, cy - s * 0.6f), WHITE);
            DrawTriangle(v2(cx - s * 0.8f, cy + s * 0.2f), v2(cx - s * 0.9f, cy + s * 0.6f), v2(cx - s * 0.5f, cy + s * 0.2f), WHITE);
            DrawCircleV(v2(cx - s * 0.95f, cy), s * 0.18f, C(255, 180, 40));
            break;
        case IT_OIL:
            DrawCircleV(v2(cx, cy + s * 0.2f), s * 0.55f, C(40, 40, 50));
            DrawTriangle(v2(cx, cy - s * 0.85f), v2(cx - s * 0.48f, cy + s * 0.0f), v2(cx + s * 0.48f, cy + s * 0.0f), C(40, 40, 50));
            DrawCircleV(v2(cx - s * 0.18f, cy + s * 0.1f), s * 0.12f, C(140, 140, 170));
            break;
        case IT_SHIELD:
            DrawRing(v2(cx, cy), s * 0.6f, s * 0.85f, 0, 360, 32, C(80, 200, 255));
            DrawCircleV(v2(cx, cy), s * 0.45f, C(80, 200, 255, 90));
            break;
        case IT_BOMB:
            DrawCircleV(v2(cx, cy + s * 0.1f), s * 0.6f, C(35, 35, 40));
            DrawRectangle((int)(cx - s * 0.12f), (int)(cy - s * 0.75f), (int)(s * 0.24f + 1), (int)(s * 0.3f), C(120, 120, 120));
            DrawCircleV(v2(cx + s * 0.15f, cy - s * 0.85f), s * 0.16f, C(255, 180, 40));
            DrawCircleV(v2(cx - s * 0.2f, cy - s * 0.1f), s * 0.12f, C(200, 200, 210));
            break;
        case IT_LIGHTNING: iconBolt(cx, cy, s * 0.9f, C(255, 230, 60)); break;
        default: break;
    }
}
static std::string fmtTime(float t) {
    if (t <= 0) return "--:--.--";
    int m = (int)(t / 60); float s = fmodf(t, 60.0f);
    char b[32]; snprintf(b, sizeof b, "%d:%05.2f", m, s); return b;
}
static bool padOn() { return padOfPlayer(vpPlayer < (int)racers.size() ? racers[vpPlayer].pid : 0) >= 0; }

// =====================================================================
//                               RACE HUD
// =====================================================================
// keyboard layout per player: player 2 uses the arrows and the right-hand keys
struct KeySet { const char *gas, *brake, *left, *right, *nitro, *special, *item, *dash, *sl, *sr; };
static KeySet keysFor(const Racer& pl) {
    if (pl.pid == 1) return {"^", "v", "<", ">", "R-SHIFT", "R-CTRL", "/", ".", ";", "'"};
    return {"W", "S", "A", "D", "SHIFT", "SPACE", "R", "F", "Q", "E"};
}
static void drawTach(const Racer& pl) {
    float R = 118, cx = SW - R - 34, cy = SH - R - 26;
    Vector2 c = v2(cx, cy);
    DrawTexturePro(texGlow, Rectangle{0, 0, 64, 64}, Rectangle{cx - R * 1.5f, cy - R * 1.5f, R * 3, R * 3}, v2(0, 0), 0, C(0, 0, 0, 170));
    DrawCircleV(c, R, C(10, 12, 18, 200));
    DrawRing(c, R - 3, R, 0, 360, 72, C(255, 255, 255, 40));
    float rpm = clampf(pl.rpm, 0, 1);
    bool wheels = (pl.base == B_CAR || pl.base == B_KART) && pl.mode == M_LAND;
    for (int k = 0; k <= 9; k++) {
        float a = (135 + 270 * k / 9.0f) * DEG2RAD;
        Color tc = k >= 7 ? C(255, 60, 50) : C(230, 230, 235);
        DrawLineEx(v2(cx + cosf(a) * (R - 16), cy + sinf(a) * (R - 16)), v2(cx + cosf(a) * (R - 4), cy + sinf(a) * (R - 4)), 3, tc);
        const char* n = TextFormat("%d", k);
        float tw = txtW(n, 18);
        txt(n, cx + cosf(a) * (R - 30) - tw / 2, cy + sinf(a) * (R - 30) - 9, 18, alpha(tc, 0.9f));
        if (k < 9) for (int m = 1; m < 4; m++) {
            float b = (135 + 270 * (k + m / 4.0f) / 9.0f) * DEG2RAD;
            DrawLineEx(v2(cx + cosf(b) * (R - 9), cy + sinf(b) * (R - 9)), v2(cx + cosf(b) * (R - 4), cy + sinf(b) * (R - 4)), 1.5f, C(200, 200, 205, 160));
        }
    }
    DrawRing(c, R - 4, R + 2, 135 + 270 * 7 / 9.0f, 405, 24, C(255, 50, 40, 110));
    Color arcC = rpm > 0.85f ? C(255, 70, 50) : mixC(C(255, 255, 255), ACC, rpm);
    DrawRing(c, R + 3, R + 8, 135, 135 + 270 * rpm, 60, arcC);
    float a = (135 + 270 * rpm) * DEG2RAD;
    DrawLineEx(v2(cx - cosf(a) * 14, cy - sinf(a) * 14), v2(cx + cosf(a) * (R - 12), cy + sinf(a) * (R - 12)), 4, C(255, 70, 40));
    DrawCircleV(c, 9, C(40, 42, 50)); DrawCircleV(c, 5, C(255, 70, 40));
    txtC(TextFormat("%d", (int)(speedOf(pl) * 3.6f)), cx + 6, cy + 14, 62, WHITE, 1);
    txtC(T("\320\232\320\234/\320\247", "KM/H"), cx, cy + 74, 16, C(180, 185, 195), 0, false);
    if (wheels) {
        const char* g = pl.gear == 0 ? "R" : TextFormat("%d", pl.gear);
        DrawRectangleRounded(Rectangle{cx - 20, cy - 58, 40, 40}, 0.3f, 6, C(30, 32, 40, 230));
        txtC(g, cx + 2, cy - 58, 38, pl.gearShiftT > 0 ? ACC : WHITE, 1, false);
        if (pl.driftCharge > 0.8f) DrawRing(v2(cx, cy - 38), 24, 28, -90, -90 + 360 * clampf(pl.driftCharge / 3.0f, 0, 1), 32, driftColor(driftStage(pl)));
    } else if (pl.mode == M_AIR) {
        txtC(TextFormat("%d %s", (int)pl.pos.y, T("\320\234", "M")), cx, cy - 46, 24, C(200, 160, 255), 1, false);
        float pa = clampf(pl.pitch, -0.6f, 0.6f);
        DrawTriangle(v2(cx - 42, cy - 50 - pa * 30), v2(cx - 52, cy - 44 - pa * 30), v2(cx - 52, cy - 56 - pa * 30), C(200, 160, 255));
    } else iconFor(pl.base, pl.mode, cx, cy - 40, 18, C(220, 225, 235));
    float bx = cx - R - 46, by = cy - 70, bh = 150;
    float nit = pl.energy / 100.0f, hp = clampf(pl.hp / 100.0f, 0, 1);
    DrawRectangleRounded(Rectangle{bx, by, 14, bh}, 0.5f, 6, C(10, 12, 18, 190));
    DrawRectangleRounded(Rectangle{bx + 2, by + 2 + (bh - 4) * (1 - nit), 10, (bh - 4) * nit}, 0.5f, 6, pl.boostLock ? C(120, 90, 60) : (pl.boosting ? WHITE : NITRO));
    iconBolt(bx + 7, by + bh + 16, 9, NITRO);
    Color hc = hp > 0.5f ? C(70, 220, 110) : hp > 0.25f ? C(255, 190, 40) : C(255, 60, 50);
    if (hp < 0.25f && fmodf(gTime, 0.5f) < 0.25f) hc = WHITE;
    DrawRectangleRounded(Rectangle{bx + 22, by, 14, bh}, 0.5f, 6, C(10, 12, 18, 190));
    DrawRectangleRounded(Rectangle{bx + 24, by + 2 + (bh - 4) * (1 - hp), 10, (bh - 4) * hp}, 0.5f, 6, hc);
    iconCross(bx + 29, by + bh + 16, 9, hc);
    float sx = cx - R - 46, sy = by - 46;
    iconFor(pl.base, pl.mode, sx + 18, sy + 14, 15, pl.switchCd > 0 ? C(120, 125, 135) : WHITE);
    if (pl.switchCd > 0) DrawRing(v2(sx + 18, sy + 14), 21, 24, -90, -90 + 360 * (1 - pl.switchCd / 6.0f), 32, ACC);
    if (pl.shieldT > 0) DrawRing(c, R + 10, R + 13, -90, -90 + 360 * pl.shieldT / 10.0f, 48, C(80, 200, 255));
    if (pl.draftK > 0.05f) {   // slipstream charge
        float k = pl.draftK, w = 150;
        panel(cx - w / 2 - 10, cy - R - 54, w + 20, 30, 0.5f * k + 0.2f);
        accentBar(cx - w / 2, cy - R - 32, w * k, 5, C(160, 220, 255));
        txtC(T("\320\241\320\233\320\230\320\237\320\241\320\242\320\240\320\230\320\234", "SLIPSTREAM"), cx, cy - R - 52, 17, alpha(C(190, 230, 255), 0.5f + 0.5f * k), 0, false);
    }
}
static void drawRadar(const Racer& pl) {
    float R = 92, cx = 34 + R, cy = SH - R - 30;
    float scale = R / 420.0f;
    DrawCircleV(v2(cx, cy), R + 6, C(8, 10, 16, 190));
    DrawRing(v2(cx, cy), R + 4, R + 7, 0, 360, 72, S.mode == GM_PURSUIT ? (fmodf(gTime * 2, 1) < 0.5f ? C(255, 40, 40) : C(40, 90, 255)) : alpha(ACC, 0.8f));
    V3 f = fwdV(pl.yaw), rt = rgtV(pl.yaw);
    auto toS = [&](V3 p, Vector2& o) {
        V3 d = sub(p, pl.pos);
        o = v2(cx + dot(d, rt) * scale, cy - dot(d, f) * scale);
        float dx = o.x - cx, dy = o.y - cy;
        return dx * dx + dy * dy < R * R;
    };
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < N; i += 2) {
            if (dist2D(track[i], pl.pos) > 470) continue;
            Vector2 a, b;
            if (!toS(track[i], a) || !toS(track[(i + 2) % N], b)) continue;
            if (pass == 0) DrawLineEx(a, b, 9, C(70, 75, 85, 230));
            else DrawLineEx(a, b, 4, zmode[i] == M_WATER ? C(60, 170, 255) : zmode[i] == M_AIR ? C(190, 110, 255) : C(205, 205, 212));
        }
    Vector2 s;
    if (toS(track[0], s)) DrawRectangle((int)s.x - 4, (int)s.y - 4, 8, 8, WHITE);
    for (int g : gateIdx) if (toS(track[g], s)) DrawCircleV(s, 5, gateColor(zmode[(g + 3) % N]));
    for (const Traffic& t : traffic) if (!t.dead && toS(t.pos, s)) DrawRectangle((int)s.x - 2, (int)s.y - 2, 4, 4, C(160, 160, 170));
    for (const Shortcut& sc : shortcuts) {   // tank shortcuts: dashed lines (yellow = closed, green = open)
        Color c = sc.open ? C(120, 255, 140) : C(255, 200, 40);
        for (float u = 0; u < sc.len; u += 24) {
            Vector2 a, b;
            if (toS(add(sc.pa, mul(sc.dir, u)), a) && toS(add(sc.pa, mul(sc.dir, std::min(sc.len, u + 12))), b)) DrawLineEx(a, b, 3, c);
        }
    }
    for (const LavaBomb& b : lavaBombs) if (toS(b.p, s)) DrawCircleLines((int)s.x, (int)s.y, 5, C(255, 80, 30));
    for (const Racer& r : racers) {
        if (r.id == pl.id || r.dead || r.out) continue;
        Color rc = r.isCop ? (fmodf(gTime * 3 + r.id * 0.3f, 1) < 0.5f ? C(255, 40, 40) : C(60, 110, 255)) : r.color;
        if (r.boss && fmodf(gTime * 2, 1) < 0.5f) rc = C(255, 60, 60);
        if (toS(r.pos, s)) { DrawCircleV(s, r.boss ? 7.5f : r.isPlayer ? 6.5f : 5.5f, r.isPlayer ? WHITE : C(10, 10, 10)); DrawCircleV(s, r.boss ? 6 : 4, rc); }
        else {
            V3 d = sub(r.pos, pl.pos); float x = dot(d, rt), y = -dot(d, f), l = sqrtf(x * x + y * y);
            if (l < 1e-3f) continue;
            DrawCircleV(v2(cx + x / l * (R - 2), cy + y / l * (R - 2)), 3.5f, alpha(rc, 0.8f));
        }
    }
    DrawTriangle(v2(cx, cy - 10), v2(cx - 7, cy + 7), v2(cx + 7, cy + 7), WHITE);
    DrawTriangle(v2(cx, cy - 6), v2(cx - 4, cy + 4), v2(cx + 4, cy + 4), ACC);
}
static int nextGate(const Racer& pl, float& distM) {
    int best = -1; float bd = 1e9f;
    for (int g : gateIdx) { int d = (g - pl.lastIdx + N) % N; if (d == 0) continue; if (d < bd) { bd = (float)d; best = g; } }
    distM = bd * trackLen / N;
    return best;
}
static void drawHints(const Racer& pl, bool forceShow) {
    float a = forceShow ? 1.0f : clampf(hintT, 0, 1);
    if (a <= 0.01f) return;
    bool pad = padOn();
    KeySet K = keysFor(pl);
    struct H { const char* k1; const char* k2; const char* t; };
    std::vector<H> hs;
    if (pad) {
        hs.push_back({"RT", "LT", pl.mode == M_AIR ? T("\320\222\320\222\320\225\320\240\320\245 / \320\222\320\235\320\230\320\227", "CLIMB / DIVE") : T("\320\223\320\220\320\227 / \320\242\320\236\320\240\320\234\320\236\320\227", "GAS / BRAKE")});
        hs.push_back({"A", nullptr, pl.base == B_TANK ? T("\320\222\320\253\320\241\320\242\320\240\320\225\320\233", "FIRE") : pl.base == B_ROBOT ? T("\320\237\320\240\320\253\320\226\320\236\320\232", "JUMP") : T("\320\227\320\220\320\235\320\236\320\241", "DRIFT")});
        hs.push_back({"X", nullptr, T("\320\235\320\230\320\242\320\240\320\236", "NITRO")});
        if (pl.item != IT_NONE) hs.push_back({"B", nullptr, T("\320\237\320\240\320\225\320\224\320\234\320\225\320\242", "ITEM")});
    } else if (pl.mode == M_AIR) {
        hs.push_back({K.gas, K.brake, T("\320\222\320\222\320\225\320\240\320\245 / \320\222\320\235\320\230\320\227", "CLIMB / DIVE")}); hs.push_back({K.left, K.right, T("\320\232\320\240\320\225\320\235", "BANK")});
        hs.push_back({K.nitro, nullptr, T("\320\244\320\236\320\240\320\241\320\220\320\226", "AFTERBURNER")}); hs.push_back({K.special, nullptr, T("\320\242\320\236\320\240\320\234\320\236\320\227", "AIRBRAKE")});
    } else {
        hs.push_back({K.gas, K.brake, pl.mode == M_WATER ? T("\320\245\320\236\320\224", "THROTTLE") : T("\320\223\320\220\320\227 / \320\242\320\236\320\240\320\234\320\236\320\227", "GAS / BRAKE")});
        hs.push_back({K.left, K.right, T("\320\240\320\243\320\233\320\254", "STEER")}); hs.push_back({K.nitro, nullptr, T("\320\235\320\230\320\242\320\240\320\236", "NITRO")});
        if ((pl.base == B_CAR || pl.base == B_KART)) hs.push_back({K.special, nullptr, T("\320\227\320\220\320\235\320\236\320\241 = \320\242\320\243\320\240\320\221\320\236", "DRIFT = BOOST")});
        if (pl.base == B_TANK) hs.push_back({K.special, nullptr, T("\320\222\320\253\320\241\320\242\320\240\320\225\320\233", "FIRE")});
        if (pl.base == B_ROBOT) { hs.push_back({K.sl, K.sr, T("\320\222\320\221\320\236\320\232", "STRAFE")}); if (pl.mode == M_LAND) hs.push_back({K.special, nullptr, T("\320\237\320\240\320\253\320\226\320\236\320\232", "JUMP")}); hs.push_back({K.dash, nullptr, T("\320\240\320\253\320\222\320\236\320\232", "DASH")}); }
    }
    if (!pad && pl.item != IT_NONE) hs.push_back({K.item, nullptr, T("\320\237\320\240\320\225\320\224\320\234\320\225\320\242", "ITEM")});
    float kh = 30, fs = 17, total = 0;
    for (int pass = 0; pass < 3; pass++) {
        total = 0;
        for (auto& h : hs) total += (h.k2 ? 2 * kh + 6 : txtW(h.k1, kh * 0.55f) + kh * 0.6f) + 10 + txtW(h.t, fs) + 28;
        if (total < SW - 620) break;
        kh *= 0.85f; fs *= 0.85f;
    }
    float x = SW / 2 - total / 2, y = SH - 64;
    panel(x - 24, y - 10, total + 30, kh + 22, 0.5f * a);
    for (auto& h : hs) {
        x += keycap(x, y, h.k1, kh) + 6;
        if (h.k2) x += keycap(x, y, h.k2, kh) + 4;
        txt(h.t, x + 4, y + kh * 0.2f, fs, alpha(WHITE, a));
        x += txtW(h.t, fs) + 30;
    }
}
static void drawItemSlot(const Racer& pl) {
    if (!itemsOn()) return;
    float x = 362, y = 14, sz = 92;
    DrawRectangleRounded(Rectangle{x, y, sz, sz}, 0.25f, 8, C(8, 10, 16, 170));
    DrawRectangleRounded(Rectangle{x + 4, y + 4, sz - 8, sz - 8}, 0.25f, 8, C(30, 34, 46, 200));
    Color border = pl.item != IT_NONE ? rainbow(gTime * 3) : C(120, 125, 135);
    DrawRing(v2(x + sz / 2, y + sz / 2), sz * 0.47f, sz * 0.5f, 0, 360, 48, alpha(border, 0.25f));
    if (pl.rouletteT > 0) iconItem(1 + ((int)(gTime * 14)) % (IT_COUNT - 1), x + sz / 2, y + sz / 2, sz * 0.32f);
    else if (pl.item != IT_NONE) {
        iconItem(pl.item, x + sz / 2, y + sz / 2, sz * 0.34f);
        if (pl.itemCount > 1) { DrawCircleV(v2(x + sz - 12, y + sz - 12), 13, ACC); txtC(TextFormat("%d", pl.itemCount), x + sz - 12, y + sz - 24, 20, C(20, 20, 20), 1, false); }
        float kx = x + sz / 2 - 13;
        keycap(kx, y + sz + 4, padOn() ? "B" : keysFor(pl).item, 24);
    } else txtC("?", x + sz / 2, y + sz / 2 - 22, 40, C(90, 95, 110), 1, false);
}
static void drawModePanel(const Racer& pl) {
    float y = 66;
    if (S.mode == GM_ELIM) {
        int act = 0; for (auto& r : racers) if (!r.out && !r.isCop) act++;
        if (act <= 1) return;
        const char* t = TextFormat(T("\320\222\320\253\320\221\320\253\320\222\320\220\320\235\320\230\320\225 \320\247\320\225\320\240\320\225\320\227 %d", "ELIMINATION IN %d"), (int)ceilf(elimT));
        float w = txtW(t, 22) + 50;
        bool hot = elimT < 5.0f;
        panel(SW / 2 - w / 2, y, w, 40, 0.6f);
        accentBar(SW / 2 - w / 2, y + 36, w * clampf(elimT / 30.0f, 0, 1), 4, hot ? C(255, 60, 40) : ACC);
        txtC(t, SW / 2, y + 8, 22, hot && fmodf(gTime * 4, 1) < 0.5f ? C(255, 80, 60) : WHITE, 0, false);
    } else if (S.mode == GM_PURSUIT) {
        float w = 360;
        panel(SW / 2 - w / 2, y, w, 52, 0.6f);
        txt(T("\320\240\320\236\320\227\320\253\320\241\320\232", "HEAT"), SW / 2 - w / 2 + 26, y + 6, 15, C(255, 90, 80));
        for (int k = 0; k < 5; k++) {
            float sx = SW / 2 - w / 2 + 30 + k * 22, sy = y + 34;
            Color sc = k < (int)heat ? (fmodf(gTime * 2 + k * 0.2f, 1) < 0.5f ? C(255, 50, 50) : C(60, 110, 255)) : C(70, 70, 80);
            DrawPoly(v2(sx, sy), 5, 9, -90, sc);
        }
        float left = std::max(0.0f, pursuitGoal - pursuitT);
        txt(T("\320\224\320\225\320\240\320\226\320\230\320\241\320\254", "SURVIVE"), SW / 2 + 20, y + 6, 15, ACC);
        txt(fmtTime(left).c_str(), SW / 2 + 20, y + 22, 26, WHITE, 1);
        if (bustedT > 0.05f) {
            float bw = 300, bx = SW / 2 - bw / 2, by = y + 62;
            panel(bx, by, bw, 30, 0.7f);
            accentBar(bx + 10, by + 8, (bw - 30) * clampf(bustedT / 4.0f, 0, 1), 14, C(255, 50, 50));
            txtC(T("\320\220\320\240\320\225\320\241\320\242! \320\224\320\222\320\230\320\223\320\220\320\231\320\241\320\257!", "BUSTED! MOVE!"), SW / 2, by + 6, 17, WHITE, 0, false);
        }
    } else if (S.mode == GM_BATTLE) {
        int act = 0; for (auto& r : racers) if (!r.out && !r.isCop) act++;
        const char* t = TextFormat(T("\320\221\320\230\320\242\320\222\320\220  %s  \320\236\320\241\320\242\320\220\320\233\320\236\320\241\320\254: %d", "BATTLE  %s  LEFT: %d"), fmtTime(std::max(0.0f, battleT)).c_str(), act);
        float w = txtW(t, 22) + 50;
        panel(SW / 2 - w / 2, y, w, 40, 0.6f);
        accentBar(SW / 2 - w / 2, y + 36, w * clampf(battleT / 180.0f, 0, 1), 4, battleT < 20 ? C(255, 60, 40) : ACC);
        txtC(t, SW / 2, y + 8, 22, WHITE, 0, false);
    } else if (S.mode == GM_TIMETRIAL) {
        float w = 360;
        panel(SW / 2 - w / 2, y, w, 44, 0.6f);
        txt(T("\320\246\320\225\320\233\320\254", "TARGET"), SW / 2 - w / 2 + 26, y + 6, 15, ACC);
        txt(fmtTime(ttTarget).c_str(), SW / 2 - w / 2 + 26, y + 20, 20, WHITE, 1);
        float gt = ghostTimeAtIdx(pl.lastIdx);
        if (gt > 0 && pl.lap >= 1) {
            float delta = (raceTime - pl.lapStart) - gt;
            Color dc = delta <= 0 ? C(90, 255, 140) : C(255, 90, 80);
            txt(T("\320\237\320\240\320\230\320\227\320\240\320\220\320\232", "GHOST"), SW / 2 + 30, y + 6, 15, ACC);
            txt(TextFormat("%+.2f", delta), SW / 2 + 30, y + 20, 20, dc, 1);
        } else txt(T("\320\237\320\240\320\230\320\227\320\240\320\220\320\232\320\220 \320\235\320\225\320\242", "NO GHOST YET"), SW / 2 + 30, y + 16, 15, C(160, 165, 175));
    }
}
static void drawRaceHUD(Camera3D& cam, const Racer& pl, int myPos, bool showHelp) {
    if (boostK(pl) > 0) {
        float k = 0.5f + 0.5f * sinf(gTime * 20);
        Color bc = pl.boostT > 0 && !pl.boosting ? C(255, 140, 40) : NITRO;
        DrawRectangleGradientH(0, 0, 160, SH, alpha(bc, 0.22f + 0.05f * k), alpha(bc, 0));
        DrawRectangleGradientH(SW - 160, 0, 160, SH, alpha(bc, 0), alpha(bc, 0.22f + 0.05f * k));
    }
    float spd = speedOf(pl);
    float lk = clampf((spd / ST[pl.base][pl.mode].vmax - 0.75f) * 3.0f, 0, 1) + (boostK(pl) > 0 ? 0.6f : 0.0f);
    if (lk > 0.05f) for (int i = 0; i < 26; i++) {
        float a = frand(0, 2 * PI_F), r0 = frand(0.45f, 0.6f), r1 = r0 + frand(0.06f, 0.2f) * lk;
        DrawLineEx(v2(SW / 2 + cosf(a) * r0 * SW * 0.6f, SH / 2 + sinf(a) * r0 * SH * 0.6f), v2(SW / 2 + cosf(a) * r1 * SW * 0.6f, SH / 2 + sinf(a) * r1 * SH * 0.6f), 2, alpha(WHITE, 0.18f * lk));
    }
    if (pl.hurtT > 0) {
        float k = pl.hurtT * 0.8f;
        DrawRectangleGradientV(0, 0, SW, 120, alpha(C(255, 0, 0), 0.35f * k), alpha(C(255, 0, 0), 0));
        DrawRectangleGradientV(0, SH - 120, SW, 120, alpha(C(255, 0, 0), 0), alpha(C(255, 0, 0), 0.35f * k));
    }
    if (pl.shrinkT > 0) DrawRectangleLinesEx(Rectangle{0, 0, (float)SW, (float)SH}, 6, alpha(C(255, 230, 60), 0.4f + 0.2f * sinf(gTime * 10)));
    for (size_t i = 0; i < order.size(); i++) {
        const Racer& r = racers[order[i]];
        if (r.id == pl.id || r.dead || r.out) continue;
        V3 d = sub(r.pos, camPos); float dl = len(d);
        if (dl > (r.boss ? 260 : 160) || dot(camFwd, d) < 0.3f * dl) continue;
        Vector2 sp = w2s(vec(r.pos.x, r.pos.y + radiusOf(r) + (S.mode == GM_BATTLE ? 5.0f : 2.5f), r.pos.z), cam);
        if (r.boss) txtC(T("\320\221\320\236\320\241\320\241", "BOSS"), sp.x, sp.y - 44, 18, alpha(C(255, 80, 60), clampf(1.6f - dl / 260.0f, 0, 1)), 1);
        float s = clampf(16 - dl * 0.05f, 9, 15), a2 = clampf(1.4f - dl / 160.0f, 0, 1);
        DrawPoly(sp, 4, s, 45, alpha(r.color, a2)); DrawPolyLinesEx(sp, 4, s, 45, 2, alpha(WHITE, a2));
        txtC(TextFormat("%d", (int)i + 1), sp.x, sp.y - s * 0.6f, s * 1.2f, alpha(WHITE, a2), 1, false);
        if (dl < 70) txtC(r.name.c_str(), sp.x, sp.y - s - 20, 15, alpha(WHITE, a2 * 0.9f));
    }
    if (pl.base == B_TANK && pl.mode != M_AIR && !pl.dead) {
        Vector2 a = w2s(add(add(pl.pos, mul(fwd3(pl), 60)), vec(0, 2, 0)), cam);
        Color rc = pl.specialCd > 0 ? C(255, 120, 60, 200) : C(120, 255, 140, 220);
        DrawRing(a, 14, 16, 0, 360, 32, rc);
        DrawLineEx(v2(a.x - 26, a.y), v2(a.x - 8, a.y), 2, rc); DrawLineEx(v2(a.x + 8, a.y), v2(a.x + 26, a.y), 2, rc);
        DrawLineEx(v2(a.x, a.y + 8), v2(a.x, a.y + 22), 2, rc);
    }
    // position / lap / time
    panel(16, 14, 330, 112, 0.55f);
    accentBar(16, 14, 330, 4, ACC);
    if (S.mode == GM_BATTLE) {
        txt(T("\320\250\320\220\320\240\320\230\320\232\320\230", "BALLOONS"), 36, 24, 15, alpha(ACC, 0.95f));
        for (int k = 0; k < 3; k++) {
            float bx = 52 + k * 40, by = 66;
            if (k < pl.balloons) { DrawCircleV(v2(bx, by), 16, balloonCol(k)); DrawCircleV(v2(bx - 5, by - 6), 4, alpha(WHITE, 0.6f)); DrawLineEx(v2(bx, by + 16), v2(bx + 3, by + 34), 2, WHITE); }
            else DrawCircleLines((int)bx, (int)by, 15, C(110, 110, 120));
        }
        txt(TextFormat("%s %d", T("\320\236\320\247\320\232\320\230", "POINTS"), pl.points), 36, 98, 18, WHITE, 1);
    } else if (S.mode == GM_RACE || S.mode == GM_ELIM) {
        txt(T("\320\237\320\236\320\227\320\230\320\246\320\230\320\257", "POSITION"), 36, 24, 15, alpha(ACC, 0.95f));
        txtS(TextFormat("%d", myPos), 34, 34, 80, WHITE, 1);
        float pw = txtW(TextFormat("%d", myPos), 80, 1);
        int act = 0; for (auto& r : racers) if (!r.out && !r.isCop) act++;
        txtS(TextFormat("/%d", S.mode == GM_ELIM ? act : numRacers), 38 + pw, 74, 30, C(190, 195, 205), 1);
        if (numHumans > 1) txt(pl.pid == 0 ? T("\320\230\320\223\320\240\320\236\320\232 1", "PLAYER 1") : T("\320\230\320\223\320\240\320\236\320\232 2", "PLAYER 2"), 262, 24, 14, pl.pid == 0 ? C(120, 200, 255) : C(255, 140, 120));
    } else if (S.mode == GM_PURSUIT) {
        txt(T("\320\222\320\253\320\221\320\230\320\242\320\236 \320\232\320\236\320\237\320\236\320\222", "COPS WRECKED"), 36, 24, 15, alpha(ACC, 0.95f));
        txtS(TextFormat("%d", pl.takedowns), 34, 34, 80, WHITE, 1);
    } else {
        txt(T("\320\222\320\240\320\225\320\234\320\257", "TIME"), 36, 24, 15, alpha(ACC, 0.95f));
        txtS(fmtTime(raceTime).c_str(), 34, 50, 32, WHITE, 1);
    }
    float lx = 186;
    if (S.mode == GM_RACE || S.mode == GM_TIMETRIAL) {
        txt(T("\320\232\320\240\320\243\320\223", "LAP"), lx, 24, 15, alpha(ACC, 0.95f));
        txtS(TextFormat("%d/%d", std::min(std::max(pl.lap, 1), S.laps), S.laps), lx, 40, 36, WHITE, 1);
    } else { txt(T("\320\232\320\240\320\243\320\223", "LAP"), lx, 24, 15, alpha(ACC, 0.95f)); txtS(TextFormat("%d", std::max(pl.lap, 1)), lx, 40, 36, WHITE, 1); }
    txtS(fmtTime(pl.lap >= 1 ? raceTime - pl.lapStart : 0).c_str(), lx, 82, 20, WHITE);
    txt(TextFormat("%s %s", T("\320\233\320\243\320\247\320\250\320\230\320\231", "BEST"), fmtTime(pl.bestLap).c_str()), lx, 104, 14, C(170, 175, 185));
    // coins
    panel(16, 132, 150, 34, 0.5f);
    iconCoin(40, 149, 11);
    txt(TextFormat("x %d", pl.coins), 58, 138, 22, pl.coins >= 10 ? C(255, 220, 60) : WHITE, 1);
    if (pl.coins >= 10) txt("MAX", 112, 142, 14, C(255, 220, 60));
    drawItemSlot(pl);
    // leaderboard
    if (S.mode == GM_RACE || S.mode == GM_ELIM || S.mode == GM_BATTLE) {
        float ly = 16, lw = 210, lxr = SW - lw - 20;
        for (size_t i = 0; i < order.size(); i++) {
            const Racer& r = racers[order[i]];
            float yy = ly + i * 27;
            bool me = r.id == pl.id;
            panel(lxr, yy, lw, 24, me ? 0.8f : r.isPlayer ? 0.65f : 0.45f, 8);
            if (me) accentBar(lxr, yy, lw, 24, alpha(ACC, 0.35f), 8);
            if (r.boss) accentBar(lxr, yy, lw, 24, alpha(C(255, 50, 40), 0.3f), 8);
            if (S.mode == GM_BATTLE && !r.out) for (int k = 0; k < r.balloons; k++) DrawCircleV(v2(lxr + lw - 46 + k * 9.0f, yy + 12), 4, balloonCol(k));
            DrawRectangle((int)lxr + 12, (int)yy + 5, 5, 14, r.out ? C(90, 90, 90) : r.color);
            txt(TextFormat("%d", (int)i + 1), lxr + 24, yy + 3, 18, me ? ACC : (r.out ? C(120, 120, 120) : WHITE), 1);
            txt(r.name.c_str(), lxr + 50, yy + 4, 16, r.out ? C(120, 120, 120) : (r.finished ? C(140, 255, 160) : WHITE));
            if (r.out) txt(T("\320\222\320\253\320\221\320\253\320\233", "OUT"), lxr + lw - 62, yy + 5, 13, C(255, 90, 80));
            else iconFor(r.base, r.mode, lxr + lw - 18, yy + 12, 8, C(200, 205, 215));
        }
    }
    float dz; int g = nextGate(pl, dz);
    if (g >= 0 && dz < 650) {
        Mode nm = zmode[(g + 3) % N];
        Color gc = gateColor(nm);
        const char* nmText = spaceNames ? (nm == M_WATER ? T("\320\237\320\233\320\220\320\227\320\234\320\220", "PLASMA") : nm == M_AIR ? T("\320\236\320\242\320\232\320\240\320\253\320\242\320\253\320\231 \320\232\320\236\320\241\320\234\320\236\320\241", "OPEN SPACE") : T("\320\237\320\220\320\233\320\243\320\221\320\220", "DECK"))
                                        : nm == M_WATER ? T("\320\222\320\236\320\224\320\220", "WATER") : nm == M_AIR ? T("\320\222\320\227\320\233\320\201\320\242", "TAKE-OFF") : T("\320\227\320\225\320\234\320\233\320\257", "LAND");
        const char* t = TextFormat("%s  %d %s", nmText, (int)dz, T("\320\274", "m"));
        float w = txtW(t, 22) + 70, x = SW / 2 - w / 2, y = 18;
        float pulse = dz < 150 ? 0.5f + 0.5f * sinf(gTime * 12) : 1.0f;
        panel(x, y, w, 40, 0.6f);
        accentBar(x, y + 36, w, 4, alpha(gc, pulse));
        if (nm == M_WATER) iconBoat(x + 34, y + 20, 14, gc); else if (nm == M_AIR) iconPlane(x + 34, y + 20, 14, gc); else iconCar(x + 34, y + 20, 14, gc);
        txt(t, x + 58, y + 8, 22, WHITE);
    }
    drawModePanel(pl);
    // boss tracker: how far ahead / behind the boss is
    for (const Racer& b : racers) {
        if (!b.boss || b.out) continue;
        float gapM = (raceScore(b) - raceScore(pl)) * trackLen / N;
        const char* t = TextFormat("%s   %s", bossName(b.boss), gapM > 0 ? TextFormat(T("\320\262\320\277\320\265\321\200\320\265\320\264\320\270 %d \320\274", "%d m ahead"), (int)gapM) : TextFormat(T("\320\277\320\276\320\267\320\260\320\264\320\270 %d \320\274", "%d m behind"), (int)-gapM));
        float w = txtW(t, 20) + 60, y = S.mode == GM_PURSUIT ? 172.0f : 70.0f;
        panel(SW / 2 - w / 2, y, w, 34, 0.65f);
        accentBar(SW / 2 - w / 2, y + 31, w, 3, gapM > 0 ? C(255, 60, 50) : C(120, 255, 140));
        DrawPoly(v2(SW / 2 - w / 2 + 28, y + 17), 3, 9, 30 + gTime * 90, C(255, 70, 50));
        txt(t, SW / 2 - w / 2 + 44, y + 6, 20, WHITE);
    }
    int ni = 0;
    for (size_t i = 0; i < notes.size(); i++) {
        const Note& n = notes[i];
        if (n.owner >= 0 && n.owner != pl.pid) continue;
        float a = clampf(n.t, 0, 1), slide = (1 - clampf((2.6f - n.t) * 5, 0, 1)) * 40;
        float y = 128 + (ni++) * 46 + (S.mode == GM_PURSUIT ? 40 : 0) + (curBoss() ? 34 : 0);
        float w = txtW(n.text.c_str(), 30, 1) + 60;
        panel(SW / 2 - w / 2 + slide, y, w, 40, 0.55f * a);
        accentBar(SW / 2 - w / 2 + slide, y + 37, w, 3, alpha(n.c, a));
        txtC(n.text.c_str(), SW / 2 + slide + 6, y + 2, 30, alpha(n.c, a), 1);
    }
    if (takedownBannerT > 0) {
        float a = clampf(takedownBannerT, 0, 1), sc = 1.0f + clampf(takedownBannerT - 2.0f, 0, 0.4f);
        txtC(T("\320\222\320\253\320\221\320\230\320\242!", "TAKEDOWN!"), SW / 2, SH * 0.2f, 72 * sc, alpha(C(255, 70, 50), a), 1);
        txtC(TextFormat("%s   +$100", takedownName.c_str()), SW / 2, SH * 0.2f + 78, 26, alpha(WHITE, a), 1);
    }
    if (pl.wrongT > 1.5f && !pl.finished) {
        float k = 0.6f + 0.4f * sinf(gTime * 10);
        txtC(T("\320\235\320\225 \320\242\320\243\320\224\320\220!", "WRONG WAY!"), SW / 2, SH * 0.32f, 64, alpha(C(255, 50, 40), k), 1);
        txtC(T("\320\275\320\260\320\266\320\274\320\270\321\202\320\265 T, \321\207\321\202\320\276\320\261\321\213 \320\262\320\265\321\200\320\275\321\203\321\202\321\214\321\201\321\217 \320\275\320\260 \321\202\321\200\320\260\321\201\321\201\321\203", "press T to reset"), SW / 2, SH * 0.32f + 70, 18, alpha(WHITE, 0.8f));
    }
    if (pl.dead && takedownBannerT <= 0) txtC(T("\320\240\320\220\320\227\320\221\320\230\320\242 \342\200\224 \320\222\320\236\320\241\320\241\320\242\320\220\320\235\320\236\320\222\320\233\320\225\320\235\320\230\320\225...", "WRECKED - RESPAWNING..."), SW / 2, SH * 0.55f, 34, C(255, 80, 60), 1);
    if (numHumans > 1 && (pl.finished || pl.out) && resultState == 0) {
        panel(SW / 2 - 260, SH * 0.36f, 520, 70, 0.6f);
        txtC(pl.out ? T("\320\222\320\253 \320\222\320\253\320\221\320\253\320\233\320\230", "YOU'RE OUT") : TextFormat(T("\320\244\320\230\320\235\320\230\320\250: %d-\320\265 \320\274\320\265\321\201\321\202\320\276", "FINISHED: place %d"), pl.finishPlace), SW / 2, SH * 0.36f + 8, 40, pl.out ? C(255, 90, 70) : ACC, 1);
    }
    drawHints(pl, showHelp);
    drawRadar(pl);
    drawTach(pl);
    if (countdown > 1.0f) {
        int n = (int)ceilf(countdown - 1.0f); float fr = (countdown - 1.0f) - (n - 1);
        float sc = 1.0f + fr * 0.6f;
        txtC(TextFormat("%d", n), SW / 2, SH / 2 - 110 * sc, 170 * sc, alpha(n == 1 ? C(255, 80, 60) : ACC, clampf(fr * 2, 0, 1)), 1);
        if (S.mode != GM_RACE) txtC(gameModeName(S.mode), SW / 2, SH / 2 + 90, 34, WHITE, 1);
        if (curBoss()) {
            txtC(TextFormat("%s: %s", T("\320\221\320\236\320\241\320\241", "BOSS"), bossName(curBoss())), SW / 2, SH / 2 + 90, 40, C(255, 80, 60), 1);
            txtC(T("\320\236\320\261\320\263\320\276\320\275\320\270\321\202\320\265 \320\261\320\276\321\201\321\201\320\260 \320\264\320\276 \321\204\320\270\320\275\320\270\321\210\320\260!", "Beat the boss to the line!"), SW / 2, SH / 2 + 138, 22, WHITE);
        }
        if (countdown < 2.3f) txtC(T("\320\223\320\260\320\267 \320\262 \320\274\320\276\320\274\320\265\320\275\321\202 \321\201\321\202\320\260\321\200\321\202\320\260 = \321\200\320\260\320\272\320\265\321\202\320\275\321\213\320\271 \321\201\321\202\320\260\321\200\321\202!", "Hit the gas right at GO for a rocket start!"), SW / 2, SH / 2 + 180, 18, alpha(C(200, 220, 255), 0.8f));
    } else if (countdown > 0.0f) txtC(T("\320\241\320\242\320\220\320\240\320\242!", "GO!"), SW / 2, SH / 2 - 100, 150, alpha(C(80, 255, 120), clampf(countdown * 2, 0, 1)), 1);
    if (finishBannerT > 0) {
        float a = clampf(finishBannerT, 0, 1);
        const char* big; const char* small; Color bc = ACC;
        if (S.mode == GM_PURSUIT) { big = resultState == 1 ? T("\320\222\320\253 \320\243\320\250\320\233\320\230!", "ESCAPED!") : T("\320\220\320\240\320\225\320\241\320\242\320\236\320\222\320\220\320\235!", "BUSTED!"); small = resultState == 1 ? T("\320\277\320\276\320\263\320\276\320\275\321\217 \320\276\320\272\320\276\320\275\321\207\320\265\320\275\320\260", "pursuit over") : T("\320\277\320\276\320\273\320\270\321\206\320\270\321\217 \320\262\320\260\321\201 \320\277\320\276\320\271\320\274\320\260\320\273\320\260", "the police got you"); if (resultState == 2) bc = C(255, 70, 60); }
        else if (S.mode == GM_ELIM) { big = resultState == 1 ? T("\320\237\320\236\320\221\320\225\320\224\320\220!", "WINNER!") : T("\320\222\320\253 \320\222\320\253\320\221\320\253\320\233\320\230", "ELIMINATED"); small = TextFormat(T("%d-\320\265 \320\274\320\265\321\201\321\202\320\276", "Place %d"), resultState == 1 ? 1 : pl.finishPlace); if (resultState == 2) bc = C(255, 70, 60); }
        else if (S.mode == GM_BATTLE) { big = resultState == 1 ? T("\320\237\320\236\320\221\320\225\320\224\320\220 \320\222 \320\221\320\230\320\242\320\222\320\225!", "BATTLE WON!") : T("\320\221\320\230\320\242\320\222\320\220 \320\237\320\240\320\236\320\230\320\223\320\240\320\220\320\235\320\220", "BATTLE LOST"); small = TextFormat(T("%d-\320\265 \320\274\320\265\321\201\321\202\320\276", "Place %d"), std::max(1, pl.finishPlace)); if (resultState == 2) bc = C(255, 70, 60); }
        else if (S.mode == GM_TIMETRIAL) { big = T("\320\244\320\230\320\235\320\230\320\250!", "FINISH!"); small = pl.finishTime <= ttTarget ? T("\321\206\320\265\320\273\321\214 \320\262\321\213\320\277\320\276\320\273\320\275\320\265\320\275\320\260!", "target beaten!") : T("\321\206\320\265\320\273\321\214 \320\275\320\265 \320\262\321\213\320\277\320\276\320\273\320\275\320\265\320\275\320\260", "target missed"); }
        else { big = T("\320\244\320\230\320\235\320\230\320\250!", "FINISH!"); small = TextFormat(T("%d-\320\265 \320\274\320\265\321\201\321\202\320\276", "Place %d"), pl.finishPlace); }
        panel(0, SH / 2 - 90, (float)SW, 150, 0.6f * a, 0);
        txtC(big, SW / 2, SH / 2 - 84, 90, alpha(bc, a), 1);
        txtC(small, SW / 2, SH / 2 + 8, 36, alpha(WHITE, a), 1);
    }
}

// =====================================================================
//                       MENUS / SCREENS
// =====================================================================
enum Screen { SC_MAIN = 0, SC_SETUP, SC_CAREER, SC_GARAGE, SC_SETTINGS, SC_CONTROLS, SC_RACE, SC_RESULTS, SC_REPLAY, SC_ACH };
static Screen screen = SC_MAIN;
static int menuSel = 0, setupRow = 0, careerSel = 0, garageRow = 0, setRow = 0, pauseSel = 0, resSel = 0;
static bool paused = false, quitGame = false, showHelp = false, photoMode = false, controlsOverlay = false;
static float menuT = 0; static int garageMode = 0; static int garageBase = 0;
static Racer showcase;

static bool gpPressed(int b) { return (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, b)) || (IsGamepadAvailable(1) && IsGamepadButtonPressed(1, b)); }
static bool kUp() { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || gpPressed(GAMEPAD_BUTTON_LEFT_FACE_UP); }
static bool kDown() { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || gpPressed(GAMEPAD_BUTTON_LEFT_FACE_DOWN); }
static bool kLeft() { return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || gpPressed(GAMEPAD_BUTTON_LEFT_FACE_LEFT); }
static bool kRight() { return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || gpPressed(GAMEPAD_BUTTON_LEFT_FACE_RIGHT); }
static bool kOk() { return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || gpPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN); }
static bool kBack() { return IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || gpPressed(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT); }
static bool mouseMoved() { Vector2 d = GetMouseDelta(); return d.x != 0 || d.y != 0; }

static bool menuItem(float x, float y, float w, float h, const char* label, bool sel, const char* value = nullptr, bool locked = false) {
    Rectangle hit = {x, y, w + 14, h};
    bool hover = CheckCollisionPointRec(GetMousePosition(), hit);
    float off = sel ? 14.0f : 0.0f;
    panel(x + off, y, w, h, sel ? 0.85f : 0.5f);
    if (sel) { accentBar(x + off - 6, y, 8, h, ACC, 14); accentBar(x + off, y + h - 3, w, 3, ACC); }
    Color lc = locked ? C(130, 130, 140) : (sel ? WHITE : C(200, 204, 214));
    txt(label, x + off + 26, y + h * 0.5f - h * 0.25f, h * 0.48f, lc, 1);
    if (value) {
        float vw = txtW(value, h * 0.42f);
        float vx = x + off + w - vw - 40;
        txt(value, vx, y + h * 0.5f - h * 0.22f, h * 0.42f, sel ? ACC : WHITE);
        if (sel) {
            DrawTriangle(v2(vx - 12, y + h / 2), v2(vx - 4, y + h / 2 + 7), v2(vx - 4, y + h / 2 - 7), ACC);
            DrawTriangle(v2(vx + vw + 12, y + h / 2), v2(vx + vw + 4, y + h / 2 - 7), v2(vx + vw + 4, y + h / 2 + 7), ACC);
        }
    }
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static bool mouseOver(float x, float y, float w, float h) { return CheckCollisionPointRec(GetMousePosition(), Rectangle{x, y, w + 14, h}); }
static void footerHints(const char* a, const char* b, const char* c) {
    float x = 40, y = SH - 50;
    x += keycap(x, y, "^", 28) + 4; x += keycap(x, y, "v", 28) + 8; txt(a, x, y + 5, 17, C(220, 224, 232)); x += txtW(a, 17) + 28;
    x += keycap(x, y, "ENTER", 28) + 8; txt(b, x, y + 5, 17, C(220, 224, 232)); x += txtW(b, 17) + 28;
    x += keycap(x, y, "ESC", 28) + 8; txt(c, x, y + 5, 17, C(220, 224, 232));
}
static void moneyBadge() {
    const char* m = TextFormat("$ %d", P.money);
    float w = txtW(m, 30, 1) + 40;
    panel(SW - w - 40, 30, w, 44, 0.65f);
    txt(m, SW - w - 14, 36, 30, C(120, 255, 150), 1);
}

static void drawShowroom(float t, Base b, Mode m, Color paint, float camDist, float camOffX, float camH = 3.6f) {
    ambCol = C(95, 100, 120); sunCol = C(255, 240, 225); lightDir = norm(vec(0.5f, 0.9f, 0.35f));
    fogCol = C(8, 9, 14); fogS = 60; fogE = 140; nightK = 1.0f; drawR = 200;
    DrawRectangleGradientV(0, 0, SW, SH, C(14, 16, 26), C(4, 4, 8));
    showcase.base = b; showcase.color = paint; showcase.isPlayer = true; showcase.dead = false; showcase.invuln = 0; showcase.hp = 100;
    showcase.helmet = DRIVERS[S.driver].helmet; showcase.driverStyle = DRIVERS[S.driver].style; showcase.decal = S.decal; showcase.color2 = PAINT[S.paint2]; showcase.number = 1;
    float dt = GetFrameTime();
    showcase.mode = m;
    showcase.morphAir += ((m == M_AIR ? 1.0f : 0.0f) - showcase.morphAir) * clampf(3 * dt, 0, 1);
    showcase.morphWater += ((m == M_WATER ? 1.0f : 0.0f) - showcase.morphWater) * clampf(3 * dt, 0, 1);
    showcase.wheelRot += dt * 3.0f; showcase.walk += dt * 2.0f; showcase.grounded = true; showcase.vel = vec(4, 0, 0);
    showcase.pos = vec(0, m == M_AIR ? 1.6f + sinf(t * 2.0f) * 0.25f : 0.0f, 0);
    showcase.yaw = t * 0.35f; showcase.pitch = m == M_AIR ? 0.05f : 0; showcase.roll = m == M_AIR ? sinf(t * 1.2f) * 0.12f : 0;
    showcase.in = Input(); showcase.steerV = sinf(t * 0.7f) * 0.5f; showcase.boosting = false;
    Camera3D c{}; c.projection = CAMERA_PERSPECTIVE; c.fovy = 34; c.up = vec(0, 1, 0);
    c.position = vec(camOffX + camDist * 0.85f, camH, -camDist * 0.6f); c.target = vec(camOffX, 1.3f, 0);
    camPos = c.position; camFwd = norm(sub(c.target, c.position));
    BeginMode3D(c);
    rlDisableBackfaceCulling();
    rlDrawRenderBatchActive(); rlDisableDepthTest();
    Color fl = C(22, 24, 32);
    rlCheckRenderBatchLimit(64 * 3);
    rlBegin(RL_TRIANGLES);
    for (int i = 0; i < 64; i++) {
        float a0 = 2 * PI_F * i / 64, a1 = 2 * PI_F * (i + 1) / 64;
        rlColor4ub(fl.r, fl.g, fl.b, 255); vtx(vec(0, 0, 0));
        rlColor4ub(4, 4, 8, 255); vtx(vec(cosf(a1) * 40, 0, sinf(a1) * 40)); vtx(vec(cosf(a0) * 40, 0, sinf(a0) * 40));
    }
    rlEnd();
    BeginBlendMode(BLEND_ADDITIVE);
    groundGlow(vec(0, 0.01f, 0), vec(7, 0, 0), vec(0, 0, 7), shade(paint, 0.35f));
    groundGlow(vec(0, 0.01f, 0), vec(4, 0, 0), vec(0, 0, 4), C(70, 70, 80));
    for (int k = 0; k < 48; k++) {
        float a = 2 * PI_F * k / 48 + t * 0.2f;
        groundGlow(vec(cosf(a) * 6.2f, 0.02f, sinf(a) * 6.2f), vec(0.35f, 0, 0), vec(0, 0, 0.35f), (k % 2) ? shade(ACC, 0.5f) : C(30, 30, 40));
    }
    EndBlendMode();
    BeginBlendMode(BLEND_ALPHA);
    groundGlow(vec(0, 0.03f, 0), vec(2.2f, 0, 0), vec(0, 0, 3.4f), C(0, 0, 0, m == M_AIR ? 90 : 200));
    EndBlendMode();
    rlDrawRenderBatchActive(); rlEnableDepthTest();
    gLightL = lightDir; gFog = 0;
    for (int k = -3; k <= 3; k++) {
        float a = k * 0.32f + PI_F * 0.85f;
        V3 p = vec(cosf(a) * 22, 0, sinf(a) * 22);
        rlPushMatrix(); rlTranslatef(p.x, 0, p.z); rlRotatef(-a * RAD2DEG + 90, 0, 1, 0);
        box(0, 6, 0, 5.5f, 12, 0.4f, C(30, 32, 42));
        box(0, 6, -0.25f, 0.18f, 10, 0.1f, k % 2 ? ACC : C(0, 200, 255), true);
        rlPopMatrix();
    }
    glows.clear(); bubbles.clear();
    drawVehicle(showcase, true);
    rlDrawRenderBatchActive(); rlDisableDepthMask(); BeginBlendMode(BLEND_ADDITIVE);
    for (int k = -3; k <= 3; k++) {
        float a = k * 0.32f + PI_F * 0.85f;
        for (int y = 2; y <= 10; y += 2) DrawBillboard(c, texGlow, vec(cosf(a) * 21.7f, (float)y, sinf(a) * 21.7f), 3.0f, shade(k % 2 ? ACC : C(0, 200, 255), 0.35f));
    }
    for (auto& g : glows) DrawBillboard(c, texGlow, g.p, g.s, g.c);
    for (int s = -1; s <= 1; s += 2) {
        V3 top = vec(s * 6.0f, 14, 3.0f);
        rlCheckRenderBatchLimit(36);
        rlBegin(RL_TRIANGLES);
        for (int i = 0; i < 12; i++) {
            float a0 = 2 * PI_F * i / 12, a1 = 2 * PI_F * (i + 1) / 12;
            rlColor4ub(16, 15, 13, 255); vtx(top);
            rlColor4ub(0, 0, 0, 255); vtx(vec(cosf(a0) * 5, 0, sinf(a0) * 5)); vtx(vec(cosf(a1) * 5, 0, sinf(a1) * 5));
        }
        rlEnd();
        DrawBillboard(c, texGlow, top, 5, C(255, 240, 210));
    }
    EndBlendMode(); rlDrawRenderBatchActive(); rlEnableDepthMask();
    EndMode3D();
    DrawRectangleGradientH(0, 0, SW / 2, SH, C(0, 0, 0, 200), C(0, 0, 0, 0));
}
static void drawTitle() {
    float x = 60, y = 40;
    txtS("MEGA RACE", x, y, 84, WHITE, 1);
    txtS("TURBO", x + txtW("MEGA RACE ", 84, 1), y + 10, 44, rainbow(menuT * 2), 1);
    accentBar(x, y + 92, 420, 6, ACC);
    txtS(T("\320\223\320\236\320\235\320\232\320\230-\320\242\320\240\320\220\320\235\320\241\320\244\320\236\320\240\320\234\320\225\320\240\320\253: \320\241\320\242\320\240\320\230\320\242 \320\230 \320\232\320\220\320\240\320\242\320\230\320\235\320\223", "TRANSFORMING STREET & KART RACING"), x + 4, y + 106, 22, ACC);
}
static void startRace();

static void updateDrawMain() {
    const int NI = 8;
    const char* items[NI] = {T("\320\232\320\220\320\240\320\254\320\225\320\240\320\220", "CAREER"), T("\320\221\320\253\320\241\320\242\320\240\320\220\320\257 \320\223\320\236\320\235\320\232\320\220", "QUICK RACE"), T("\320\227\320\220\320\224\320\220\320\235\320\230\320\225 \320\224\320\235\320\257", "DAILY CHALLENGE"), T("\320\223\320\220\320\240\320\220\320\226", "GARAGE"),
                             T("\320\224\320\236\320\241\320\242\320\230\320\226\320\225\320\235\320\230\320\257", "ACHIEVEMENTS"), T("\320\235\320\220\320\241\320\242\320\240\320\236\320\231\320\232\320\230", "SETTINGS"), T("\320\243\320\237\320\240\320\220\320\222\320\233\320\225\320\235\320\230\320\225", "CONTROLS"), T("\320\222\320\253\320\245\320\236\320\224", "EXIT")};
    if (kUp()) { menuSel = (menuSel + NI - 1) % NI; play(sClick); }
    if (kDown()) { menuSel = (menuSel + 1) % NI; play(sClick); }
    int activate = kOk() ? menuSel : -1;
    drawShowroom(menuT, S.base, M_LAND, PAINT[S.paint], S.base == B_TANK ? 15.0f : 13.0f, 2.6f);
    drawTitle();
    int day = todayId(); Daily dl = makeDaily(day); bool dailyDoneToday = P.dailyDay == day;
    for (int i = 0; i < NI; i++) {
        float y = 186 + i * 52;
        if (mouseOver(60, y, 360, 44) && mouseMoved()) menuSel = i;
        if (menuItem(60, y, 360, 44, items[i], menuSel == i)) activate = i;
        if (i == 2 && !dailyDoneToday) DrawCircleV(v2(menuSel == i ? 440.0f : 426.0f, y + 12), 7, fmodf(menuT, 1.0f) < 0.5f ? ACC : C(255, 90, 60));
        if (i == 4) { int n = 0; for (int a = 0; a < A_COUNT; a++) if (achDone(a)) n++; txt(TextFormat("%d/%d", n, A_COUNT), (menuSel == i ? 14 : 0) + 340.0f, y + 12, 18, C(255, 215, 70)); }
    }
    moneyBadge();
    panel(SW - 470, SH - 250, 420, 170, 0.55f);
    txt(T("\320\227\320\220\320\224\320\220\320\235\320\230\320\225 \320\224\320\235\320\257", "DAILY CHALLENGE"), SW - 440, SH - 238, 15, ACC);
    txt(dailyText(dl).c_str(), SW - 440, SH - 214, 17, WHITE);
    txt(dailyDoneToday ? T("\320\222\321\213\320\277\320\276\320\273\320\275\320\265\320\275\320\276! \320\235\320\276\320\262\320\276\320\265 \320\267\320\260\320\264\320\260\320\275\320\270\320\265 \320\267\320\260\320\262\321\202\321\200\320\260.", "Done! A new one tomorrow.") : TextFormat(T("\320\235\320\260\320\263\321\200\320\260\320\264\320\260: $%d", "Reward: $%d"), dl.reward), SW - 440, SH - 188, 16, dailyDoneToday ? C(120, 255, 150) : C(255, 215, 70));
    txt(T("\320\222\320\220\320\250 \320\242\320\240\320\220\320\235\320\241\320\237\320\236\320\240\320\242", "YOUR RIDE"), SW - 440, SH - 156, 15, ACC);
    txtS(TextFormat("%s  /  %s", baseName(S.base), driverName(S.driver)), SW - 442, SH - 136, 26, WHITE, 1);
    int done = 0; for (int i = 0; i < CAREER_N; i++) if (P.career[i]) done++;
    txt(TextFormat("%s: %d / %d   %s: %d", T("\320\232\320\260\321\200\321\214\320\265\321\200\320\260", "Career"), done, CAREER_N, T("\320\222\321\213\320\261\320\270\321\202\320\276", "Takedowns"), P.totalTakedowns), SW - 440, SH - 104, 16, C(210, 214, 222));
    footerHints(T("\320\262\321\213\320\261\320\276\321\200", "select"), T("\320\276\320\272", "ok"), T("\320\262\321\213\321\205\320\276\320\264", "quit"));
    if (activate == 0) { screen = SC_CAREER; play(sClick); }
    else if (activate == 1) { screen = SC_SETUP; play(sClick); }
    else if (activate == 2) {
        if (dailyDoneToday) { notify(T("\320\241\320\265\320\263\320\276\320\264\320\275\321\217\321\210\320\275\320\265\320\265 \320\267\320\260\320\264\320\260\320\275\320\270\320\265 \321\203\320\266\320\265 \320\262\321\213\320\277\320\276\320\273\320\275\320\265\320\275\320\276", "Today's challenge is already done"), C(120, 255, 150)); play(sClick, 1, 0.6f); }
        else { savedS = S; setupDaily(dl); careerEvent = -1; dailyActive = true; startRace(); }
    }
    else if (activate == 3) { screen = SC_GARAGE; garageBase = S.base; play(sClick); }
    else if (activate == 4) { screen = SC_ACH; play(sClick); }
    else if (activate == 5) { screen = SC_SETTINGS; play(sClick); }
    else if (activate == 6) { screen = SC_CONTROLS; play(sClick); }
    else if (activate == 7 || (kBack() && !IsKeyPressed(KEY_BACKSPACE))) quitGame = true;
}

static void updateDrawSetup() {
    const int ROWS = 11;
    if (kUp()) { setupRow = (setupRow + ROWS - 1) % ROWS; if (setupRow == 5 && S.players < 2) setupRow = 4; play(sClick); }
    if (kDown()) { setupRow = (setupRow + 1) % ROWS; if (setupRow == 5 && S.players < 2) setupRow = 6; play(sClick); }
    int dir = kLeft() ? -1 : kRight() ? 1 : 0;
    bool ok = kOk();
    drawShowroom(menuT, S.base, M_LAND, PAINT[S.paint], 14, 6.0f);
    txtS(T("\320\221\320\253\320\241\320\242\320\240\320\220\320\257 \320\223\320\236\320\235\320\232\320\220", "QUICK RACE"), 60, 40, 64, WHITE, 1); accentBar(60, 112, 380, 5, ACC);
    const char* tod[3] = {T("\320\235\320\236\320\247\320\254", "NIGHT"), T("\320\227\320\220\320\232\320\220\320\242", "SUNSET"), T("\320\224\320\225\320\235\320\254", "DAY")};
    const char* wth[6] = {T("\320\234\320\225\320\235\320\257\320\225\320\242\320\241\320\257", "DYNAMIC"), T("\320\257\320\241\320\235\320\236", "CLEAR"), T("\320\224\320\236\320\226\320\224\320\254", "RAIN"), T("\320\242\320\243\320\234\320\220\320\235", "FOG"), T("\320\241\320\235\320\225\320\223", "SNOW"), T("\320\223\320\240\320\236\320\227\320\220", "STORM")};
    bool trackLocked = S.track == 2 && !backroomsUnlocked();
    bool soloMode = S.mode == GM_TIMETRIAL || S.mode == GM_PURSUIT;
    std::string pl2 = S.players == 2 ? (soloMode ? std::string(T("1 (\321\200\320\265\320\266\320\270\320\274 \320\276\320\264\320\270\320\275\320\276\321\207\320\275\321\213\320\271)", "1 (solo mode)")) : std::string("2")) : std::string("1");
    std::string v[ROWS] = {gameModeName(S.mode), trackLocked ? "???" : trackName(S.track), S.mode == GM_BATTLE ? std::string("3:00") : std::string(TextFormat("%d", S.laps)), TextFormat("%d", S.opponents),
                           pl2, baseName(S.base2), tod[S.tod], wth[S.weather],
                           S.items ? T("\320\222\320\232\320\233", "ON") : T("\320\222\320\253\320\232\320\233", "OFF"), S.traffic ? T("\320\222\320\232\320\233", "ON") : T("\320\222\320\253\320\232\320\233", "OFF"), ""};
    const char* names[ROWS] = {T("\320\240\320\225\320\226\320\230\320\234", "MODE"), T("\320\242\320\240\320\220\320\241\320\241\320\220", "TRACK"), S.mode == GM_BATTLE ? T("\320\222\320\240\320\225\320\234\320\257", "TIME") : T("\320\232\320\240\320\243\320\223\320\230", "LAPS"), T("\320\241\320\236\320\237\320\225\320\240\320\235\320\230\320\232\320\230", "RIVALS"), T("\320\230\320\223\320\240\320\236\320\232\320\230", "PLAYERS"),
                               T("\320\242\320\240\320\220\320\235\320\241\320\237\320\236\320\240\320\242 \320\230\320\223\320\240\320\236\320\232\320\220 2", "PLAYER 2 RIDE"), T("\320\222\320\240\320\225\320\234\320\257 \320\241\320\243\320\242\320\236\320\232", "TIME OF DAY"), T("\320\237\320\236\320\223\320\236\320\224\320\220", "WEATHER"), T("\320\237\320\240\320\225\320\224\320\234\320\225\320\242\320\253", "ITEMS"), T("\320\242\320\240\320\220\320\244\320\230\320\232", "TRAFFIC"), T("\320\241\320\242\320\220\320\240\320\242!", "START!")};
    for (int i = 0, row = 0; i < ROWS; i++) {
        if (i == 5 && S.players < 2) continue;
        float y = 128 + row * 46; row++;
        if (mouseOver(60, y, 520, 40) && mouseMoved()) setupRow = i;
        if (menuItem(60, y, 520, 40, names[i], setupRow == i, i < ROWS - 1 ? v[i].c_str() : nullptr)) { setupRow = i; if (i == ROWS - 1) ok = true; else dir = 1; }
    }
    if (dir) {
        play(sClick);
        switch (setupRow) {
            case 0: S.mode = (S.mode + dir + GM_COUNT) % GM_COUNT; break;
            case 1: S.track = (S.track + dir + NUM_TRACKS) % NUM_TRACKS; break;
            case 2: S.laps = (S.laps + dir + 4) % 5 + 1; break;
            case 3: S.opponents = (S.opponents - 1 + dir + 7) % 7 + 1; break;
            case 4: S.players = S.players == 2 ? 1 : 2; break;
            case 5: { int b = S.base2; do { b = (b + dir + B_COUNT) % B_COUNT; } while (!P.owned[b]); S.base2 = (Base)b; } break;
            case 6: S.tod = (S.tod + dir + 3) % 3; break;
            case 7: S.weather = (S.weather + dir + 6) % 6; break;
            case 8: S.items = !S.items; break;
            case 9: S.traffic = !S.traffic; break;
        }
    }
    // info card
    float px = SW - 480, py = 150;
    panel(px, py, 430, 300, 0.6f);
    txt(trackLocked ? "???" : trackName(S.track), px + 28, py + 14, 30, ACC, 1);
    const char* desc[NUM_TRACKS] = {T("\320\235\320\276\321\207\320\275\320\276\320\271 \320\274\320\265\320\263\320\260\320\277\320\276\320\273\320\270\321\201: \321\210\320\276\321\201\321\201\320\265, \320\263\320\260\320\262\320\260\320\275\321\214, \320\272\320\260\320\275\321\214\320\276\320\275, \321\202\320\276\320\275\320\275\320\265\320\273\321\214 \320\270 \320\267\320\260\320\273\320\270\320\262.", "Night city: highway, harbor, canyon, tunnel and bay."),
                           T("\320\236\321\201\321\202\321\200\320\276\320\262\320\260, \320\277\320\260\320\273\321\214\320\274\321\213, \320\273\320\260\320\263\321\203\320\275\320\260 \320\270 \320\277\320\276\320\273\321\221\321\202 \320\275\320\260\320\264 \320\274\320\276\321\200\320\265\320\274.", "Islands, palms, a lagoon and a flight over the sea."),
                           T("\320\221\320\265\321\201\320\272\320\276\320\275\320\265\321\207\320\275\321\213\320\265 \320\266\321\221\320\273\321\202\321\213\320\265 \320\272\320\276\320\274\320\275\320\260\321\202\321\213 \320\270 \320\261\320\260\321\201\321\201\320\265\320\271\320\275\321\213\320\265 \320\267\320\260\320\273\321\213...", "Endless yellow rooms and the poolrooms..."),
                           T("\320\241\320\275\320\265\320\263, \321\201\320\272\320\276\320\273\321\214\320\267\320\272\320\276\320\265 \320\276\320\267\320\265\321\200\320\276, \320\277\320\276\320\273\321\221\321\202 \320\275\320\260\320\264 \320\277\320\270\320\272\320\260\320\274\320\270 \320\270 \321\204\321\214\320\276\321\200\320\264.", "Snow, a slippery frozen lake, peaks and a fjord."),
                           T("\320\237\321\203\321\201\321\202\321\213\320\275\321\217, \320\276\320\260\320\267\320\270\321\201, \320\273\320\260\320\262\320\260 \320\270 \320\277\320\276\320\273\321\221\321\202 \320\275\320\260\320\264 \320\272\321\200\320\260\321\202\320\265\321\200\320\276\320\274.", "Desert, an oasis, lava and a flight over the crater."),
                           T("\320\245\320\276\320\262\320\265\321\200\320\272\320\260\321\200\321\213, \320\277\320\273\320\260\320\267\320\274\320\260 \320\270 \320\276\321\202\320\272\321\200\321\213\321\202\321\213\320\271 \320\272\320\276\321\201\320\274\320\276\321\201. \320\235\320\270\320\267\320\272\320\260\321\217 \320\263\321\200\320\260\320\262\320\270\321\202\320\260\321\206\320\270\321\217!", "Hovercars, plasma and open space. Low gravity!")};
    txt(trackLocked ? T("\320\241\320\265\320\272\321\200\320\265\321\202\320\275\320\260\321\217 \321\202\321\200\320\260\321\201\321\201\320\260. \320\237\321\200\320\276\320\271\320\264\320\270\321\202\320\265 14-\320\271 \321\215\321\202\320\260\320\277 \320\272\320\260\321\200\321\214\320\265\321\200\321\213.", "Secret track. Clear career event 14.") : desc[S.track], px + 28, py + 56, 15, WHITE);
    if (S.track == 5 && (S.base == B_TANK || S.base == B_ROBOT)) txt(T("\320\222 \320\272\320\276\321\201\320\274\320\276\321\201\320\265 \321\202\320\260\320\275\320\272 \320\270 \321\200\320\276\320\261\320\276\321\202 \320\267\320\260\320\274\320\265\320\275\321\217\321\216\321\202\321\201\321\217 \321\205\320\276\320\262\320\265\321\200\320\272\320\260\321\200\320\276\320\274.", "In space the tank and robot become a hovercar."), px + 28, py + 268, 14, C(190, 150, 255));
    txt(TextFormat("%s: %s", T("\320\233\321\203\321\207\321\210\320\270\320\271 \320\272\321\200\321\203\320\263", "Best lap"), fmtTime(P.bestLap[S.track]).c_str()), px + 28, py + 90, 18, C(200, 205, 215));
    txt(TextFormat("%s: %s", T("\320\233\321\203\321\207\321\210\320\260\321\217 \320\263\320\276\320\275\320\272\320\260", "Best race"), fmtTime(P.bestRace[S.track]).c_str()), px + 28, py + 116, 18, C(200, 205, 215));
    const char* md[GM_COUNT] = {T("\320\236\320\261\321\213\321\207\320\275\320\260\321\217 \320\263\320\276\320\275\320\272\320\260 \321\201 \320\277\321\200\320\265\320\264\320\274\320\265\321\202\320\260\320\274\320\270 \320\270 \320\274\320\276\320\275\320\265\321\202\320\272\320\260\320\274\320\270.", "A race with items and coins."),
                         T("\320\236\320\264\320\270\320\275 \320\275\320\260 \321\202\321\200\320\260\321\201\321\201\320\265 \320\277\321\200\320\276\321\202\320\270\320\262 \320\277\321\200\320\270\320\267\321\200\320\260\320\272\320\260 \320\273\321\203\321\207\321\210\320\265\320\263\320\276 \320\272\321\200\321\203\320\263\320\260.", "Solo run against your best-lap ghost."),
                         T("\320\232\320\260\320\266\320\264\321\213\320\265 30 \321\201\320\265\320\272\321\203\320\275\320\264 \320\277\320\276\321\201\320\273\320\265\320\264\320\275\320\270\320\271 \320\262\321\213\320\261\321\213\320\262\320\260\320\265\321\202.", "Every 30 s the last racer is eliminated."),
                         T("\320\243\321\205\320\276\320\264\320\270\321\202\320\265 \320\276\321\202 \320\277\320\276\320\273\320\270\321\206\320\270\320\270, \320\277\320\276\320\272\320\260 \320\270\320\264\321\221\321\202 \321\202\320\260\320\271\320\274\320\265\321\200.", "Escape the police until the timer runs out."),
                         T("\320\243 \320\272\320\260\320\266\320\264\320\276\320\263\320\276 3 \321\210\320\260\321\200\320\270\320\272\320\260. \320\241\320\261\320\270\320\262\320\260\320\271 \321\207\321\203\320\266\320\270\320\265 \320\277\321\200\320\265\320\264\320\274\320\265\321\202\320\260\320\274\320\270 \320\270 \321\202\320\260\321\200\320\260\320\275\320\276\320\274!", "3 balloons each. Pop rivals with items and rams!")};
    txt(gameModeName(S.mode), px + 28, py + 160, 24, WHITE, 1);
    txt(md[S.mode], px + 28, py + 194, 15, C(220, 224, 232));
    txt(TextFormat("%s: %s", T("\320\242\321\200\320\260\320\275\321\201\320\277\320\276\321\200\321\202", "Vehicle"), baseName(S.base)), px + 28, py + 230, 18, ACC);
    footerHints(T("\320\262\321\213\320\261\320\276\321\200", "select"), T("\321\201\321\202\320\260\321\200\321\202", "start"), T("\320\275\320\260\320\267\320\260\320\264", "back"));
    if (ok && (setupRow == ROWS - 1 || IsKeyPressed(KEY_ENTER))) {
        if (trackLocked) { notify(T("\320\242\321\200\320\260\321\201\321\201\320\260 \320\267\320\260\320\272\321\200\321\213\321\202\320\260", "Track locked"), C(255, 90, 80)); play(sClick, 1, 0.5f); }
        else { careerEvent = -1; dailyActive = false; saveProfile(); startRace(); }
    }
    if (kBack()) { screen = SC_MAIN; play(sClick); saveProfile(); }
}
static void updateDrawCareer() {
    if (kUp()) { careerSel = (careerSel + CAREER_N - 1) % CAREER_N; play(sClick); }
    if (kDown()) { careerSel = (careerSel + 1) % CAREER_N; play(sClick); }
    bool ok = kOk();
    drawShowroom(menuT, S.base, M_LAND, PAINT[S.paint], 14, 6.5f);
    txtS(T("\320\232\320\220\320\240\320\254\320\225\320\240\320\220", "CAREER"), 60, 30, 60, WHITE, 1); accentBar(60, 96, 260, 5, ACC);
    moneyBadge();
    const int VIS = 12;
    int first = std::max(0, std::min(careerSel - VIS / 2, CAREER_N - VIS));
    if (first > 0) DrawTriangle(v2(300, 104), v2(290, 112), v2(310, 112), ACC);
    if (first + VIS < CAREER_N) DrawTriangle(v2(290, 116 + VIS * 44 - 2), v2(300, 124 + VIS * 44 - 2), v2(310, 116 + VIS * 44 - 2), ACC);
    for (int i = first; i < first + VIS; i++) {
        float y = 116 + (i - first) * 44;
        bool avail = careerAvailable(i);
        if (mouseOver(60, y, 520, 38) && mouseMoved()) careerSel = i;
        const char* nm = T(CAREER[i].ru, CAREER[i].en);
        if (menuItem(60, y, 520, 38, TextFormat("%d. %s", i + 1, avail ? nm : "???"), careerSel == i, nullptr, !avail)) { careerSel = i; ok = true; }
        int m = P.career[i];
        if (m) { Color mc = m == 1 ? C(255, 210, 60) : m == 2 ? C(210, 215, 225) : C(215, 140, 80); DrawCircleV(v2(560, y + 19), 11, mc); txtC(TextFormat("%d", m), 560, y + 9, 18, C(30, 30, 30), 1, false); }
        else if (!avail) { DrawRectangleRounded(Rectangle{550, y + 12, 20, 16}, 0.3f, 4, C(120, 120, 130)); DrawRing(v2(560, y + 12), 5, 8, 180, 360, 12, C(120, 120, 130)); }
    }
    const CareerEv& e = CAREER[careerSel];
    bool avail = careerAvailable(careerSel);
    float px = SW - 500, py = 120;
    panel(px, py, 450, 360, 0.65f);
    txt(avail ? T(e.ru, e.en) : "???", px + 28, py + 16, 28, ACC, 1);
    const char* tod[3] = {T("\320\275\320\276\321\207\321\214", "night"), T("\320\267\320\260\320\272\320\260\321\202", "sunset"), T("\320\264\320\265\320\275\321\214", "day")};
    const char* wth[6] = {T("\320\274\320\265\320\275\321\217\320\265\321\202\321\201\321\217", "dynamic"), T("\321\217\321\201\320\275\320\276", "clear"), T("\320\264\320\276\320\266\320\264\321\214", "rain"), T("\321\202\321\203\320\274\320\260\320\275", "fog"), T("\321\201\320\275\320\265\320\263", "snow"), T("\320\263\321\200\320\276\320\267\320\260", "storm")};
    float ly = py + 64;
    auto row = [&](const char* k, const char* v) { txt(k, px + 28, ly, 17, C(180, 186, 196)); txt(v, px + 200, ly, 17, WHITE); ly += 30; };
    row(T("\320\240\320\265\320\266\320\270\320\274", "Mode"), gameModeName(e.mode));
    row(T("\320\242\321\200\320\260\321\201\321\201\320\260", "Track"), e.track == 2 && !backroomsUnlocked() && !avail ? "???" : trackName(e.track));
    if (e.mode == GM_RACE || e.mode == GM_TIMETRIAL) row(T("\320\232\321\200\321\203\320\263\320\270", "Laps"), TextFormat("%d", e.laps));
    if (e.mode == GM_RACE || e.mode == GM_ELIM) row(T("\320\241\320\276\320\277\320\265\321\200\320\275\320\270\320\272\320\270", "Rivals"), TextFormat("%d", e.opp));
    if (e.mode == GM_PURSUIT) row(T("\320\237\321\200\320\276\320\264\320\265\321\200\320\266\320\260\321\202\321\214\321\201\321\217", "Survive"), fmtTime(e.goal).c_str());
    row(T("\320\243\321\201\320\273\320\276\320\262\320\270\321\217", "Conditions"), TextFormat("%s, %s", tod[e.tod], wth[e.weather]));
    row(T("\320\235\320\260\320\263\321\200\320\260\320\264\320\260", "Reward"), TextFormat("$ %d", e.reward));
    if (e.boss) row(T("\320\221\320\276\321\201\321\201", "Boss"), bossName(e.boss));
    const char* goal = e.boss ? T("\320\244\320\270\320\275\320\270\321\210\320\270\321\200\321\203\320\271\321\202\320\265 \321\200\320\260\320\275\321\214\321\210\320\265 \320\261\320\276\321\201\321\201\320\260!", "Finish ahead of the boss!") : e.mode == GM_RACE ? T("\320\227\320\260\320\271\320\274\320\270\321\202\320\265 \320\274\320\265\321\201\321\202\320\276 \320\262 \321\202\321\200\320\276\320\271\320\272\320\265", "Finish in the top 3") : e.mode == GM_ELIM ? T("\320\237\321\200\320\276\320\264\320\265\321\200\320\266\320\270\321\202\320\265\321\201\321\214 \320\264\320\276 \321\202\321\200\320\276\320\271\320\272\320\270", "Survive to the top 3")
                     : e.mode == GM_BATTLE ? T("\320\237\320\276\320\261\320\265\320\264\320\270\321\202\320\265 \320\262 \320\261\320\270\321\202\320\262\320\265", "Win the battle") : e.mode == GM_TIMETRIAL ? T("\320\243\320\273\320\276\320\266\320\270\321\202\320\265\321\201\321\214 \320\262 \321\206\320\265\320\273\320\265\320\262\320\276\320\265 \320\262\321\200\320\265\320\274\321\217", "Beat the target time") : T("\320\235\320\265 \320\277\320\276\320\277\320\260\320\264\320\270\321\202\320\265\321\201\321\214 \320\277\320\276\320\273\320\270\321\206\320\270\320\270", "Don't get busted");
    txt(goal, px + 28, ly + 10, 18, ACC);
    if (P.career[careerSel]) txt(T("\320\237\321\200\320\276\320\271\320\264\320\265\320\275\320\276! \320\234\320\276\320\266\320\275\320\276 \321\203\320\273\321\203\321\207\321\210\320\270\321\202\321\214 \321\200\320\265\320\267\321\203\320\273\321\214\321\202\320\260\321\202.", "Completed! You can improve it."), px + 28, ly + 44, 16, C(120, 255, 150));
    else if (!avail) txt(T("\320\241\320\275\320\260\321\207\320\260\320\273\320\260 \320\277\321\200\320\276\320\271\320\264\320\270\321\202\320\265 \320\277\321\200\320\265\320\264\321\213\320\264\321\203\321\211\320\270\320\271 \321\215\321\202\320\260\320\277.", "Clear the previous event first."), px + 28, ly + 44, 16, C(255, 120, 100));
    footerHints(T("\320\262\321\213\320\261\320\276\321\200", "select"), T("\321\201\321\202\320\260\321\200\321\202", "start"), T("\320\275\320\260\320\267\320\260\320\264", "back"));
    if (ok && avail) { savedS = S; careerEvent = careerSel; dailyActive = false; startRace(); }
    if (kBack()) { screen = SC_MAIN; play(sClick); }
}

static void statBar(float x, float y, const char* name, float v, float bonus = 0) {
    txt(name, x, y, 16, C(200, 204, 214));
    float bx = x + 150, w = 180;
    int on = (int)roundf(v * 10), on2 = (int)roundf(clampf(v + bonus, 0, 1.4f) * 10);
    for (int k = 0; k < 14; k++) {
        if (k >= 10 && k >= on2) break;
        Color c = k < on ? ACC : k < on2 ? C(120, 255, 150) : C(255, 255, 255, 40);
        accentBar(bx + k * (w / 10), y + 2, w / 10 - 4, 14, c, 4);
    }
}
// rows: 0 model, 1 form, 2 paint, 3 decal, 4 decal colour, 5 driver, 6..9 upgrades
static void cyclePaint(int dir) { int p = S.paint; do { p = (p + dir + NPAINT) % NPAINT; } while (!paintUnlocked(p)); S.paint = p; }
static void cycleDecal(int dir) { int d = S.decal; do { d = (d + dir + NDECAL) % NDECAL; } while (!decalUnlocked(d)); S.decal = d; if (d) unlockAch(A_STYLE); }
static void updateDrawGarage() {
    const int ROWS = 10;
    if (kUp()) { garageRow = (garageRow + ROWS - 1) % ROWS; play(sClick); }
    if (kDown()) { garageRow = (garageRow + 1) % ROWS; play(sClick); }
    int dir = kLeft() ? -1 : kRight() ? 1 : 0;
    bool buy = IsKeyPressed(KEY_ENTER) || gpPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    if (dir) {
        play(sClick);
        if (garageRow == 0) { garageBase = (garageBase + B_COUNT + dir) % B_COUNT; if (P.owned[garageBase]) S.base = (Base)garageBase; }
        else if (garageRow == 1) garageMode = (garageMode + 3 + dir) % 3;
        else if (garageRow == 2) cyclePaint(dir);
        else if (garageRow == 3) cycleDecal(dir);
        else if (garageRow == 4) S.paint2 = (S.paint2 + dir + NPAINT_FREE) % NPAINT_FREE;
        else if (garageRow == 5) S.driver = (S.driver + dir + NDRIVER) % NDRIVER;
    }
    Base gb = (Base)garageBase;
    drawShowroom(menuT, gb, (Mode)garageMode, PAINT[S.paint], (gb == B_TANK ? 15.0f : gb == B_KART ? 11.5f : 12.5f) * (garageMode == 2 ? 1.4f : garageMode == 1 ? 1.12f : 1.0f), garageMode == 2 ? 3.5f : 2.0f, garageMode == 2 ? 8.0f : 4.2f);
    txtS(T("\320\223\320\220\320\240\320\220\320\226", "GARAGE"), 60, 30, 60, WHITE, 1); accentBar(60, 96, 220, 5, ACC);
    moneyBadge();
    const char* modes[3] = {T("\320\227\320\225\320\234\320\233\320\257", "LAND"), T("\320\222\320\236\320\224\320\220", "WATER"), T("\320\222\320\236\320\227\320\224\320\243\320\245", "AIR")};
    bool owned = P.owned[gb] != 0;
    std::string modelVal = owned ? baseName(gb) : TextFormat("%s  $%d", baseName(gb), VPRICE[gb]);
    const float RY = 110, RS = 46, RH = 40;
    if (menuItem(60, RY, 440, RH, T("\320\234\320\236\320\224\320\225\320\233\320\254", "MODEL"), garageRow == 0, modelVal.c_str())) { garageRow = 0; garageBase = (garageBase + 1) % B_COUNT; if (P.owned[garageBase]) S.base = (Base)garageBase; }
    if (menuItem(60, RY + RS, 440, RH, T("\320\244\320\236\320\240\320\234\320\220", "FORM"), garageRow == 1, modes[garageMode])) { garageRow = 1; garageMode = (garageMode + 1) % 3; }
    std::string pn = S.paint >= NPAINT_FREE ? std::string(paintName(S.paint)) + "      " : std::string("      ");
    if (menuItem(60, RY + RS * 2, 440, RH, T("\320\246\320\222\320\225\320\242", "PAINT"), garageRow == 2, pn.c_str())) { garageRow = 2; cyclePaint(1); }
    DrawRectangleRounded(Rectangle{(garageRow == 2 ? 14.0f : 0.0f) + 60 + 440 - 92, RY + RS * 2 + 10, 48, 20}, 0.4f, 6, PAINT[S.paint]);
    if (menuItem(60, RY + RS * 3, 440, RH, T("\320\235\320\220\320\232\320\233\320\225\320\231\320\232\320\220", "DECAL"), garageRow == 3, decalName(S.decal))) { garageRow = 3; cycleDecal(1); }
    if (menuItem(60, RY + RS * 4, 440, RH, T("\320\246\320\222\320\225\320\242 \320\235\320\220\320\232\320\233\320\225\320\231\320\232\320\230", "DECAL COLOUR"), garageRow == 4, "      ")) { garageRow = 4; S.paint2 = (S.paint2 + 1) % NPAINT_FREE; }
    DrawRectangleRounded(Rectangle{(garageRow == 4 ? 14.0f : 0.0f) + 60 + 440 - 92, RY + RS * 4 + 10, 48, 20}, 0.4f, 6, PAINT[S.paint2]);
    if (menuItem(60, RY + RS * 5, 440, RH, T("\320\223\320\236\320\235\320\251\320\230\320\232", "DRIVER"), garageRow == 5, driverName(S.driver))) { garageRow = 5; S.driver = (S.driver + 1) % NDRIVER; }
    const char* un[4] = {T("\320\224\320\222\320\230\320\223\320\220\320\242\320\225\320\233\320\254", "ENGINE"), T("\320\235\320\230\320\242\320\240\320\236", "NITRO"), T("\320\221\320\240\320\236\320\235\320\257", "ARMOR"), T("\320\243\320\237\320\240\320\220\320\222\320\233\320\225\320\235\320\230\320\225", "HANDLING")};
    for (int u = 0; u < 4; u++) {
        float y = RY + RS * (6 + u) + 8;
        int lvl = P.upg[u];
        std::string val = lvl >= 5 ? std::string(T("\320\234\320\220\320\232\320\241", "MAX")) : std::string(TextFormat("$%d", UPCOST[lvl]));
        if (menuItem(60, y, 440, RH, un[u], garageRow == 6 + u, val.c_str())) { garageRow = 6 + u; buy = true; }
        for (int k = 0; k < 5; k++) DrawRectangle(60 + (garageRow == 6 + u ? 14 : 0) + 230 + k * 14, (int)y + 15, 10, 10, k < lvl ? C(120, 255, 150) : C(70, 72, 80));
    }
    if (buy) {
        if (garageRow == 0 && !owned) {
            if (P.money >= VPRICE[gb]) { P.money -= VPRICE[gb]; P.owned[gb] = 1; S.base = gb; play(sCash); notify(T("\320\232\320\243\320\237\320\233\320\225\320\235\320\236!", "PURCHASED!"), C(120, 255, 150)); saveProfile(); }
            else { play(sClick, 1, 0.5f); notify(T("\320\235\320\225 \320\245\320\222\320\220\320\242\320\220\320\225\320\242 \320\224\320\225\320\235\320\225\320\223", "NOT ENOUGH MONEY"), C(255, 90, 80)); }
        } else if (garageRow >= 6) {
            int u = garageRow - 6, lvl = P.upg[u];
            if (lvl < 5 && P.money >= UPCOST[lvl]) { P.money -= UPCOST[lvl]; P.upg[u]++; play(sCash); checkAchievements(); saveProfile(); }
            else if (lvl < 5) { play(sClick, 1, 0.5f); notify(T("\320\235\320\225 \320\245\320\222\320\220\320\242\320\220\320\225\320\242 \320\224\320\225\320\235\320\225\320\223", "NOT ENOUGH MONEY"), C(255, 90, 80)); }
        }
    }
    float spx = SW - 440, spy = SH - 262;
    panel(spx - 26, spy - 12, 420, 250, 0.6f);
    txt(T("\320\245\320\220\320\240\320\220\320\232\320\242\320\225\320\240\320\230\320\241\320\242\320\230\320\232\320\230", "STATS"), spx, spy, 16, ACC);
    const char* sn[4] = {T("\320\241\320\272\320\276\321\200\320\276\321\201\321\202\321\214", "Top speed"), T("\320\240\320\260\320\267\320\263\320\276\320\275", "Acceleration"), T("\320\243\320\277\321\200\320\260\320\262\320\273\320\265\320\275\320\270\320\265", "Handling"), T("\320\221\321\200\320\276\320\275\321\217", "Armor")};
    float bonus[4] = {P.upg[0] * 0.035f, P.upg[0] * 0.07f, P.upg[3] * 0.06f, P.upg[2] * 0.08f};
    for (int k = 0; k < 4; k++) statBar(spx, spy + 30 + k * 30, sn[k], GSTAT[gb][k], bonus[k]);
    const char* desc = gb == B_CAR ? T("\320\221\321\213\321\201\321\202\321\200\320\260\321\217 \320\270 \321\206\320\265\320\277\320\272\320\260\321\217. \320\227\320\260\320\275\320\276\321\201 (SPACE) \320\267\320\260\321\200\321\217\320\266\320\260\320\265\321\202 \321\202\321\203\321\200\320\261\320\276.", "Fast and grippy. Drift (SPACE) charges a boost.")
                     : gb == B_TANK ? T("\320\242\321\217\320\266\321\221\320\273\321\213\320\271 \321\202\320\260\321\200\320\260\320\275. SPACE \342\200\224 \320\262\321\213\321\201\321\202\321\200\320\265\320\273 \320\277\321\203\321\210\320\272\320\270.", "Heavy rammer. SPACE fires the cannon.")
                     : gb == B_ROBOT ? T("\320\250\320\260\320\263 \320\262\320\261\320\276\320\272 (Q/E), \320\277\321\200\321\213\320\266\320\276\320\272 (SPACE), \321\200\321\213\320\262\320\276\320\272 (F).", "Strafe (Q/E), jump (SPACE), dash (F).")
                                     : T("\320\233\321\221\320\263\320\272\320\270\320\271 \320\270 \320\262\321\221\321\200\321\202\320\272\320\270\320\271 \320\272\320\260\321\200\321\202. \320\234\320\270\320\275\320\270-\321\202\321\203\321\200\320\261\320\276 \320\262 \320\267\320\260\320\275\320\276\321\201\320\260\321\205!", "Light, nimble kart. Mini-turbos in drifts!");
    txt(desc, spx, spy + 158, 15, WHITE);
    txt(T("\320\235\320\260 \320\262\320\276\320\264\320\265 \342\200\224 \320\272\320\260\321\202\320\265\321\200, \320\262 \320\267\320\276\320\275\320\265 \320\262\320\267\320\273\321\221\321\202\320\260 \342\200\224 \321\201\320\260\320\274\320\276\320\273\321\221\321\202.", "Boat on water, plane in air zones."), spx, spy + 184, 14, C(180, 186, 196));
    if (garageRow >= 2 && garageRow <= 3) txt(T("\320\227\320\276\320\273\320\276\321\202\320\276, \321\205\321\200\320\276\320\274, \302\253\320\234\320\260\320\263\320\274\320\260\302\273 \320\270 \320\272\320\276\321\200\320\276\320\275\320\260 \342\200\224 \320\275\320\260\320\263\321\200\320\260\320\264\321\213 \320\267\320\260 \320\261\320\276\321\201\321\201\320\276\320\262.", "Gold, chrome, Magma and the crown are boss rewards."), spx, spy + 228, 14, C(255, 210, 90));
    if (!owned) txt(T("ENTER \342\200\224 \320\272\321\203\320\277\320\270\321\202\321\214", "ENTER - buy"), spx, spy + 208, 16, ACC);
    float x = 40, y = SH - 50;
    x += keycap(x, y, "<", 28) + 4; x += keycap(x, y, ">", 28) + 8; txt(T("\320\270\320\267\320\274\320\265\320\275\320\270\321\202\321\214", "change"), x, y + 5, 17, C(220, 224, 232)); x += 110;
    x += keycap(x, y, "ENTER", 28) + 8; txt(T("\320\272\321\203\320\277\320\270\321\202\321\214", "buy"), x, y + 5, 17, C(220, 224, 232)); x += 90;
    x += keycap(x, y, "ESC", 28) + 8; txt(T("\320\275\320\260\320\267\320\260\320\264", "back"), x, y + 5, 17, C(220, 224, 232));
    if (kBack()) { if (!P.owned[garageBase]) garageBase = S.base; screen = SC_MAIN; play(sClick); saveProfile(); }
}
static void updateDrawSettings() {
    int rows = cyrOK ? 10 : 9;
    if (kUp()) { setRow = (setRow + rows - 1) % rows; play(sClick); }
    if (kDown()) { setRow = (setRow + 1) % rows; play(sClick); }
    int dir = kLeft() ? -1 : (kRight() || IsKeyPressed(KEY_ENTER)) ? 1 : 0;
    drawShowroom(menuT, S.base, M_LAND, PAINT[S.paint], 13, 5.0f);
    txtS(T("\320\235\320\220\320\241\320\242\320\240\320\236\320\231\320\232\320\230", "SETTINGS"), 60, 30, 60, WHITE, 1); accentBar(60, 96, 300, 5, ACC);
    const char* diff[3] = {T("\320\233\320\201\320\223\320\232\320\220\320\257", "EASY"), T("\320\235\320\236\320\240\320\234\320\220\320\233\320\254\320\235\320\220\320\257", "NORMAL"), T("\320\241\320\233\320\236\320\226\320\235\320\220\320\257", "HARD")};
    const char* qual[3] = {T("\320\235\320\230\320\227\320\232\320\220\320\257", "LOW"), T("\320\241\320\240\320\225\320\224\320\235\320\257\320\257", "MEDIUM"), T("\320\222\320\253\320\241\320\236\320\232\320\220\320\257", "HIGH")};
    std::string v[10] = {diff[S.difficulty], S.sound ? T("\320\222\320\232\320\233", "ON") : T("\320\222\320\253\320\232\320\233", "OFF"), TextFormat("%d / 10", S.music),
                         S.musicSrc ? T("\320\241\320\222\320\236\320\257 (\320\277\320\260\320\277\320\272\320\260 music)", "OWN (music folder)") : T("\320\222\320\241\320\242\320\240\320\236\320\225\320\235\320\235\320\220\320\257", "BUILT-IN"), qual[S.quality],
                         S.post ? T("\320\222\320\232\320\233", "ON") : T("\320\222\320\253\320\232\320\233", "OFF"), S.vibration ? T("\320\222\320\232\320\233", "ON") : T("\320\222\320\253\320\232\320\233", "OFF"), S.showFps ? T("\320\222\320\232\320\233", "ON") : T("\320\222\320\253\320\232\320\233", "OFF"),
                         S.fullscreen ? T("\320\224\320\220", "YES") : T("\320\235\320\225\320\242", "NO"), langSel == 0 ? "\320\240\320\243\320\241\320\241\320\232\320\230\320\231" : "ENGLISH"};
    const char* names[10] = {T("\320\241\320\233\320\236\320\226\320\235\320\236\320\241\320\242\320\254", "DIFFICULTY"), T("\320\227\320\222\320\243\320\232", "SOUND"), T("\320\223\320\240\320\236\320\234\320\232\320\236\320\241\320\242\320\254 \320\234\320\243\320\227\320\253\320\232\320\230", "MUSIC VOLUME"), T("\320\234\320\243\320\227\320\253\320\232\320\220", "MUSIC"), T("\320\232\320\220\320\247\320\225\320\241\320\242\320\222\320\236 \320\223\320\240\320\220\320\244\320\230\320\232\320\230", "GRAPHICS QUALITY"),
                             T("\320\255\320\244\320\244\320\225\320\232\320\242\320\253 (\320\241\320\222\320\225\320\247\320\225\320\235\320\230\320\225)", "EFFECTS (BLOOM)"), T("\320\222\320\230\320\221\320\240\320\220\320\246\320\230\320\257 \320\223\320\225\320\231\320\234\320\237\320\220\320\224\320\220", "GAMEPAD RUMBLE"), T("\320\237\320\236\320\232\320\220\320\227\320\253\320\222\320\220\320\242\320\254 FPS", "SHOW FPS"), T("\320\237\320\236\320\233\320\235\320\253\320\231 \320\255\320\232\320\240\320\220\320\235", "FULLSCREEN"), T("\320\257\320\227\320\253\320\232", "LANGUAGE")};
    for (int i = 0; i < rows; i++) {
        if (mouseOver(60, 116 + i * 52, 600, 44) && mouseMoved()) setRow = i;
        if (menuItem(60, 116 + i * 52, 600, 44, names[i], setRow == i, v[i].c_str())) { setRow = i; dir = 1; }
    }
    const char* tip = setRow == 3 ? T("\320\237\320\276\320\273\320\276\320\266\320\270\321\202\320\265 mp3 / ogg / wav \320\262 \320\277\320\260\320\277\320\272\321\203 music \321\200\321\217\320\264\320\276\320\274 \321\201 \320\270\320\263\321\200\320\276\320\271. \320\222 \320\263\320\276\320\275\320\272\320\265 M \342\200\224 \321\201\320\273\320\265\320\264\321\203\321\216\321\211\320\270\320\271 \321\202\321\200\320\265\320\272.", "Put mp3 / ogg / wav files into the music folder next to the game. M skips a track.")
                    : setRow == 4 ? T("\320\235\320\270\320\267\320\272\320\260\321\217: \320\274\320\265\320\275\321\214\321\210\320\265 \320\264\320\260\320\273\321\214\320\275\320\276\321\201\321\202\321\214 \320\270 \321\207\320\260\321\201\321\202\320\270\321\206, \320\272\320\260\321\200\321\202\320\270\320\275\320\272\320\260 \321\200\320\265\320\275\320\264\320\265\321\200\320\270\321\202\321\201\321\217 \320\262 \320\277\320\276\320\275\320\270\320\266\320\265\320\275\320\275\320\276\320\274 \321\200\320\260\320\267\321\200\320\265\321\210\320\265\320\275\320\270\320\270.", "Low: shorter draw distance, fewer particles, lower render resolution.")
                    : setRow == 6 ? T("\320\242\321\200\321\217\321\201\320\272\320\260 \320\263\320\265\320\271\320\274\320\277\320\260\320\264\320\260 \320\277\321\200\320\270 \321\203\320\264\320\260\321\200\320\260\321\205, \320\277\321\200\320\270\320\267\320\265\320\274\320\273\320\265\320\275\320\270\321\217\321\205 \320\270 \321\202\321\203\321\200\320\261\320\276.", "Gamepad rumble on crashes, landings and boosts.") : "";
    if (tip[0]) txt(tip, 60, 640, 16, C(200, 205, 215));
    if (dir) {
        play(sClick);
        switch (setRow) {
            case 0: S.difficulty = (S.difficulty + dir + 3) % 3; break;
            case 1: S.sound = !S.sound; break;
            case 2: S.music = (S.music + dir + 11) % 11; break;
            case 3: S.musicSrc = !S.musicSrc; break;
            case 4: S.quality = (S.quality + dir + 3) % 3; break;
            case 5: S.post = !S.post; break;
            case 6: S.vibration = !S.vibration; if (S.vibration) rumblePad(0, 0.6f, 0.25f); break;
            case 7: S.showFps = !S.showFps; break;
            case 8: S.fullscreen = !S.fullscreen; ToggleFullscreen(); break;
            case 9: langSel = 1 - langSel; break;
        }
        saveProfile();
    }
    footerHints(T("\320\262\321\213\320\261\320\276\321\200", "select"), T("\320\270\320\267\320\274\320\265\320\275\320\270\321\202\321\214", "change"), T("\320\275\320\260\320\267\320\260\320\264", "back"));
    if (kBack()) { screen = SC_MAIN; play(sClick); }
}
static void drawControlsPanel(float x0, float y0) {
    struct Row { const char* k1; const char* k2; const char* ru; const char* en; const char* pad; };
    const Row rows[] = {
        {"W", "S", "\320\263\320\260\320\267 / \321\202\320\276\321\200\320\274\320\276\320\267 (\320\262 \320\262\320\276\320\267\320\264\321\203\321\205\320\265: \320\262\320\262\320\265\321\200\321\205 / \320\262\320\275\320\270\320\267)", "gas / brake (air: climb / dive)", "RT / LT"},
        {"A", "D", "\321\200\321\203\320\273\321\214, \320\272\321\200\320\265\320\275", "steer, bank", "L-STICK"},
        {"SHIFT", nullptr, "\320\275\320\270\321\202\321\200\320\276 / \321\204\320\276\321\200\321\201\320\260\320\266", "nitro / afterburner", "X"},
        {"SPACE", nullptr, "\320\267\320\260\320\275\320\276\321\201 (= \320\274\320\270\320\275\320\270-\321\202\321\203\321\200\320\261\320\276), \320\262\321\213\321\201\321\202\321\200\320\265\320\273, \320\277\321\200\321\213\320\266\320\276\320\272, \321\202\321\200\321\216\320\272", "drift (= mini-turbo), fire, jump, trick", "A"},
        {"R", nullptr, "\320\270\321\201\320\277\320\276\320\273\321\214\320\267\320\276\320\262\320\260\321\202\321\214 \320\277\321\200\320\265\320\264\320\274\320\265\321\202", "use item", "B"},
        {"Q", "E", "\321\200\320\276\320\261\320\276\321\202: \321\210\320\260\320\263 \320\262\320\261\320\276\320\272", "robot: strafe", "R-STICK"},
        {"F", nullptr, "\321\200\320\276\320\261\320\276\321\202: \321\200\321\213\320\262\320\276\320\272-\321\202\320\260\321\200\320\260\320\275", "robot: ram dash", "RB"},
        {"1", "4", "\321\202\321\200\320\260\320\275\321\201\320\277\320\276\321\200\321\202: \320\274\320\260\321\210\320\270\320\275\320\260, \321\202\320\260\320\275\320\272, \321\200\320\276\320\261\320\276\321\202, \320\272\320\260\321\200\321\202", "vehicle: car, tank, robot, kart", "D-PAD"},
        {"V", nullptr, "\320\272\320\260\320\274\320\265\321\200\320\260", "camera", "Y"},
        {"C", nullptr, "\320\277\320\276\321\201\320\274\320\276\321\202\321\200\320\265\321\202\321\214 \320\275\320\260\320\267\320\260\320\264", "look back", "LB"},
        {"T", nullptr, "\320\262\320\265\321\200\320\275\321\203\321\202\321\214\321\201\321\217 \320\275\320\260 \321\202\321\200\320\260\321\201\321\201\321\203", "reset to track", "BACK"},
        {"ESC", nullptr, "\320\277\320\260\321\203\320\267\320\260 / \321\204\320\276\321\202\320\276\321\200\320\265\320\266\320\270\320\274", "pause / photo mode", "START"},
        {"M", nullptr, "\321\201\320\273\320\265\320\264\321\203\321\216\321\211\320\270\320\271 \321\202\321\200\320\265\320\272 \321\201\320\262\320\276\320\265\320\271 \320\274\321\203\320\267\321\213\320\272\320\270", "next track (own music)", ""}};
    float y = y0;
    for (const Row& r : rows) {
        float x = x0;
        x += keycap(x, y, r.k1, 28) + 4;
        if (r.k2) x += keycap(x, y, r.k2, 28) + 4;
        txt(T(r.ru, r.en), x0 + 120, y + 5, 16, WHITE);
        if (r.pad[0]) txt(r.pad, x0 + 520, y + 6, 14, C(150, 200, 255));
        y += 33;
    }
    txt(T("\320\230\320\223\320\240\320\236\320\232 2 (\320\262\320\264\320\262\320\276\321\221\320\274): \321\201\321\202\321\200\320\265\320\273\320\272\320\270, R-SHIFT \320\275\320\270\321\202\321\200\320\276, R-CTRL \320\267\320\260\320\275\320\276\321\201/\320\262\321\213\321\201\321\202\321\200\320\265\320\273, / \320\277\321\200\320\265\320\264\320\274\320\265\321\202, . \321\200\321\213\320\262\320\276\320\272, 7-0 \321\202\321\200\320\260\320\275\321\201\320\277\320\276\321\200\321\202, \\ \320\275\320\260 \321\202\321\200\320\260\321\201\321\201\321\203",
          "PLAYER 2: arrows, R-SHIFT nitro, R-CTRL drift/fire, / item, . dash, 7-0 vehicle, \\ reset"), x0, y + 4, 14, C(255, 170, 150));
    txt(T("\320\241 \320\276\320\264\320\275\320\270\320\274 \320\263\320\265\320\271\320\274\320\277\320\260\320\264\320\276\320\274 \320\276\320\275 \321\203 \320\270\320\263\321\200\320\276\320\272\320\260 2, \321\201 \320\264\320\262\321\203\320\274\321\217 \342\200\224 \321\203 \320\272\320\260\320\266\320\264\320\276\320\263\320\276 \321\201\320\262\320\276\320\271.", "One gamepad goes to player 2; with two, each player gets one."), x0, y + 24, 14, C(200, 205, 215));
    x0 = x0 + 640; y = y0;
    txt(T("\320\237\320\240\320\225\320\224\320\234\320\225\320\242\320\253 (\320\272\320\276\321\200\320\276\320\261\320\272\320\270 \302\253?\302\273)", "ITEMS (\"?\" boxes)"), x0, y, 16, ACC); y += 28;
    struct I { int it; const char* ru; const char* en; };
    const I items[6] = {{IT_TURBO, "\321\202\321\203\321\200\320\261\320\276-\321\203\321\201\320\272\320\276\321\200\320\265\320\275\320\270\320\265", "turbo boost"}, {IT_ROCKET, "\321\201\320\260\320\274\320\276\320\275\320\260\320\262\320\276\320\264\321\217\321\211\320\260\321\217\321\201\321\217 \321\200\320\260\320\272\320\265\321\202\320\260", "homing rocket"},
                        {IT_OIL, "\320\273\321\203\320\266\320\260 \320\274\320\260\321\201\320\273\320\260 \342\200\224 \320\267\320\260\320\275\320\276\321\201 \321\201\320\267\320\260\320\264\320\270", "oil slick behind you"}, {IT_SHIELD, "\321\211\320\270\321\202 \320\276\321\202 \320\276\320\264\320\275\320\276\320\263\320\276 \321\203\320\264\320\260\321\200\320\260", "shield blocks one hit"},
                        {IT_BOMB, "\320\274\320\270\320\275\320\260 \320\267\320\260 \321\201\320\276\320\261\320\276\320\271", "mine dropped behind"}, {IT_LIGHTNING, "\320\274\320\276\320\273\320\275\320\270\321\217 \321\203\320\274\320\265\320\275\321\214\321\210\320\260\320\265\321\202 \320\273\320\270\320\264\320\265\321\200\320\276\320\262", "lightning shrinks leaders"}};
    for (const I& it : items) { iconItem(it.it, x0 + 14, y + 12, 12); txt(T(it.ru, it.en), x0 + 36, y + 4, 16, WHITE); y += 32; }
    y += 10;
    txt(T("\320\227\320\236\320\235\320\253 \320\242\320\240\320\220\320\241\320\241\320\253", "TRACK ZONES"), x0, y, 16, ACC); y += 28;
    struct Z { Mode m; const char* ru; const char* en; };
    const Z zs[3] = {{M_WATER, "\321\201\320\270\320\275\320\270\320\265 \320\262\320\276\321\200\320\276\321\202\320\260 \342\200\224 \320\262\320\276\320\264\320\260: \320\262\321\213 \321\201\321\202\320\260\320\275\320\276\320\262\320\270\321\202\320\265\321\201\321\214 \320\272\320\260\321\202\320\265\321\200\320\276\320\274", "blue gates - water: you become a boat"},
                     {M_AIR, "\321\204\320\270\320\276\320\273\320\265\321\202\320\276\320\262\321\213\320\265 \342\200\224 \320\262\320\267\320\273\321\221\321\202: \320\273\320\265\321\202\320\270\321\202\320\265 \321\207\320\265\321\200\320\265\320\267 \320\272\320\276\320\273\321\214\321\206\320\260", "purple gates - air: fly through the rings"},
                     {M_LAND, "\320\276\321\200\320\260\320\275\320\266\320\265\320\262\321\213\320\265 \342\200\224 \321\201\320\275\320\276\320\262\320\260 \320\275\320\260 \320\267\320\265\320\274\320\273\321\216", "orange gates - back on land"}};
    for (const Z& z : zs) {
        DrawRectangleRounded(Rectangle{x0, y + 2, 26, 26}, 0.3f, 6, gateColor(z.m));
        if (z.m == M_WATER) iconBoat(x0 + 13, y + 15, 9, WHITE); else if (z.m == M_AIR) iconPlane(x0 + 13, y + 15, 9, WHITE); else iconCar(x0 + 13, y + 15, 9, WHITE);
        txt(T(z.ru, z.en), x0 + 40, y + 6, 15, WHITE); y += 34;
    }
    y += 6;
    txt(T("\320\234\320\276\320\275\320\265\321\202\320\272\320\270 \320\264\320\260\321\216\321\202 \321\201\320\272\320\276\321\200\320\276\321\201\321\202\321\214 (\320\264\320\276 10). \320\242\321\200\320\260\320\274\320\277\320\273\320\270\320\275 + SPACE = \321\202\321\200\321\216\320\272 \320\270 \321\202\321\203\321\200\320\261\320\276.", "Coins add speed (max 10). Ramp + SPACE = trick and boost."), x0, y, 14, C(200, 205, 215));
    txt(T("\320\225\320\267\320\264\320\260 \320\262\320\277\320\273\320\276\321\202\320\275\321\203\321\216 \320\267\320\260 \321\201\320\276\320\277\320\265\321\200\320\275\320\270\320\272\320\276\320\274 = \321\201\320\273\320\270\320\277\321\201\321\202\321\200\320\270\320\274.", "Tuck in behind a rival = slipstream."), x0, y + 22, 14, C(200, 205, 215));
    txt(T("\320\226\321\221\320\273\321\202\321\213\320\265 \321\217\321\211\320\270\320\272\320\270 \321\203 \320\264\320\276\321\200\320\276\320\263\320\270 \342\200\224 \321\201\321\200\320\265\320\267\320\272\320\260: \320\277\321\200\320\276\320\273\320\276\320\274\320\270\321\202\320\265 \320\270\321\205 \321\202\320\260\320\275\320\272\320\276\320\274.", "Yellow crates by the road: a shortcut, smash them with a tank."), x0, y + 44, 14, C(200, 205, 215));
    txt(T("\320\223\320\260\320\267 \321\202\320\276\321\207\320\275\320\276 \320\275\320\260 \302\253\320\241\320\242\320\220\320\240\320\242!\302\273 = \321\200\320\260\320\272\320\265\321\202\320\275\321\213\320\271 \321\201\321\202\320\260\321\200\321\202.", "Gas exactly at GO = rocket start."), x0, y + 66, 14, C(200, 205, 215));
}
static bool updateDrawControls(bool overlay) {
    if (!overlay) drawShowroom(menuT, S.base, M_LAND, PAINT[S.paint], 16, 9.0f);
    else DrawRectangle(0, 0, SW, SH, C(0, 0, 0, 190));
    txtS(T("\320\243\320\237\320\240\320\220\320\222\320\233\320\225\320\235\320\230\320\225", "CONTROLS"), 60, 30, 60, WHITE, 1); accentBar(60, 96, 320, 5, ACC);
    panel(40, 118, (float)SW - 100, 540, 0.65f);
    drawControlsPanel(70, 136);
    float x = 40, y = SH - 50;
    x += keycap(x, y, "ESC", 28) + 8; txt(T("\320\275\320\260\320\267\320\260\320\264", "back"), x, y + 5, 17, C(220, 224, 232));
    if (kBack() || IsKeyPressed(KEY_ENTER)) { if (!overlay) screen = SC_MAIN; play(sClick); return true; }
    return false;
}
static void drawResults(int myPos) {
    DrawRectangle(0, 0, SW, SH, C(0, 0, 0, 150));
    float w = 760, x = SW / 2 - w / 2 - 150, y = 40;
    const char* title = careerEvent >= 0 ? T(CAREER[careerEvent].ru, CAREER[careerEvent].en) : dailyActive ? T("\320\227\320\220\320\224\320\220\320\235\320\230\320\225 \320\224\320\235\320\257", "DAILY CHALLENGE") : T("\320\240\320\225\320\227\320\243\320\233\320\254\320\242\320\220\320\242\320\253", "RESULTS");
    txtS(title, x, y, 50, WHITE, 1);
    accentBar(x, y + 58, 360, 5, ACC);
    y += 80;
    bool table = S.mode == GM_RACE || S.mode == GM_ELIM || S.mode == GM_BATTLE;
    bool battle = S.mode == GM_BATTLE;
    if (table) {
        txt(T("\320\234\320\225\320\241\320\242\320\236", "POS"), x + 20, y, 15, ACC); txt(T("\320\223\320\236\320\235\320\251\320\230\320\232", "DRIVER"), x + 110, y, 15, ACC);
        txt(battle ? T("\320\236\320\247\320\232\320\230", "POINTS") : T("\320\222\320\240\320\225\320\234\320\257", "TIME"), x + 430, y, 15, ACC); txt(battle ? T("\320\250\320\220\320\240\320\230\320\232\320\230", "BALLOONS") : T("\320\233\320\243\320\247\320\250\320\230\320\231 \320\232\320\240\320\243\320\223", "BEST LAP"), x + 580, y, 15, ACC);
        y += 24;
        for (size_t i = 0; i < order.size(); i++) {
            const Racer& r = racers[order[i]];
            panel(x, y, w, 40, r.isPlayer ? 0.85f : 0.5f, 10);
            if (r.isPlayer) accentBar(x, y + 37, w, 3, ACC, 10);
            Color pc = i == 0 ? C(255, 210, 60) : i == 1 ? C(210, 215, 225) : i == 2 ? C(215, 140, 80) : WHITE;
            txt(TextFormat("%d", (int)i + 1), x + 30, y + 4, 30, pc, 1);
            DrawRectangle((int)x + 100, (int)y + 9, 6, 22, r.color);
            txt(r.name.c_str(), x + 116, y + 8, 22, r.boss ? C(255, 110, 90) : WHITE, 1);
            if (r.boss) txt(T("\320\221\320\236\320\241\320\241", "BOSS"), x + 116 + txtW(r.name.c_str(), 22, 1) + 10, y + 12, 14, C(255, 80, 60));
            iconFor(r.base, M_LAND, x + 360, y + 20, 13, C(200, 205, 215));
            if (battle) {
                txt(TextFormat("%d", r.points), x + 430, y + 9, 20, WHITE);
                for (int k = 0; k < r.balloons; k++) DrawCircleV(v2(x + 590 + k * 22.0f, y + 20), 8, balloonCol(k));
                if (r.out) txt(T("\320\262\321\213\320\261\321\213\320\273", "out"), x + 580, y + 9, 18, C(255, 110, 90));
            } else {
                std::string tt = r.out ? std::string(T("\320\262\321\213\320\261\321\213\320\273", "out")) : r.finished ? fmtTime(r.finishTime) : std::string(T("\320\262 \320\263\320\276\320\275\320\272\320\265...", "racing..."));
                txt(tt.c_str(), x + 430, y + 9, 20, r.finished ? WHITE : C(150, 155, 165));
                txt(fmtTime(r.bestLap).c_str(), x + 580, y + 9, 20, C(200, 205, 215));
            }
            y += 45;
        }
    } else {
        const Racer& pl = racers[0];
        panel(x, y, w, 150, 0.6f);
        if (S.mode == GM_TIMETRIAL) {
            txt(T("\320\222\320\220\320\250\320\225 \320\222\320\240\320\225\320\234\320\257", "YOUR TIME"), x + 30, y + 16, 16, ACC); txt(fmtTime(pl.finishTime).c_str(), x + 30, y + 36, 44, WHITE, 1);
            txt(T("\320\246\320\225\320\233\320\254", "TARGET"), x + 330, y + 16, 16, ACC); txt(fmtTime(ttTarget).c_str(), x + 330, y + 36, 44, pl.finishTime <= ttTarget ? C(120, 255, 150) : C(255, 120, 100), 1);
            txt(TextFormat("%s %s", T("\320\233\321\203\321\207\321\210\320\270\320\271 \320\272\321\200\321\203\320\263:", "Best lap:"), fmtTime(pl.bestLap).c_str()), x + 30, y + 104, 18, C(200, 205, 215));
        } else {
            txt(resultState == 1 ? T("\320\222\320\253 \320\243\320\250\320\233\320\230 \320\236\320\242 \320\237\320\236\320\223\320\236\320\235\320\230", "YOU ESCAPED") : T("\320\222\320\220\320\241 \320\220\320\240\320\225\320\241\320\242\320\236\320\222\320\220\320\233\320\230", "YOU WERE BUSTED"), x + 30, y + 20, 36, resultState == 1 ? C(120, 255, 150) : C(255, 100, 80), 1);
            txt(TextFormat("%s %s   %s %d   %s %d", T("\320\222\321\200\320\265\320\274\321\217:", "Time:"), fmtTime(pursuitT).c_str(), T("\320\232\320\276\320\277\320\276\320\262 \320\262\321\213\320\261\320\270\321\202\320\276:", "Cops wrecked:"), pl.takedowns, T("\320\240\320\276\320\267\321\213\321\201\320\272:", "Heat:"), (int)heat), x + 30, y + 84, 18, WHITE);
        }
        y += 170;
    }
    // reward card
    float rx = x + w + 30, ry = 120, rw = 280;
    panel(rx, ry, rw, 250, 0.7f);
    txt(T("\320\235\320\220\320\223\320\240\320\220\320\224\320\220", "REWARD"), rx + 26, ry + 14, 18, ACC);
    const Reward& rw2 = lastReward;
    float ly = ry + 48;
    auto line = [&](const char* k, int v) { txt(k, rx + 26, ly, 17, C(200, 205, 215)); txt(TextFormat("$%d", v), rx + rw - 90, ly, 17, WHITE); ly += 30; };
    line(T("\320\227\320\260 \321\200\320\265\320\267\321\203\320\273\321\214\321\202\320\260\321\202", "Result"), rw2.placeMoney);
    line(T("\320\234\320\276\320\275\320\265\321\202\320\272\320\270", "Coins"), rw2.coinMoney);
    line(T("\320\242\320\260\321\200\320\260\320\275\321\213", "Takedowns"), rw2.takedownMoney);
    DrawLine((int)rx + 26, (int)ly, (int)(rx + rw - 30), (int)ly, C(120, 120, 130));
    ly += 10;
    txt(T("\320\230\320\242\320\236\320\223\320\236", "TOTAL"), rx + 26, ly, 22, ACC, 1); txt(TextFormat("$%d", rw2.total), rx + rw - 110, ly, 24, C(120, 255, 150), 1);
    if (careerEvent >= 0) txt(rw2.success ? T("\320\255\320\242\320\220\320\237 \320\237\320\240\320\236\320\231\320\224\320\225\320\235!", "EVENT CLEARED!") : T("\320\255\320\242\320\220\320\237 \320\235\320\225 \320\237\320\240\320\236\320\231\320\224\320\225\320\235", "EVENT FAILED"), rx + 26, ly + 44, 18, rw2.success ? C(120, 255, 150) : C(255, 120, 100), 1);
    if (curBoss()) txt(bossBeaten ? T("\320\221\320\236\320\241\320\241 \320\237\320\236\320\222\320\225\320\240\320\226\320\225\320\235!", "BOSS BEATEN!") : T("\320\221\320\236\320\241\320\241 \320\236\320\232\320\220\320\227\320\220\320\233\320\241\320\257 \320\221\320\253\320\241\320\242\320\240\320\225\320\225", "THE BOSS WAS FASTER"), rx + 26, ly + 72, 16, bossBeaten ? C(255, 215, 70) : C(255, 120, 100), 1);
    y = std::max(y + 12, 470.0f);
    const char* it[3] = {T("\320\225\320\251\320\201 \320\240\320\220\320\227", "RETRY"), T("\320\237\320\236\320\222\320\242\320\236\320\240", "REPLAY"), careerEvent >= 0 ? T("\320\232 \320\232\320\220\320\240\320\254\320\225\320\240\320\225", "CAREER") : T("\320\222 \320\234\320\225\320\235\320\256", "MENU")};
    if (kLeft()) { resSel = (resSel + 2) % 3; play(sClick); }
    if (kRight()) { resSel = (resSel + 1) % 3; play(sClick); }
    int act = (IsKeyPressed(KEY_ENTER) || gpPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) ? resSel : -1;
    for (int i = 0; i < 3; i++) {
        float bx = SW / 2 - 480 + i * 330;
        if (mouseOver(bx, y, 290, 50) && mouseMoved()) resSel = i;
        if (menuItem(bx, y, 290, 50, it[i], resSel == i)) act = i;
    }
    (void)myPos;
    if (IsKeyPressed(KEY_R)) act = 0;
    if (act == 0) startRace();
    else if (act == 1) { screen = SC_REPLAY; play(sClick); }
    else if (act == 2 || kBack()) {
        if (careerEvent >= 0) { Base b = S.base; int pnt = S.paint; S = savedS; S.base = b; S.paint = pnt; screen = SC_CAREER; careerEvent = -1; }
        else if (dailyActive) { Base b = S.base; S = savedS; S.base = b; dailyActive = false; screen = SC_MAIN; }
        else screen = SC_MAIN;
        play(sClick);
    }
}

// =====================================================================
//                         ACHIEVEMENTS SCREEN
// =====================================================================
static void iconTrophy(float cx, float cy, float s, Color c) {
    DrawRectangleRounded(Rectangle{cx - s * 0.55f, cy - s * 0.7f, s * 1.1f, s * 0.8f}, 0.4f, 6, c);
    DrawRing(v2(cx - s * 0.55f, cy - s * 0.35f), s * 0.18f, s * 0.3f, 90, 270, 12, c);
    DrawRing(v2(cx + s * 0.55f, cy - s * 0.35f), s * 0.18f, s * 0.3f, -90, 90, 12, c);
    DrawRectangle((int)(cx - s * 0.1f), (int)(cy + s * 0.1f), (int)(s * 0.2f + 1), (int)(s * 0.35f), c);
    DrawRectangle((int)(cx - s * 0.4f), (int)(cy + s * 0.45f), (int)(s * 0.8f), (int)(s * 0.2f + 1), c);
}
static void updateDrawAch() {
    drawShowroom(menuT, S.base, M_LAND, PAINT[S.paint], 16, 9.0f);
    DrawRectangle(0, 0, SW, SH, C(0, 0, 0, 120));
    int n = 0; for (int a = 0; a < A_COUNT; a++) if (achDone(a)) n++;
    txtS(T("\320\224\320\236\320\241\320\242\320\230\320\226\320\225\320\235\320\230\320\257", "ACHIEVEMENTS"), 60, 26, 56, WHITE, 1); accentBar(60, 88, 380, 5, ACC);
    txt(TextFormat("%d / %d", n, A_COUNT), 470, 44, 30, C(255, 215, 70), 1);
    moneyBadge();
    int perCol = (A_COUNT + 1) / 2;
    float colW = (SW - 140) / 2.0f;
    for (int a = 0; a < A_COUNT; a++) {
        int col = a / perCol, row = a % perCol;
        float x = 60 + col * (colW + 20), y = 110 + row * 44;
        bool done = achDone(a);
        panel(x, y, colW - 20, 40, done ? 0.7f : 0.45f, 10);
        iconTrophy(x + 30, y + 22, 16, done ? C(255, 205, 60) : C(90, 92, 100));
        txt(T(ACH[a].ru, ACH[a].en), x + 56, y + 3, 18, done ? WHITE : C(170, 174, 184), 1);
        txt(T(ACH[a].dru, ACH[a].den), x + 56, y + 22, 13, C(180, 186, 196));
        int c, t; achProgress(a, c, t);
        float bw = 110, bx = x + colW - 20 - bw - 70;
        if (!done && t > 1) {
            DrawRectangle((int)bx, (int)y + 16, (int)bw, 8, C(50, 52, 60));
            DrawRectangle((int)bx, (int)y + 16, (int)(bw * c / (float)t), 8, ACC);
            txt(TextFormat("%d/%d", c, t), bx + bw + 8, y + 12, 14, WHITE);
        } else txt(done ? T("\320\223\320\236\320\242\320\236\320\222\320\236", "DONE") : TextFormat("$%d", ACH[a].reward), x + colW - 20 - 100, y + 12, 15, done ? C(120, 255, 150) : C(255, 215, 70));
    }
    float x = 40, y = SH - 50;
    x += keycap(x, y, "ESC", 28) + 8; txt(T("\320\275\320\260\320\267\320\260\320\264", "back"), x, y + 5, 17, C(220, 224, 232));
    if (kBack() || IsKeyPressed(KEY_ENTER)) { screen = SC_MAIN; play(sClick); }
}

// =====================================================================
//                  POST-PROCESSING (bloom, speed blur)
// =====================================================================
static Shader postShader; static bool postOK = false;
static RenderTexture2D sceneRT; static int rtW = 0, rtH = 0;
static int locRes = -1, locBoost = -1, locBloom = -1, locDamage = -1, locThresh = -1;
static const char* POST_FS =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform vec2 res;\n"
    "uniform float boost;\n"
    "uniform float bloomK;\n"
    "uniform float damage;\n"
    "uniform float thresh;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    vec2 uv = fragTexCoord;\n"
    "    vec2 c = uv - 0.5;\n"
    "    vec3 col = vec3(0.0);\n"
    "    float bl = boost * 0.035;\n"
    "    for (int i = 0; i < 6; i++) { float k = float(i) / 6.0; col += texture(texture0, uv - c * bl * k).rgb; }\n"
    "    col /= 6.0;\n"
    "    vec3 b = vec3(0.0);\n"
    "    for (int x = -3; x <= 3; x++) for (int y = -3; y <= 3; y++) {\n"
    "        vec2 o = vec2(float(x), float(y)) * 3.5 / res;\n"
    "        vec3 s = texture(texture0, uv + o).rgb;\n"
    "        float l = max(max(s.r, s.g), s.b);\n"
    "        b += s * smoothstep(thresh, 1.0, l);\n"
    "    }\n"
    "    col += b / 49.0 * bloomK;\n"
    "    col.r = mix(col.r, texture(texture0, uv + c * boost * 0.006).r, boost * 0.6);\n"
    "    col.b = mix(col.b, texture(texture0, uv - c * boost * 0.006).b, boost * 0.6);\n"
    "    float g = dot(col, vec3(0.299, 0.587, 0.114));\n"
    "    col = mix(vec3(g), col, 1.12);\n"
    "    col *= 1.0 - dot(c, c) * 0.85;\n"
    "    col.r += damage * 0.35 * dot(c, c) * 3.0;\n"
    "    finalColor = vec4(col, 1.0);\n"
    "}\n";
static void initPost() {
    postShader = LoadShaderFromMemory(nullptr, POST_FS);
    postOK = postShader.id != 0 && postShader.id != rlGetShaderIdDefault();
    if (postOK) {
        locRes = GetShaderLocation(postShader, "res"); locBoost = GetShaderLocation(postShader, "boost");
        locBloom = GetShaderLocation(postShader, "bloomK"); locDamage = GetShaderLocation(postShader, "damage"); locThresh = GetShaderLocation(postShader, "thresh");
    }
}
static bool usePost() { return postOK && S.post; }
static float renderScale() { return S.quality == 0 ? 0.6f : S.quality == 1 ? 0.85f : 1.0f; }
// off-screen views: one per player in split screen, or a scaled one for low quality
struct View { RenderTexture2D rt; int w = 0, h = 0; };
static View views[2];
static void ensureView(View& v, int w, int h) {
    w = std::max(16, w); h = std::max(16, h);
    if (v.w == w && v.h == h) return;
    if (v.w > 0) UnloadRenderTexture(v.rt);
    v.rt = LoadRenderTexture(w, h); v.w = w; v.h = h;
    SetTextureFilter(v.rt.texture, TEXTURE_FILTER_BILINEAR);
}
static void applyPostUniforms(int w, int h, float boost, float dmg) {
    float res[2] = {(float)w, (float)h};
    float bloom = indoorTrack() ? 1.1f : spaceTrack() ? 1.3f : nightK > 0.5f ? (curTrack == 3 ? 1.1f : 2.2f) : (S.tod == TD_SUNSET ? 1.2f : 0.6f);
    float thr = indoorTrack() ? 0.85f : spaceTrack() ? 0.7f : nightK > 0.5f ? (curTrack == 3 ? 0.82f : 0.6f) : (curTrack == 3 ? 0.92f : 0.85f);
    SetShaderValue(postShader, locRes, res, SHADER_UNIFORM_VEC2);
    SetShaderValue(postShader, locBoost, &boost, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader, locBloom, &bloom, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader, locDamage, &dmg, SHADER_UNIFORM_FLOAT);
    SetShaderValue(postShader, locThresh, &thr, SHADER_UNIFORM_FLOAT);
}
// draw a finished view onto the screen (inside BeginDrawing)
static void blitView(View& v, float x, float y, float w, float h, float boost, float dmg) {
    bool post = usePost();
    if (post) { applyPostUniforms(v.w, v.h, boost, dmg); BeginShaderMode(postShader); }
    DrawTexturePro(v.rt.texture, Rectangle{0, 0, (float)v.w, (float)-v.h}, Rectangle{x, y, w, h}, v2(0, 0), 0, WHITE);
    if (post) EndShaderMode();
}
static void ensureRT() {
    if (rtW == SW && rtH == SH) return;
    if (rtW > 0) UnloadRenderTexture(sceneRT);
    sceneRT = LoadRenderTexture(SW, SH); rtW = SW; rtH = SH;
}
static void beginScene() {
    if (usePost()) { ensureRT(); BeginTextureMode(sceneRT); ClearBackground(fogCol); }
    else { BeginDrawing(); ClearBackground(fogCol); }
}
static void endScene(float boost, float dmg) {
    if (!usePost()) return;
    EndTextureMode();
    BeginDrawing();
    ClearBackground(BLACK);
    applyPostUniforms(SW, SH, boost, dmg);
    BeginShaderMode(postShader);
    DrawTextureRec(sceneRT.texture, Rectangle{0, 0, (float)rtW, (float)-rtH}, v2(0, 0), WHITE);
    EndShaderMode();
}

// =====================================================================
//                            RACE SCREEN
// =====================================================================
struct Drop { float x, y, s; };
static std::vector<Drop> rainD, snowD;
static bool autoPilot = false;
static int photoCount = 0; static float photoYaw = 0, photoPitch = 0.3f, photoDist = 12, photoMsgT = 0;

static float padAxisP(int pad, int a) { if (pad < 0 || !IsGamepadAvailable(pad)) return 0; float v = GetGamepadAxisMovement(pad, a); return fabsf(v) < 0.15f ? 0.0f : v; }
static float padTrigP(int pad, int a) { if (pad < 0 || !IsGamepadAvailable(pad)) return 0; float v = GetGamepadAxisMovement(pad, a); return clampf((v + 1) * 0.5f, 0, 1); }
static bool padDownP(int pad, int b) { return pad >= 0 && IsGamepadAvailable(pad) && IsGamepadButtonDown(pad, b); }
static bool padPressedP(int pad, int b) { return pad >= 0 && IsGamepadAvailable(pad) && IsGamepadButtonPressed(pad, b); }
static float startHold[2] = {0, 0};

static void startRace() {
    resetRace(); screen = SC_RACE; paused = false; photoMode = false; controlsOverlay = false; rewardGiven = false;
    cs = CamState(); cs.yaw = racers[0].yaw;
    cs2 = CamState(); if (racers.size() > 1) cs2.yaw = racers[1].yaw;
    startHold[0] = startHold[1] = 0;
    play(sClick);
}
static void leaveRaceToMenu() {
    if (careerEvent >= 0) { Base b = S.base; int pnt = S.paint; S = savedS; S.base = b; S.paint = pnt; careerEvent = -1; screen = SC_CAREER; }
    else if (dailyActive) { Base b = S.base; S = savedS; S.base = b; dailyActive = false; screen = SC_MAIN; }
    else screen = SC_MAIN;
    paused = false; aP.siren = 0; aP.hum = 0; aP.musicStyle = 0;
}
// read one human player's controls: player 1 = WASD side (+ arrows when alone), player 2 = arrows side; gamepads by padOfPlayer()
static Input readHuman(Racer& r, bool allowActions) {
    Input in;
    bool p2 = r.pid == 1, solo = numHumans < 2;
    if (!p2) {
        in.throttle = (float)((IsKeyDown(KEY_W) || (solo && IsKeyDown(KEY_UP))) - (IsKeyDown(KEY_S) || (solo && IsKeyDown(KEY_DOWN))));
        in.steer = (float)((IsKeyDown(KEY_D) || (solo && IsKeyDown(KEY_RIGHT))) - (IsKeyDown(KEY_A) || (solo && IsKeyDown(KEY_LEFT))));
        in.strafe = (float)(IsKeyDown(KEY_E) - IsKeyDown(KEY_Q));
        in.boost = IsKeyDown(KEY_LEFT_SHIFT) || (solo && IsKeyDown(KEY_RIGHT_SHIFT));
        in.special = IsKeyDown(KEY_SPACE);
        in.special2 = IsKeyDown(KEY_F);
        in.item = IsKeyDown(KEY_R) || IsKeyDown(KEY_LEFT_CONTROL);
    } else {
        in.throttle = (float)(IsKeyDown(KEY_UP) - IsKeyDown(KEY_DOWN));
        in.steer = (float)(IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT));
        in.strafe = (float)(IsKeyDown(KEY_APOSTROPHE) - IsKeyDown(KEY_SEMICOLON));
        in.boost = IsKeyDown(KEY_RIGHT_SHIFT);
        in.special = IsKeyDown(KEY_RIGHT_CONTROL);
        in.special2 = IsKeyDown(KEY_PERIOD);
        in.item = IsKeyDown(KEY_SLASH) || IsKeyDown(KEY_KP_0);
    }
    int pad = padOfPlayer(r.pid);
    if (pad >= 0) {
        in.steer = clampf(in.steer + padAxisP(pad, GAMEPAD_AXIS_LEFT_X), -1, 1);
        in.throttle = clampf(in.throttle + padTrigP(pad, GAMEPAD_AXIS_RIGHT_TRIGGER) - padTrigP(pad, GAMEPAD_AXIS_LEFT_TRIGGER) - (r.mode == M_AIR ? padAxisP(pad, GAMEPAD_AXIS_LEFT_Y) : 0.0f), -1, 1);
        in.strafe = clampf(in.strafe + padAxisP(pad, GAMEPAD_AXIS_RIGHT_X), -1, 1);
        in.boost = in.boost || padDownP(pad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
        in.special = in.special || padDownP(pad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        in.item = in.item || padDownP(pad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        in.special2 = in.special2 || padDownP(pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
    }
    if (allowActions) {
        const Base order4[4] = {B_CAR, B_TANK, B_ROBOT, B_KART};
        int keys[4] = {p2 ? KEY_SEVEN : KEY_ONE, p2 ? KEY_EIGHT : KEY_TWO, p2 ? KEY_NINE : KEY_THREE, p2 ? KEY_ZERO : KEY_FOUR};
        for (int k = 0; k < 4; k++) if (IsKeyPressed(keys[k])) switchBase(r, order4[k]);
        if (pad >= 0 && (padPressedP(pad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || padPressedP(pad, GAMEPAD_BUTTON_LEFT_FACE_LEFT))) {
            int d = padPressedP(pad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ? 1 : 3, b = (int)r.base;
            for (int t = 0; t < 4; t++) { b = (b + d) % 4; if (baseAllowed((Base)b)) break; }
            switchBase(r, (Base)b);
        }
        if (IsKeyPressed(p2 ? KEY_BACKSLASH : KEY_T) || padPressedP(pad, GAMEPAD_BUTTON_MIDDLE_LEFT)) respawn(r);
    }
    return in;
}
static void simulateRace(float rdt) {
    float dt = rdt * (slowmoT > 0 ? 0.3f : 1.0f);
    slowmoT = std::max(0.0f, slowmoT - rdt); takedownBannerT = std::max(0.0f, takedownBannerT - rdt);
    bool started = countdown <= 1.0f;
    gTime += dt;
    int prevC = (int)ceilf(countdown - 1.0f);
    countdown -= dt;
    int nowC = (int)ceilf(countdown - 1.0f);
    if (nowC != prevC && countdown > 0) { if (nowC >= 1 && nowC <= 3) play(sBeep); else if (nowC == 0) play(sGo); }
    bool justStarted = !started && countdown <= 1.0f;
    if (started) raceTime += dt;
    for (auto& n : notes) n.t -= rdt;
    notes.erase(std::remove_if(notes.begin(), notes.end(), [](const Note& n) { return n.t <= 0; }), notes.end());
    hintT = std::max(0.0f, hintT - rdt);
    finishBannerT = std::max(0.0f, finishBannerT - rdt);
    updateWeather(dt);
    updateAtmosphere(dt, false);
    int myPos; computeOrder(myPos);
    // humans: input, rocket start
    bool humanCtl[2] = {false, false};
    for (int h = 0; h < numHumans && h < (int)racers.size(); h++) {
        Racer& pl = racers[h];
        bool live = !pl.dead && !pl.finished && !pl.out && resultState == 0 && screen == SC_RACE && !autoPilot;
        if (!started && !autoPilot) {
            Input pre = readHuman(pl, false);
            if (pre.throttle > 0.3f) startHold[h] += rdt; else startHold[h] = 0;
            pl.in = Input();
        }
        if (justStarted && !autoPilot) {
            if (startHold[h] > 0.02f && startHold[h] < 0.6f) { pl.boostT = std::max(pl.boostT, 1.4f); notifyP(pl, T("\320\240\320\220\320\232\320\225\320\242\320\235\320\253\320\231 \320\241\320\242\320\220\320\240\320\242!", "ROCKET START!"), C(255, 200, 60), 2.0f); play(sWhoosh, 0.8f, 1.5f); rumblePad(pl.pid, 0.6f, 0.3f); }
            else if (startHold[h] > 1.6f) { pl.startBoostT = 0.9f; notifyP(pl, T("\320\237\320\225\320\240\320\225\320\223\320\220\320\227\320\236\320\222\320\232\320\220!", "OVER-REVVED!"), C(255, 120, 80), 1.6f); }
        }
        if (started && live) { pl.in = readHuman(pl, true); humanCtl[h] = true; }
    }
    if (justStarted) for (auto& r : racers) if (!r.isPlayer && !r.isCop && GetRandomValue(0, 100) < 30) r.boostT = std::max(r.boostT, 0.8f);
    for (size_t i = 0; i < racers.size(); i++) {
        if (i < 2 && humanCtl[i]) continue;
        if (started) aiThink(racers[i], dt); else racers[i].in = Input();
        if (racers[i].isPlayer && !autoPilot) racers[i].in.item = false;
    }
    for (size_t i = 0; i < racers.size(); i++) { if (started) updateRacer(racers[i], dt); else updateProgress(racers[i]); }
    if (started) { collideRacers(); updateShortcuts(dt); updateShells(dt); updateItems(dt); updateTraffic(dt); updateModes(dt); }
    for (auto& p : parts) {
        p.p = add(p.p, mul(p.v, dt)); p.v = mul(p.v, 1.0f - 1.4f * dt);
        if (p.grav) p.v.y -= 18.0f * gravK() * dt;
        p.life -= dt;
    }
    parts.erase(std::remove_if(parts.begin(), parts.end(), [](const Particle& p) { return p.life <= 0 || p.p.y < -60.0f; }), parts.end());
    shakeAmt = std::max(0.0f, shakeAmt - rdt * 2.0f);
    for (int h = 0; h < 2; h++) shakeP[h] = std::max(0.0f, shakeP[h] - rdt * 2.0f);
    if (started) { recT -= dt; if (recT <= 0) { recT = 0.05f; recordFrame(); } }
    Racer& p0 = racers[0];
    aP.type = p0.mode == M_AIR ? 4 : p0.mode == M_WATER ? (p0.base == B_TANK ? 1 : 3) : (p0.base == B_KART ? 5 : (int)p0.base);
    aP.rpm = started ? p0.rpm - (p0.gearShiftT > 0 ? 0.15f : 0.0f) : 0.25f + (startHold[0] > 0 ? 0.5f : 0.0f);
    aP.thr = std::max(0.0f, p0.in.throttle) + (started ? 0.0f : (startHold[0] > 0 ? 1.0f : 0.0f));
    aP.speed = speedOf(p0); aP.boost = boostK(p0) > 0 ? 1.0f : 0.0f; aP.rain = wetK; aP.walk = p0.walk;
    aP.master = p0.dead ? 0.0f : 0.75f;
}
static void drawWeather2D(float dt) {
    if (indoorTrack()) return;
    if (curWeather == W_RAIN || curWeather == W_STORM) {
        int cnt = (int)(rainD.size() * wI * (curWeather == W_STORM ? 1.0f : 0.6f));
        float wind = curWeather == W_STORM ? 380.0f : 110.0f;
        for (int i = 0; i < cnt; i++) {
            Drop& d = rainD[i];
            d.y += 1700 * d.s * dt; d.x -= wind * d.s * dt;
            if (d.y > SH) { d.y = -20; d.x = frand(0, SW + 200.0f); }
            if (d.x < -50) d.x = SW + 20.0f;
            DrawLineEx(v2(d.x, d.y), v2(d.x - wind * 0.02f, d.y + 22 * d.s), 1.2f, alpha(mixC(C(200, 210, 235), fogCol, 0.3f), 0.45f));
        }
    }
    if (curWeather == W_SNOW) {
        int cnt = (int)(snowD.size() * wI);
        for (int i = 0; i < cnt; i++) {
            Drop& d = snowD[i];
            d.y += (60 + 110 * d.s) * dt; d.x += sinf(gTime * 1.5f + d.s * 20) * 35 * dt - 20 * dt;
            if (d.y > SH) { d.y = -10; d.x = frand(0, (float)SW); }
            if (d.x < -10) d.x = SW + 5.0f;
            DrawCircleV(v2(d.x, d.y), 1.5f + 2.5f * d.s, alpha(WHITE, 0.85f));
        }
    }
    if (flashT > 0) DrawRectangle(0, 0, SW, SH, alpha(WHITE, flashT * 0.6f));
}
static GhostFrame ghostNow; static bool ghostVisible = false;
static float drawRCap = 1.0f;
static void renderRaceWorld(Camera3D& cam, bool hidePlayer) {
    drawR = std::min(fogE * 1.05f, 1000.0f) * drawRCap;
    drawSky(cam);
    BeginMode3D(cam);
    rlDisableBackfaceCulling();
    rlDrawRenderBatchActive(); rlDisableDepthTest();
    drawGround(cam);
    rlDrawRenderBatchActive(); rlEnableDepthTest();
    glows.clear(); bubbles.clear();
    drawTrackside();
    drawBuildings();
    drawLamps();
    drawTrees();
    drawDecos();
    drawHarborAndBoards();
    drawGatesRings();
    drawKartLayer();
    drawShortcuts();
    drawTraffic();
    for (const Racer& r : racers) { if (hidePlayer && r.id == vpPlayer) continue; drawVehicle(r, true); }
    rlDrawRenderBatchActive(); rlDisableDepthMask();
    if (ghostVisible) {
        Racer g; g.base = (Base)ghostNow.base; g.mode = (Mode)ghostNow.mode; g.pos = ghostNow.p; g.yaw = ghostNow.yaw; g.pitch = ghostNow.pitch; g.roll = ghostNow.roll;
        g.morphAir = ghostNow.ma; g.morphWater = ghostNow.mw; g.color = C(120, 200, 255); g.helmet = WHITE; g.hp = 100;
        gAlpha = 110; drawVehicle(g, false); gAlpha = 255;
    }
    BeginBlendMode(BLEND_ALPHA);
    drawParticles(cam, false);
    drawBubbles(cam);
    EndBlendMode();
    BeginBlendMode(BLEND_ADDITIVE);
    drawLightCones();
    drawParticles(cam, true);
    for (const GlowPt& g : glows) glow(cam, g.p, g.s, g.c);
    EndBlendMode();
    rlDrawRenderBatchActive(); rlEnableDepthMask();
    EndMode3D();
}
static Camera3D photoCamera(const Racer& pl) {
    Camera3D cam{}; cam.projection = CAMERA_PERSPECTIVE; cam.fovy = 50; cam.up = vec(0, 1, 0);
    cam.target = add(pl.pos, vec(0, 1.2f, 0));
    cam.position = clampToCorridor(add(cam.target, vec(sinf(photoYaw) * cosf(photoPitch) * photoDist, sinf(photoPitch) * photoDist, cosf(photoYaw) * cosf(photoPitch) * photoDist)), pl.lastIdx);
    camPos = cam.position; camFwd = norm(sub(cam.target, cam.position));
    return cam;
}
static void drawFpsCounter() {
    if (!S.showFps) return;
    const char* f = TextFormat("FPS %d", GetFPS());
    DrawRectangle(SW / 2 - 50, SH - 26, 100, 22, C(0, 0, 0, 150));
    txtC(f, SW / 2, SH - 25, 18, GetFPS() >= 50 ? C(120, 255, 150) : C(255, 180, 80), 0, false);
}
// render one player's view of the world into a view target
static void renderPlayerView(View& v, Camera3D& cam, bool hidePlayer, float dt) {
    int sw = SW, sh = SH;
    BeginTextureMode(v.rt);
    ClearBackground(fogCol);
    SW = v.w; SH = v.h; vpW = v.w; vpH = v.h; hudScale = 1.0f;
    renderRaceWorld(cam, hidePlayer);
    drawWeather2D(dt);
    EndTextureMode();
    SW = sw; SH = sh; vpW = sw; vpH = sh;
}
// HUD for one player inside a screen region, laid out in scaled virtual units
static void drawPlayerHUD(Camera3D& cam, const Racer& pl, int pos, float rx, float ry, float rw, float rh, float zoom) {
    int sw = SW, sh = SH;
    Camera2D c2{}; c2.offset = v2(rx, ry); c2.target = v2(0, 0); c2.rotation = 0; c2.zoom = zoom;
    BeginScissorMode((int)rx, (int)ry, (int)rw, (int)rh);
    BeginMode2D(c2);
    SW = (int)(rw / zoom); SH = (int)(rh / zoom); vpW = (int)rw; vpH = (int)rh; hudScale = zoom; vpPlayer = pl.id;
    drawRaceHUD(cam, pl, pos, showHelp);
    EndMode2D();
    EndScissorMode();
    SW = sw; SH = sh; vpW = sw; vpH = sh; hudScale = 1.0f; vpPlayer = 0;
}
static void updateDrawRace(float dt) {
    if (screen == SC_RACE && !controlsOverlay && !photoMode) {
        bool p = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) || gpPressed(GAMEPAD_BUTTON_MIDDLE_RIGHT);
        if (p) { paused = !paused; pauseSel = 0; play(sClick); }
    }
    if (!paused) {
        int pad0 = padOfPlayer(0), pad1 = padOfPlayer(1);
        if (IsKeyPressed(KEY_V) || padPressedP(pad0, GAMEPAD_BUTTON_RIGHT_FACE_UP) || (numHumans < 2 && IsKeyPressed(KEY_RIGHT_BRACKET))) cs.mode = (cs.mode + 1) % 3;
        if (numHumans > 1 && (IsKeyPressed(KEY_RIGHT_BRACKET) || padPressedP(pad1, GAMEPAD_BUTTON_RIGHT_FACE_UP))) cs2.mode = (cs2.mode + 1) % 3;
        if (IsKeyPressed(KEY_H)) showHelp = !showHelp;
        if (IsKeyPressed(KEY_M) && S.musicSrc) nextSong();
        simulateRace(dt);
    } else { aP.master = 0; aP.siren = 0; }
    Racer& pl = racers[0];
    bool humansDone = true;
    for (const Racer& r : racers) if (r.isPlayer && !r.finished && !r.out) humansDone = false;
    bool over = humansDone || resultState != 0;
    if (screen == SC_RACE && over && finishBannerT <= 0) { screen = SC_RESULTS; resSel = 0; int mp; computeOrder(mp); computeRewards(mp); }
    int myPos; computeOrder(myPos);
    int pad0 = padOfPlayer(0), pad1 = padOfPlayer(1);
    bool rear0 = (IsKeyDown(KEY_C) || padDownP(pad0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) && screen == SC_RACE && !paused;
    bool rear1 = numHumans > 1 && (IsKeyDown(KEY_LEFT_BRACKET) || padDownP(pad1, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) && screen == SC_RACE && !paused;
    ghostVisible = S.mode == GM_TIMETRIAL && pl.lap >= 1 && ghostSample(raceTime - pl.lapStart, ghostNow);
    partCap = S.quality == 0 ? 500 : S.quality == 1 ? 1000 : 1800;
    bool split = numHumans > 1 && screen == SC_RACE && !photoMode && racers.size() > 1;
    Camera3D cam;
    if (photoMode) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { Vector2 d = GetMouseDelta(); photoYaw -= d.x * 0.006f; photoPitch = clampf(photoPitch + d.y * 0.005f, -0.1f, 1.4f); }
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) photoYaw += dt * 1.5f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) photoYaw -= dt * 1.5f;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) photoPitch = clampf(photoPitch + dt, -0.1f, 1.4f);
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) photoPitch = clampf(photoPitch - dt, -0.1f, 1.4f);
        photoDist = clampf(photoDist - GetMouseWheelMove() * 1.5f, 4, 60);
        cam = photoCamera(pl);
    } else if (screen == SC_RESULTS) {
        cam = Camera3D{}; cam.projection = CAMERA_PERSPECTIVE; cam.fovy = 50; cam.up = vec(0, 1, 0);
        float a = gTime * 0.25f;
        cam.target = add(pl.pos, vec(0, 1.5f, 0));
        cam.position = clampToCorridor(add(pl.pos, vec(sinf(a) * 12, indoorTrack() ? 4.0f : 4.5f, cosf(a) * 12)), pl.lastIdx);
        camPos = cam.position; camFwd = norm(sub(cam.target, cam.position));
    } else cam = updateCamera(cs, pl, paused ? 0.0f : dt, rear0);

    float qd = S.quality == 0 ? 0.55f : S.quality == 1 ? 0.78f : 1.0f;
    if (split) {
        // ---------------- two players: top and bottom halves ----------------
        int hh = SH / 2;
        const Racer& p2 = racers[1];
        // player 1 camera was computed above; render it, then player 2
        float sc = renderScale();
        ensureView(views[0], (int)(SW * sc), (int)(hh * sc)); ensureView(views[1], (int)(SW * sc), (int)(hh * sc));
        Camera3D camA = cam;
        vpPlayer = 0; drawRCap = 0.82f * qd;
        renderPlayerView(views[0], camA, cs.mode == 1 && !rear0, paused ? 0.0f : dt);
        V3 saveP = camPos, saveF = camFwd; (void)saveP; (void)saveF;
        Camera3D camB = updateCamera(cs2, p2, paused ? 0.0f : dt, rear1);
        vpPlayer = 1;
        renderPlayerView(views[1], camB, cs2.mode == 1 && !rear1, paused ? 0.0f : dt);
        vpPlayer = 0; drawRCap = qd;
        BeginDrawing();
        ClearBackground(BLACK);
        blitView(views[0], 0, 0, (float)SW, (float)hh, clampf(boostK(pl) * 0.8f, 0, 1), pl.hurtT);
        blitView(views[1], 0, (float)hh, (float)SW, (float)(SH - hh), clampf(boostK(p2) * 0.8f, 0, 1), p2.hurtT);
        int pos2 = placeOf(1);
        if (!paused) {
            float zoom = clampf(hh / 520.0f, 0.5f, 0.85f);
            camPos = camA.position; camFwd = norm(sub(camA.target, camA.position));
            drawPlayerHUD(camA, pl, myPos, 0, 0, (float)SW, (float)hh, zoom);
            camPos = camB.position; camFwd = norm(sub(camB.target, camB.position));
            drawPlayerHUD(camB, p2, pos2, 0, (float)hh, (float)SW, (float)(SH - hh), zoom);
        }
        DrawRectangle(0, hh - 2, SW, 4, C(10, 10, 14));
        DrawRectangle(0, hh - 1, SW, 2, alpha(ACC, 0.8f));
    } else {
        // ---------------- one view ----------------
        drawRCap = qd;
        float sc = renderScale();
        bool direct = sc >= 0.999f && !usePost();
        if (direct) {
            BeginDrawing(); ClearBackground(fogCol);
            vpW = SW; vpH = SH;
            renderRaceWorld(cam, cs.mode == 1 && !rear0 && screen == SC_RACE && !photoMode && slowmoT <= 0);
            drawWeather2D(paused ? 0.0f : dt);
        } else {
            ensureView(views[0], (int)(SW * sc), (int)(SH * sc));
            renderPlayerView(views[0], cam, cs.mode == 1 && !rear0 && screen == SC_RACE && !photoMode && slowmoT <= 0, paused ? 0.0f : dt);
            camPos = cam.position; camFwd = norm(sub(cam.target, cam.position));
            BeginDrawing(); ClearBackground(BLACK);
            blitView(views[0], 0, 0, (float)SW, (float)SH, photoMode ? 0.0f : clampf(boostK(pl) * 0.8f + (slowmoT > 0 ? 0.6f : 0.0f), 0, 1), pl.hurtT);
        }
    }
    if (photoMode) {
        bool shot = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_F) || gpPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        if (shot) {
            rlDrawRenderBatchActive();
            TakeScreenshot(TextFormat("MegaRace_photo_%03d.png", ++photoCount));
            photoMsgT = 2.5f; play(sCash, 0.6f);
        }
        photoMsgT = std::max(0.0f, photoMsgT - dt);
        if (!IsKeyDown(KEY_H)) {
            float x = 30, y = SH - 52;
            panel(10, y - 10, 900, 48, 0.5f);
            x += keycap(x, y, T("\320\234\320\253\320\250\320\254", "MOUSE"), 28) + 8; txt(T("\320\262\321\200\320\260\321\211\320\260\321\202\321\214", "orbit"), x, y + 5, 16, WHITE); x += 90;
            x += keycap(x, y, T("\320\232\320\236\320\233\320\225\320\241\320\236", "WHEEL"), 28) + 8; txt(T("\320\267\321\203\320\274", "zoom"), x, y + 5, 16, WHITE); x += 60;
            x += keycap(x, y, "ENTER", 28) + 8; txt(T("\321\201\320\275\320\270\320\274\320\276\320\272", "photo"), x, y + 5, 16, WHITE); x += 90;
            x += keycap(x, y, "H", 28) + 8; txt(T("\321\201\320\272\321\200\321\213\321\202\321\214", "hide"), x, y + 5, 16, WHITE); x += 80;
            x += keycap(x, y, "ESC", 28) + 8; txt(T("\320\275\320\260\320\267\320\260\320\264", "back"), x, y + 5, 16, WHITE);
            txtS(T("\320\244\320\236\320\242\320\236\320\240\320\225\320\226\320\230\320\234", "PHOTO MODE"), 30, 24, 40, WHITE, 1);
        }
        if (photoMsgT > 0) txtC(TextFormat(T("\320\241\320\275\320\270\320\274\320\276\320\272 \321\201\320\276\321\205\321\200\320\260\320\275\321\221\320\275: MegaRace_photo_%03d.png", "Saved: MegaRace_photo_%03d.png"), photoCount), SW / 2, 30, 22, alpha(C(120, 255, 150), clampf(photoMsgT, 0, 1)));
        if (kBack()) { photoMode = false; play(sClick); }
        EndDrawing();
        return;
    }
    if (screen == SC_RACE && !paused && !split) {
        vpPlayer = 0;
        drawRaceHUD(cam, pl, myPos, showHelp);
        if (rear0) txtC(T("\320\222\320\230\320\224 \320\235\320\220\320\227\320\220\320\224", "REAR VIEW"), SW / 2, 120, 20, alpha(WHITE, 0.8f));
    } else if (screen == SC_RESULTS) drawResults(myPos);
    if (paused) {
        if (controlsOverlay) { if (updateDrawControls(true)) controlsOverlay = false; }
        else {
            DrawRectangle(0, 0, SW, SH, C(0, 0, 0, 175));
            txtS(T("\320\237\320\220\320\243\320\227\320\220", "PAUSED"), 60, 60, 72, WHITE, 1); accentBar(60, 140, 240, 5, ACC);
            const char* it[5] = {T("\320\237\320\240\320\236\320\224\320\236\320\233\320\226\320\230\320\242\320\254", "RESUME"), T("\320\227\320\220\320\235\320\236\320\222\320\236", "RESTART"), T("\320\244\320\236\320\242\320\236\320\240\320\225\320\226\320\230\320\234", "PHOTO MODE"), T("\320\243\320\237\320\240\320\220\320\222\320\233\320\225\320\235\320\230\320\225", "CONTROLS"), T("\320\222\320\253\320\231\320\242\320\230 \320\222 \320\234\320\225\320\235\320\256", "QUIT TO MENU")};
            if (kUp()) { pauseSel = (pauseSel + 4) % 5; play(sClick); }
            if (kDown()) { pauseSel = (pauseSel + 1) % 5; play(sClick); }
            int act = (IsKeyPressed(KEY_ENTER) || gpPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) ? pauseSel : -1;
            for (int i = 0; i < 5; i++) {
                float y = 180 + i * 62;
                if (mouseOver(60, y, 360, 50) && mouseMoved()) pauseSel = i;
                if (menuItem(60, y, 360, 50, it[i], pauseSel == i)) act = i;
            }
            panel(SW - 470, 160, 430, 300, 0.55f);
            txt(careerEvent >= 0 ? T(CAREER[careerEvent].ru, CAREER[careerEvent].en) : gameModeName(S.mode), SW - 440, 176, 18, ACC, 1);
            txt(TextFormat("%s: %s", T("\320\242\321\200\320\260\321\201\321\201\320\260", "Track"), trackName(curTrack)), SW - 440, 210, 18, WHITE);
            txt(TextFormat("%s: %s", T("\320\237\320\276\320\263\320\276\320\264\320\260", "Weather"), weatherName(curWeather)), SW - 440, 240, 18, WHITE);
            txt(TextFormat("%s: %d / %d", T("\320\237\320\276\320\267\320\270\321\206\320\270\321\217", "Position"), myPos, numRacers), SW - 440, 270, 18, WHITE);
            txt(TextFormat("%s: %d   %s: %d", T("\320\234\320\276\320\275\320\265\321\202\320\272\320\270", "Coins"), pl.coinsTotal, T("\320\242\320\260\321\200\320\260\320\275\321\213", "Takedowns"), pl.takedowns), SW - 440, 300, 18, WHITE);
            txt(TextFormat("%s: %s", T("\320\233\321\203\321\207\321\210\320\270\320\271 \320\272\321\200\321\203\320\263", "Best lap"), fmtTime(pl.bestLap).c_str()), SW - 440, 330, 18, WHITE);
            if (dailyActive) txt(dailyText(makeDaily(todayId())).c_str(), SW - 440, 370, 15, C(255, 215, 70));
            if (act == 0 || (kBack() && !IsKeyPressed(KEY_ESCAPE))) { paused = false; play(sClick); }
            else if (act == 1) startRace();
            else if (act == 2) { photoMode = true; photoYaw = racers[0].yaw + 2.5f; play(sClick); }
            else if (act == 3) { controlsOverlay = true; play(sClick); }
            else if (act == 4) { leaveRaceToMenu(); play(sClick); }
        }
    }
    drawFpsCounter();
    EndDrawing();
}

// =====================================================================
//                               REPLAY
// =====================================================================
static float rpT = 0; static int rpSpeedIdx = 2, rpCam = 0, rpFocus = 0; static bool rpPause = false, rpHideUI = false;
static std::vector<Racer> rpRacers; static std::vector<Traffic> rpTraffic;
static void updateDrawReplay(float dt) {
    if (replay.size() < 2) { screen = SC_RESULTS; return; }
    const float speeds[5] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
    if (IsKeyPressed(KEY_SPACE) || gpPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) rpPause = !rpPause;
    if (kLeft()) rpSpeedIdx = std::max(0, rpSpeedIdx - 1);
    if (kRight()) rpSpeedIdx = std::min(4, rpSpeedIdx + 1);
    if (IsKeyPressed(KEY_V) || gpPressed(GAMEPAD_BUTTON_RIGHT_FACE_UP)) rpCam = (rpCam + 1) % 3;
    if (IsKeyPressed(KEY_H)) rpHideUI = !rpHideUI;
    if (IsKeyPressed(KEY_R)) rpT = 0;
    int nAct = (int)replay[0].a.size();
    if (kUp() || IsKeyPressed(KEY_E)) rpFocus = (rpFocus + 1) % std::max(1, nAct);
    if (kDown() || IsKeyPressed(KEY_Q)) rpFocus = (rpFocus + nAct - 1) % std::max(1, nAct);
    float t0 = replay.front().t, t1 = replay.back().t;
    if (!rpPause) rpT += dt * speeds[rpSpeedIdx];
    if (rpT > t1 - t0) rpT = 0;
    float t = t0 + rpT;
    size_t lo = 0, hi = replay.size() - 1;
    while (hi - lo > 1) { size_t m = (lo + hi) / 2; if (replay[m].t <= t) lo = m; else hi = m; }
    const RecFrame &A = replay[lo], &B = replay[hi];
    float k = (B.t - A.t) > 1e-4f ? clampf((t - A.t) / (B.t - A.t), 0, 1) : 0;
    rpRacers.clear();
    for (size_t i = 0; i < A.a.size(); i++) {
        const RecActor& a = A.a[i];
        const RecActor* b = nullptr;
        for (const RecActor& q : B.a) if (q.id == a.id) { b = &q; break; }
        Racer r; r.id = a.id; r.base = (Base)a.base; r.mode = (Mode)a.mode; r.color = a.c; r.isCop = (a.flags & 4) != 0; r.isPlayer = (a.flags & 64) != 0;
        r.helmet = PAINT[(r.id * 3 + 2) % 8];
        r.pos = b ? add(a.p, mul(sub(b->p, a.p), k)) : a.p;
        r.yaw = b ? a.yaw + wrapA(b->yaw - a.yaw) * k : a.yaw; r.pitch = a.pitch; r.roll = a.roll; r.spinAng = a.spin;
        r.morphAir = a.ma; r.morphWater = a.mw; r.steerV = a.steer; r.dead = (a.flags & 1) != 0; r.out = (a.flags & 32) != 0;
        r.boosting = (a.flags & 2) != 0; r.shieldT = (a.flags & 8) ? 1.0f : 0.0f; r.shrinkT = (a.flags & 16) ? 1.0f : 0.0f;
        r.vel = b ? mul(sub(b->p, a.p), 1.0f / std::max(0.01f, B.t - A.t)) : vec(0, 0, 0);
        r.wheelRot = t * 20; r.walk = t * 8; r.grounded = true; r.hp = 100;
        int best = 0; float bd = 1e18f; for (int j = 0; j < N; j += 4) { float d = dist2D(track[j], r.pos); if (d < bd) { bd = d; best = j; } }
        r.lastIdx = best;
        rpRacers.push_back(r);
    }
    rpTraffic.clear();
    for (const RecTraffic& q : A.tr) {
        Traffic tr{}; tr.pos = q.p; tr.yaw = q.yaw; tr.type = q.type; tr.c = q.c; tr.wreck = q.wreck != 0; tr.dead = false;
        const float L[4] = {4.4f, 5.0f, 8.5f, 11.0f}, W[4] = {1.9f, 2.1f, 2.5f, 2.6f};
        tr.len = L[tr.type % 4]; tr.wid = W[tr.type % 4];
        rpTraffic.push_back(tr);
    }
    if (rpFocus >= (int)rpRacers.size()) rpFocus = 0;
    Racer& f = rpRacers[rpFocus];
    gTime = t;
    Camera3D cam{}; cam.projection = CAMERA_PERSPECTIVE; cam.up = vec(0, 1, 0); cam.fovy = 55;
    if (rpCam == 0) {   // TV cameras along the track
        int seg = (f.lastIdx / 90) * 90;
        int ci = (seg + 60) % N;
        float side = ((seg / 90) % 2) ? 1.0f : -1.0f;
        V3 cp = add(track[ci], mul(norv[ci], side * (lim[ci] + 6)));
        cp.y = (zmode[ci] == M_AIR ? airAlt[ci] * 0.6f : th[ci]) + (indoorTrack() ? 4.0f : 5.0f);
        cam.position = cp; cam.target = add(f.pos, vec(0, 1, 0));
        cam.fovy = clampf(2400.0f / std::max(20.0f, len(sub(cp, f.pos))), 12, 60);
    } else if (rpCam == 1) {
        V3 dir = fwdV(f.yaw);
        cam.position = add(sub(f.pos, mul(dir, 9)), vec(0, 3.2f, 0)); cam.target = add(f.pos, add(mul(dir, 8), vec(0, 1.2f, 0)));
    } else {
        float a = rpT * 0.4f;
        cam.target = add(f.pos, vec(0, 1, 0)); cam.position = add(f.pos, vec(sinf(a) * 11, 4, cosf(a) * 11));
    }
    cam.position = clampToCorridor(cam.position, f.lastIdx);
    if (indoorTrack()) cam.position.y = std::min(cam.position.y, 5.3f);
    camPos = cam.position; camFwd = norm(sub(cam.target, cam.position));
    std::swap(racers, rpRacers); std::swap(traffic, rpTraffic);
    std::vector<Particle> savedParts; std::swap(parts, savedParts);
    beginScene();
    renderRaceWorld(cam, false);
    endScene(0, 0);
    std::swap(parts, savedParts);
    std::swap(racers, rpRacers); std::swap(traffic, rpTraffic);
    if (!rpHideUI) {
        panel(16, 16, 360, 70, 0.6f);
        txtS(T("\320\237\320\236\320\222\320\242\320\236\320\240", "REPLAY"), 36, 20, 40, rpPause ? ACC : WHITE, 1);
        const char* camN[3] = {T("\320\242\320\222-\320\232\320\220\320\234\320\225\320\240\320\253", "TV CAMS"), T("\320\241\320\227\320\220\320\224\320\230", "CHASE"), T("\320\236\320\221\320\233\320\201\320\242", "ORBIT")};
        txt(TextFormat("x%.2g   %s   %s", speeds[rpSpeedIdx], camN[rpCam], fmtTime(rpT).c_str()), 200, 34, 17, WHITE);
        accentBar(16, 92, 360 * clampf(rpT / std::max(1.0f, t1 - t0), 0, 1), 4, ACC);
        float x = 30, y = SH - 52;
        panel(10, y - 10, 1060, 48, 0.5f);
        x += keycap(x, y, "SPACE", 28) + 8; txt(T("\320\277\320\260\321\203\320\267\320\260", "pause"), x, y + 5, 16, WHITE); x += 70;
        x += keycap(x, y, "<", 28) + 4; x += keycap(x, y, ">", 28) + 8; txt(T("\321\201\320\272\320\276\321\200\320\276\321\201\321\202\321\214", "speed"), x, y + 5, 16, WHITE); x += 90;
        x += keycap(x, y, "^", 28) + 4; x += keycap(x, y, "v", 28) + 8; txt(T("\320\263\320\276\320\275\321\211\320\270\320\272", "driver"), x, y + 5, 16, WHITE); x += 80;
        x += keycap(x, y, "V", 28) + 8; txt(T("\320\272\320\260\320\274\320\265\321\200\320\260", "camera"), x, y + 5, 16, WHITE); x += 80;
        x += keycap(x, y, "H", 28) + 8; txt(T("\321\201\320\272\321\200\321\213\321\202\321\214", "hide"), x, y + 5, 16, WHITE); x += 70;
        x += keycap(x, y, "ESC", 28) + 8; txt(T("\320\275\320\260\320\267\320\260\320\264", "back"), x, y + 5, 16, WHITE);
        if (rpFocus < (int)racers.size()) txtC(racers[rpFocus].name.c_str(), SW / 2, 30, 30, WHITE, 1);
    }
    EndDrawing();
    if (IsKeyPressed(KEY_ESCAPE) || gpPressed(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) { screen = SC_RESULTS; rpT = 0; play(sClick); }
}

// =====================================================================
//                                MAIN
// =====================================================================
int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SW, SH, "MEGA RACE 3D - Turbo Edition");
    SetWindowMinSize(960, 600);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
#if defined(RAYLIB_VERSION_MAJOR) && defined(RAYLIB_VERSION_MINOR)
#if (RAYLIB_VERSION_MAJOR > 5) || (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 5)
    rlSetClipPlanes(0.3, 3000.0);
#endif
#endif
    rlDisableBackfaceCulling();
    SetRandomSeed((unsigned)time(NULL));
    loadFonts();
    loadProfile();
    initAudio();
    initPost();
    genStaticTextures();
    genItemTex();
    initSky();
    for (int i = 0; i < 700; i++) rainD.push_back({frand(0, (float)SW), frand(0, (float)SH), frand(0.7f, 1.3f)});
    for (int i = 0; i < 450; i++) snowD.push_back({frand(0, (float)SW), frand(0, (float)SH), frand(0.2f, 1.0f)});
    buildTrack(S.track == 2 && !backroomsUnlocked() ? 0 : S.track);
    placeProps();
    genFacades(S.tod);
    garageBase = S.base;
    aP.musicStyle = 0;

    // automated screenshot mode for testing: MEGARACE_SHOT=scene:track:mode:tod:weather:pos:base:cam:frames:file
    const char* shot = getenv("MEGARACE_SHOT");
    int shotFrames = -1; std::string shotFile;
    if (shot) {
        char scene[32] = {0}, file[256] = {0}; int tr = 0, md = 0, tod = 0, w = 1, base = 0, camm = 0, fr = 120; float pos = 0;
        sscanf(shot, "%31[^:]:%d:%d:%d:%d:%f:%d:%d:%d:%255s", scene, &tr, &md, &tod, &w, &pos, &base, &camm, &fr, file);
        shotFrames = fr; shotFile = file;
        S.track = tr; S.mode = md; S.tod = tod; S.weather = w; S.base = (Base)base; P.backrooms = 1;
        std::string sc = scene;
        if (sc == "menu") screen = SC_MAIN;
        else if (sc == "garage") { screen = SC_GARAGE; garageMode = camm; garageBase = base; }
        else if (sc == "settings") screen = SC_SETTINGS;
        else if (sc == "controls") screen = SC_CONTROLS;
        else if (sc == "setup") screen = SC_SETUP;
        else if (sc == "career") { screen = SC_CAREER; P.career[0] = 1; P.career[1] = 2; P.money = 4200; careerSel = 2; }
        else if (sc == "ach") { screen = SC_ACH; P.ach = 0x5A3ull; P.wins = 2; P.totalTakedowns = 7; P.totalCoins = 64; }
        else {
            if (sc == "split") { S.players = 2; S.base2 = B_KART; }
            if (sc == "event") { S.base = B_CAR; savedS = S; careerEvent = base; }
            startRace(); autoPilot = true; cs.mode = camm; cs2.mode = camm;
            if (sc == "pause") { paused = true; autoPilot = false; }
            if (sc == "photo") { paused = true; photoMode = true; photoYaw = 2.2f; }
            int idx = ((int)(pos * N)) % N;
            for (auto& r : racers) { r.lastIdx = (idx - r.id * 6 + N) % N; respawn(r); r.invuln = 0; r.lap = 1; r.checkMask = 7; r.lapStart = 0; }
            countdown = 0.9f;
            if (sc == "hud") hintT = 100;
            if (sc == "results" || sc == "replay") { for (int k = 0; k < 400; k++) simulateRace(1 / 60.0f); for (size_t k = 0; k < racers.size(); k++) { if (racers[k].isCop) continue; racers[k].finished = true; racers[k].finishPlace = (int)k + 1; racers[k].finishTime = 360 + k * 7.3f; racers[k].bestLap = 170 + k * 3.1f; } int mp; computeOrder(mp); computeRewards(mp); screen = sc == "results" ? SC_RESULTS : SC_REPLAY; }
        }
    }
    int frame = 0;
    while (!quitGame && !WindowShouldClose()) {
        SW = GetScreenWidth(); SH = GetScreenHeight(); vpW = SW; vpH = SH; hudScale = 1.0f;
        float dt = std::min(GetFrameTime(), 0.033f);
        if (shot) dt = 1.0f / 60.0f;
        if (IsKeyPressed(KEY_F11)) { S.fullscreen = !S.fullscreen; ToggleFullscreen(); }
        menuT += dt;
        for (auto& n : notes) if (screen != SC_RACE && screen != SC_RESULTS) n.t -= dt;
        if (screen == SC_RACE || screen == SC_RESULTS) updateDrawRace(dt);
        else if (screen == SC_REPLAY) updateDrawReplay(dt);
        else {
            aP.master = 0; aP.siren = 0; aP.hum = 0; aP.musicStyle = 0;
            BeginDrawing();
            ClearBackground(BLACK);
            switch (screen) {
                case SC_MAIN: updateDrawMain(); break;
                case SC_SETUP: updateDrawSetup(); break;
                case SC_CAREER: updateDrawCareer(); break;
                case SC_GARAGE: updateDrawGarage(); break;
                case SC_SETTINGS: updateDrawSettings(); break;
                case SC_ACH: updateDrawAch(); break;
                default: updateDrawControls(false); break;
            }
            for (size_t i = 0; i < notes.size(); i++) {
                const Note& n = notes[i]; if (n.t <= 0) continue;
                float w = txtW(n.text.c_str(), 26, 1) + 50;
                panel(SW / 2 - w / 2, SH - 120 - i * 44, w, 38, 0.7f);
                txtC(n.text.c_str(), SW / 2, SH - 116 - i * 44, 26, alpha(n.c, clampf(n.t, 0, 1)), 1);
            }
            notes.erase(std::remove_if(notes.begin(), notes.end(), [](const Note& n) { return n.t <= 0; }), notes.end());
            EndDrawing();
            atmoInit = false;
        }
        updateAudio();
        updateOwnMusic(dt);
        frame++;
        if (shot && frame == shotFrames) { TakeScreenshot(shotFile.c_str()); break; }
    }
    saveProfile();
    if (audioOK) { UnloadAudioStream(engStream); CloseAudioDevice(); }
    CloseWindow();
    return 0;
}
