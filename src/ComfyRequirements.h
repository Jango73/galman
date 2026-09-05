#pragma once

#include <QString>
#include <QStringList>
#include <QVariantList>

class ComfyRequirements
{
public:
    static QString requirementsFilePath();
    static void reload();

    static QStringList nodeKeys();
    static QStringList modelKeys();
    static QVariantList nodeEntries();
    static QVariantList modelEntries();

    static QString nodeClassType(const QString &nodeKey);
    static QString nodePackageId(const QString &nodeKey);
    static QString nodeRepositoryUrl(const QString &nodeKey);
    static QString modelFileName(const QString &modelKey);
    static QString modelFolder(const QString &modelKey);
    static QString modelDownloadUrl(const QString &modelKey);

    static QString checkpointLoaderClassType();
    static QString upscaleModelLoaderClassType();
    static QString clipTextEncodeClassType();
    static QString emptyLatentImageClassType();
    static QString samplerClassType();
    static QString vaeDecodeClassType();
    static QString imageUpscaleWithModelClassType();
    static QString vaeEncodeClassType();
    static QString tiledSamplerClassType();
    static QString faceDetailerClassType();
    static QString saveImageClassType();
    static QString unetLoaderClassType();
    static QString modelSamplingClassType();
    static QString clipLoaderClassType();
    static QString clipVisionLoaderClassType();
    static QString vaeLoaderClassType();
    static QString loadImageClassType();
    static QString imageScaleClassType();
    static QString clipVisionEncodeClassType();
    static QString wanImageToVideoClassType();
    static QString videoCombineClassType();

    static QString checkpointFileName();
    static QString upscaleFileName();
    static QString videoUnetFileName();
    static QString videoClipFileName();
    static QString videoClipVisionFileName();
    static QString videoVaeFileName();
};
