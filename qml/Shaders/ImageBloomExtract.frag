#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform qt_ubuf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float bloomCutoff;
    float bloomMaximum;
    float luminanceRed;
    float luminanceGreen;
    float luminanceBlue;
} ubuf;

void main()
{
    vec4 color = texture(source, qt_TexCoord0);
    float luminance = dot(color.rgb, vec3(ubuf.luminanceRed, ubuf.luminanceGreen, ubuf.luminanceBlue));
    float strength = smoothstep(ubuf.bloomCutoff, ubuf.bloomMaximum, luminance);
    fragColor = vec4(color.rgb * strength, color.a) * ubuf.qt_Opacity;
}
