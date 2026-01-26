#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;
layout(binding = 2) uniform sampler2D bloomSource;

layout(std140, binding = 0) uniform qt_ubuf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float bloom;
    float brightness;
    float contrast;
    float pivot;
    float bloomScale;
    float contrastBase;
} ubuf;

void main()
{
    vec4 baseColor = texture(source, qt_TexCoord0);
    vec4 bloomColor = texture(bloomSource, qt_TexCoord0);
    baseColor.rgb += ubuf.brightness;
    baseColor.rgb = (baseColor.rgb - vec3(ubuf.pivot)) * (ubuf.contrastBase + ubuf.contrast) + vec3(ubuf.pivot);
    baseColor.rgb += bloomColor.rgb * ubuf.bloom * ubuf.bloomScale;
    fragColor = baseColor * ubuf.qt_Opacity;
}
