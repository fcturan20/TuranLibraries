#pragma once

typedef struct uvec3_tgfx {
    unsigned int x, y, z;
} uvec3_tgfx;

typedef struct vec2_tgfx {
    float x, y;
} vec2_tgfx;

typedef struct vec3_tgfx {
    float x, y, z;
} vec3_tgfx;

typedef struct vec4_tgfx {
    float x, y, z, w;
} vec4_tgfx;

typedef struct boxregion_tgfx {
    unsigned int XOffset, YOffset, WIDTH, HEIGHT;
} boxregion_tgfx;

typedef struct cuberegion_tgfx {
    unsigned int XOffset, YOffset, ZOffset, WIDTH, HEIGHT, DEPTH;
} cuberegion_tgfx;