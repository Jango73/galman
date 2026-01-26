#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform qt_ubuf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 texelSize;
    vec2 direction;
    float weight0;
    float weight1;
    float weight2;
    float weight3;
    float weight4;
    float weight5;
    float weight6;
    float weight7;
    float weight8;
    float offset1;
    float offset2;
    float offset3;
    float offset4;
    float offset5;
    float offset6;
    float offset7;
    float offset8;
} ubuf;

void main()
{
    vec2 stepOffset = ubuf.texelSize * ubuf.direction;
    vec4 color = texture(source, qt_TexCoord0) * ubuf.weight0;
    color += texture(source, qt_TexCoord0 + stepOffset * ubuf.offset1) * ubuf.weight1;
    color += texture(source, qt_TexCoord0 - stepOffset * ubuf.offset1) * ubuf.weight1;
    color += texture(source, qt_TexCoord0 + stepOffset * ubuf.offset2) * ubuf.weight2;
    color += texture(source, qt_TexCoord0 - stepOffset * ubuf.offset2) * ubuf.weight2;
    color += texture(source, qt_TexCoord0 + stepOffset * ubuf.offset3) * ubuf.weight3;
    color += texture(source, qt_TexCoord0 - stepOffset * ubuf.offset3) * ubuf.weight3;
    color += texture(source, qt_TexCoord0 + stepOffset * ubuf.offset4) * ubuf.weight4;
    color += texture(source, qt_TexCoord0 - stepOffset * ubuf.offset4) * ubuf.weight4;
    color += texture(source, qt_TexCoord0 + stepOffset * ubuf.offset5) * ubuf.weight5;
    color += texture(source, qt_TexCoord0 - stepOffset * ubuf.offset5) * ubuf.weight5;
    color += texture(source, qt_TexCoord0 + stepOffset * ubuf.offset6) * ubuf.weight6;
    color += texture(source, qt_TexCoord0 - stepOffset * ubuf.offset6) * ubuf.weight6;
    color += texture(source, qt_TexCoord0 + stepOffset * ubuf.offset7) * ubuf.weight7;
    color += texture(source, qt_TexCoord0 - stepOffset * ubuf.offset7) * ubuf.weight7;
    color += texture(source, qt_TexCoord0 + stepOffset * ubuf.offset8) * ubuf.weight8;
    color += texture(source, qt_TexCoord0 - stepOffset * ubuf.offset8) * ubuf.weight8;
    fragColor = color * ubuf.qt_Opacity;
}
