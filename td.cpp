#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>

struct buffer_t
{
    char* buffer;
    uint32_t length;
};

#define LEVEL_WIDTH 10
#define LEVEL_HEIGHT 10

enum unit_type_t
{
    FREE,
    BLOCK,
    START,
    END,
    TOWER
};

struct level_unit_t
{
    unit_type_t unit_type;
    uint32_t simulation_reference;
};

struct enemy_t
{
    uint32_t id;

    Vector3 position;
    int hp;
    int dmg;
    float speed;

    int path_index;
};

struct simulation_t
{
    level_unit_t level[LEVEL_WIDTH][LEVEL_HEIGHT];

    std::vector<Vector3> path;
    std::vector<enemy_t> enemies;
};

// Vector3 approaching = {0, 0, 8};

Vector3 add(Vector3 a, Vector3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
Vector3 sub(Vector3 a, Vector3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vector3 div(Vector3 a, float b)
{
    return {a.x / b, a.y / b, a.z / b};
}
Vector3 mul(Vector3 a, float b)
{
    return {a.x * b, a.y * b, a.z * b};
}

float length(Vector3 a)
{
    return sqrt((a.x * a.x) + (a.y * a.y) + (a.z * a.z));
}

#define csv(a) a.x, a.y, a.z

void update_enemy(simulation_t& simulation, float delta, enemy_t& enemy)
{
    Vector3 approaching = simulation.path[enemy.path_index];
    auto diff = sub(approaching, enemy.position);

    // printf("%.3f %.3f %.3f\n", csv(diff));
    float diff_length = length(diff);
    if (diff_length < 0.05f)
    {
        enemy.position = approaching;
        enemy.path_index++;

        if (enemy.path_index >= simulation.path.size())
        {
            printf("im done with this shit.\n");
            enemy.path_index = simulation.path.size() - 1;
        }

        return;
    }

    auto direction = div(diff, diff_length);
    auto walking_distance = mul(direction, enemy.speed);
    auto walking_distance_delta = mul(walking_distance, delta);

    float wdd_length = length(walking_distance_delta);
    if (wdd_length > diff_length)
    {
        enemy.position = approaching;
        enemy.path_index++;
        if (enemy.path_index >= simulation.path.size())
        {
            printf("im done with this shit.\n");
            enemy.path_index = simulation.path.size() - 1;

            return;
        }
    }
    else
    {
        enemy.position = add(enemy.position, walking_distance_delta);
    }
}

void update_simulation(simulation_t& simulation, float delta)
{

    for (int i = 0; i < simulation.enemies.size(); i++)
    {
        auto& enemy = simulation.enemies[i];
        update_enemy(simulation, delta, enemy);
    }
}

buffer_t read_entire_file(const char* file_name)
{
    FILE* f = fopen(file_name, "rb");
    if (!f)
    {
        printf("could not open file: %s\n", file_name);
        exit(0);
    }

    fseek(f, 0, SEEK_END);

    size_t buffer_size = ftell(f);
    printf("%lu\n", buffer_size);
    fseek(f, 0, SEEK_SET);

    void* buffer = malloc(buffer_size);

    fread(buffer, buffer_size, 1, f);

    return {(char*)buffer, (uint32_t)buffer_size};
}

void write_entire_file(const char* file_name, buffer_t buffer)
{
    FILE* f = fopen(file_name, "wb");
    if (!f)
    {
        printf("could not open file: %s\n", file_name);
        exit(0);
    }

    fwrite(buffer.buffer, buffer.length, 1, f);
}

void update_path(simulation_t& simulation)
{

    simulation.path = {{0, 0, 8}, {0, 0, 6}, {7, 0, 6}};
}

int main()
{
    auto level_file = read_entire_file("level_0.csv");
    printf("read file:\n%.*s\n", level_file.length, level_file.buffer);

    simulation_t simulation;
    enemy_t enemy;
    enemy.path_index = 0;
    enemy.speed = 3;
    enemy.position = {8, 0, 8};
    simulation.enemies.push_back(enemy);

    update_path(simulation);

    memset(simulation.level, 0, sizeof(simulation.level));
    simulation.level[0][0] = {TOWER, 0};

    auto& level = simulation.level;

    uint32_t cursor = 0;
    int x = 0, y = 0;

    while (cursor != level_file.length)
    {

        char c = level_file.buffer[cursor];
        switch (c)
        {
        case ',':
            y++;
            break;
        case 'E':
            level[x][y] = {END};
            break;
        case 'S':
            level[x][y] = {START};
            break;
        case 'B':
            level[x][y] = {BLOCK};
            break;
        case '\n':
            y = 0;
            x++;
            break;
        default:
            printf("unhandled file loading case: %c\n", c);
        }
        cursor++;
    }

    for (int i = 0; i < LEVEL_WIDTH; i++)
    {
        for (int j = 0; j < LEVEL_HEIGHT; j++)
        {
            printf("%d ", level[j][i].unit_type);
        }
        puts("");
    }

    InitWindow(1600, 900, "yeet");
    // SetTargetFPS(20);
    Camera3D camera = {0};
    camera.position = (Vector3){0.0f, 10.0f, 10.0f}; // Camera position
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};     // Camera looking at point
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};         // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                             // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;          // Camera mode type

    Vector3 global_map_offet = {-4.5f, 0, -4.5f};
    auto tower_model = LoadModel("tower.glb");

    int camera_mode = CAMERA_CUSTOM;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_ONE))
        {
            camera_mode = CAMERA_FIRST_PERSON;
        }
        if (IsKeyPressed(KEY_TWO))
        {
            camera_mode = CAMERA_CUSTOM;
        }
        if (IsKeyPressed(KEY_THREE))
        {
            camera_mode = CAMERA_ORBITAL;
        }

        float delta = GetFrameTime();
        update_simulation(simulation, delta);

        UpdateCamera(&camera, camera_mode);
        ClearBackground(BLACK);
        BeginDrawing();

        DrawLine(0, 0, 500, 500, PINK);

        BeginMode3D(camera);
        DrawGrid(10, 1.0f);

        for (int i = 0; i < LEVEL_WIDTH; i++)
        {
            for (int j = 0; j < LEVEL_HEIGHT; j++)
            {
                auto& element = level[i][j];
                switch (element.unit_type)
                {
                case BLOCK:
                    DrawCube(add(global_map_offet, {(float)j, 0.25, (float)i}), 1, 0.5f, 1, BROWN);
                    break;
                case START:
                    DrawCube(add(global_map_offet, {(float)j, 0.1, (float)i}), 1, 0.2f, 1, RED);
                    break;
                case END:
                    DrawCube(add(global_map_offet, {(float)j, 0.1, (float)i}), 1, 0.2f, 1, GREEN);
                    break;
                case FREE:
                    DrawPlane(add(global_map_offet, {(float)j, 0, (float)i}), {1, 1}, LIME);
                    break;
                case TOWER:
                    DrawModel(tower_model, add(global_map_offet, {(float)j, 0, (float)i}), 1, PURPLE);
                    break;
                }
            }
        }

        for (int i = 0; i < simulation.enemies.size(); i++)
        {
            DrawSphere(add(global_map_offet, simulation.enemies[i].position), 0.5f, BEIGE);
        }

        for (int i = 0; i < simulation.path.size(); i++)
            DrawSphere(add(global_map_offet, simulation.path[i]), 0.25f, GREEN);

        EndMode3D();

        EndDrawing();
    }
}

// unterhaltungsindustrie
//   games

// rüstung

// banking
// verwaltungssoftware