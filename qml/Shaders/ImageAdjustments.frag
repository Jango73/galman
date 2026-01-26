#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform qt_ubuf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float bloom;
    float brightness;
    float contrast;
    float pivot;
    float bloomCutoff;
    float bloomScale;
    float bloomMaximum;
    float contrastBase;
    float luminanceRed;
    float luminanceGreen;
    float luminanceBlue;
} ubuf;

void main()
{
    vec4 color = texture(source, qt_TexCoord0);
    color.rgb += ubuf.brightness;
    color.rgb = (color.rgb - vec3(ubuf.pivot)) * (ubuf.contrastBase + ubuf.contrast) + vec3(ubuf.pivot);

    float luminance = dot(color.rgb, vec3(ubuf.luminanceRed, ubuf.luminanceGreen, ubuf.luminanceBlue));
    float bloomValue = smoothstep(ubuf.bloomCutoff, ubuf.bloomMaximum, luminance) * ubuf.bloom * ubuf.bloomScale;
    color.rgb += bloomValue * color.rgb;

    fragColor = color * ubuf.qt_Opacity;
}
