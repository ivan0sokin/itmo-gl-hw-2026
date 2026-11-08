#version 330 core

uniform int iterations;

const int NUM_TRANSFORMS = 4;

uniform mat3 fs[NUM_TRANSFORMS];
uniform float cdf[NUM_TRANSFORMS];

uniform vec2 min_xy;
uniform vec2 max_xy;

float next_rand(inout uint state) {
    state = state * 1664525u + 1013904223u;
    return float(state >> 8u) / 16777216.0;
}

mat3 coefs_to_mat3(float a, float b, float c, float d, float e, float g) {
    return mat3(a, c, 0, b, d, 0, e, g, 1);
}

int get_index(float u) {
    for (int i = 0; i < NUM_TRANSFORMS; i++) {
        if (u < cdf[i]) return i;
    }
    return NUM_TRANSFORMS - 1;
}

void main() {
    uint rng_state = uint(gl_VertexID);

    vec3 xy = vec3(0, 0, 1);
    for (int t = 0; t < iterations; t++) {
        float rand = next_rand(rng_state);
        int i = get_index(rand);
        xy = fs[i] * xy;
    }

    vec2 scale = 2.0 / (max_xy - min_xy);
    vec2 offset = -(max_xy + min_xy) / (max_xy - min_xy);
    mat3 world_to_screen = coefs_to_mat3(scale.x, 0.0, 0.0, scale.y, offset.x, offset.y);

    xy = world_to_screen * xy;

    gl_Position = vec4(xy.xy, 0.0, 1.0);
    gl_PointSize = 1.0;
}
