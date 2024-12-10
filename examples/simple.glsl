#version 450

varying vec4 Position;
varying vec4 Color;

vec4 vert() {
    Color = vec4(Position.zyx, 1.0);

    return vec4(Position.xyz, 1.0);
}

vec4 frag() {
    return Color;
}
