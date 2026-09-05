/************************************************************************\

    Galman - Picture gallery manager
    Copyright (C) 2026 Jango73

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

\************************************************************************/

#include "ComfyRequirements.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>

namespace
{

const char requirementsFileName[] = "ComfyRequirements.json";
const char requirementsVersionKey[] = "version";
const char requirementsNodesKey[] = "nodes";
const char requirementsModelsKey[] = "models";
const char fieldClassType[] = "classType";
const char fieldPackageId[] = "packageId";
const char fieldRepositoryUrl[] = "repositoryUrl";
const char fieldRequiredFor[] = "requiredFor";
const char fieldFileName[] = "fileName";
const char fieldFolder[] = "folder";
const char fieldDownloadUrl[] = "downloadUrl";
const char fallbackPackageCore[] = "comfy-core";
const int expectedRequirementsVersion = 1;

QMap<QString, QString> defaultNodeClassTypes()
{
    return {
        {QStringLiteral("checkpointLoader"), QStringLiteral("CheckpointLoaderSimple")},
        {QStringLiteral("upscaleModelLoader"), QStringLiteral("UpscaleModelLoader")},
        {QStringLiteral("clipTextEncode"), QStringLiteral("CLIPTextEncode")},
        {QStringLiteral("emptyLatentImage"), QStringLiteral("EmptyLatentImage")},
        {QStringLiteral("sampler"), QStringLiteral("KSampler")},
        {QStringLiteral("vaeDecode"), QStringLiteral("VAEDecode")},
        {QStringLiteral("imageUpscaleWithModel"), QStringLiteral("ImageUpscaleWithModel")},
        {QStringLiteral("vaeEncode"), QStringLiteral("VAEEncode")},
        {QStringLiteral("tiledSampler"), QStringLiteral("BNK_TiledKSampler")},
        {QStringLiteral("faceDetailer"), QStringLiteral("DZ_Face_Detailer")},
        {QStringLiteral("saveImage"), QStringLiteral("SaveImage")},
        {QStringLiteral("unetLoader"), QStringLiteral("UNETLoader")},
        {QStringLiteral("modelSampling"), QStringLiteral("ModelSamplingSD3")},
        {QStringLiteral("clipLoader"), QStringLiteral("CLIPLoader")},
        {QStringLiteral("clipVisionLoader"), QStringLiteral("CLIPVisionLoader")},
        {QStringLiteral("vaeLoader"), QStringLiteral("VAELoader")},
        {QStringLiteral("loadImage"), QStringLiteral("LoadImage")},
        {QStringLiteral("imageScale"), QStringLiteral("ImageScale")},
        {QStringLiteral("clipVisionEncode"), QStringLiteral("CLIPVisionEncode")},
        {QStringLiteral("wanImageToVideo"), QStringLiteral("WanImageToVideo")},
        {QStringLiteral("videoCombine"), QStringLiteral("VHS_VideoCombine")},
    };
}

QMap<QString, QString> defaultNodePackageIds()
{
    return {
        {QStringLiteral("tiledSampler"), QStringLiteral("ComfyUI_TiledKSampler")},
        {QStringLiteral("faceDetailer"), QStringLiteral("DZ-FaceDetailer")},
        {QStringLiteral("videoCombine"), QStringLiteral("ComfyUI-VideoHelperSuite")},
    };
}

QMap<QString, QString> defaultNodeRepositories()
{
    return {
        {QStringLiteral("tiledSampler"), QStringLiteral("https://github.com/BlenderNeko/ComfyUI_TiledKSampler")},
        {QStringLiteral("faceDetailer"), QStringLiteral("https://github.com/nicofdga/DZ-FaceDetailer")},
        {QStringLiteral("videoCombine"), QStringLiteral("https://github.com/Kosinkadink/ComfyUI-VideoHelperSuite")},
    };
}

QMap<QString, QStringList> defaultNodeTargets()
{
    const QStringList imageOnly = {QStringLiteral("image")};
    const QStringList videoOnly = {QStringLiteral("video")};
    const QStringList shared = {QStringLiteral("image"), QStringLiteral("video")};
    return {
        {QStringLiteral("checkpointLoader"), imageOnly},
        {QStringLiteral("upscaleModelLoader"), imageOnly},
        {QStringLiteral("clipTextEncode"), shared},
        {QStringLiteral("emptyLatentImage"), imageOnly},
        {QStringLiteral("sampler"), shared},
        {QStringLiteral("vaeDecode"), shared},
        {QStringLiteral("imageUpscaleWithModel"), imageOnly},
        {QStringLiteral("vaeEncode"), imageOnly},
        {QStringLiteral("tiledSampler"), imageOnly},
        {QStringLiteral("faceDetailer"), imageOnly},
        {QStringLiteral("saveImage"), imageOnly},
        {QStringLiteral("unetLoader"), videoOnly},
        {QStringLiteral("modelSampling"), videoOnly},
        {QStringLiteral("clipLoader"), videoOnly},
        {QStringLiteral("clipVisionLoader"), videoOnly},
        {QStringLiteral("vaeLoader"), videoOnly},
        {QStringLiteral("loadImage"), videoOnly},
        {QStringLiteral("imageScale"), videoOnly},
        {QStringLiteral("clipVisionEncode"), videoOnly},
        {QStringLiteral("wanImageToVideo"), videoOnly},
        {QStringLiteral("videoCombine"), videoOnly},
    };
}

QMap<QString, QString> defaultModelFileNames()
{
    return {
        {QStringLiteral("checkpoint"), QStringLiteral("realisticVisionV60B1_v51HyperVAE.safetensors")},
        {QStringLiteral("upscale"), QStringLiteral("RealESRGAN_x2plus.pth")},
        {QStringLiteral("videoUnet"), QStringLiteral("wan2.1_i2v_480p_14B_fp8_scaled.safetensors")},
        {QStringLiteral("videoClip"), QStringLiteral("umt5_xxl_fp8_e4m3fn_scaled.safetensors")},
        {QStringLiteral("videoClipVision"), QStringLiteral("clip_vision_h.safetensors")},
        {QStringLiteral("videoVae"), QStringLiteral("wan_2.1_vae.safetensors")},
    };
}

QMap<QString, QString> defaultModelFolders()
{
    return {
        {QStringLiteral("checkpoint"), QStringLiteral("checkpoints")},
        {QStringLiteral("upscale"), QStringLiteral("upscale_models")},
        {QStringLiteral("videoUnet"), QStringLiteral("diffusion_models")},
        {QStringLiteral("videoClip"), QStringLiteral("text_encoders")},
        {QStringLiteral("videoClipVision"), QStringLiteral("clip_vision")},
        {QStringLiteral("videoVae"), QStringLiteral("vae")},
    };
}

QMap<QString, QStringList> defaultModelTargets()
{
    const QStringList imageOnly = {QStringLiteral("image")};
    const QStringList videoOnly = {QStringLiteral("video")};
    return {
        {QStringLiteral("checkpoint"), imageOnly},
        {QStringLiteral("upscale"), imageOnly},
        {QStringLiteral("videoUnet"), videoOnly},
        {QStringLiteral("videoClip"), videoOnly},
        {QStringLiteral("videoClipVision"), videoOnly},
        {QStringLiteral("videoVae"), videoOnly},
    };
}

QStringList candidateRequirementsPaths()
{
    const QString relativePath = QString::fromLatin1("data/comfy/%1").arg(QString::fromLatin1(requirementsFileName));
    QStringList candidates;
    candidates.append(QDir::current().filePath(relativePath));
    const QString appFolder = QCoreApplication::applicationDirPath();
    candidates.append(QDir(appFolder).filePath(QString::fromLatin1("../") + relativePath));
    candidates.append(QDir(appFolder).filePath(QString::fromLatin1("../share/galman/comfy/%1").arg(QString::fromLatin1(requirementsFileName))));
    candidates.append(QDir(appFolder).filePath(relativePath));
    return candidates;
}

QString resolveRequirementsFilePath()
{
    const QStringList candidates = candidateRequirementsPaths();
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate) && QFileInfo(candidate).isFile()) {
            return QDir(candidate).absolutePath();
        }
    }
    return QString();
}

QJsonObject loadRequirementsDocument()
{
    const QString path = resolveRequirementsFilePath();
    if (path.isEmpty()) {
        qWarning() << "ComfyRequirements file not found, using built-in defaults";
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ComfyRequirements cannot open file, using built-in defaults:" << path;
        return {};
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "ComfyRequirements invalid JSON, using built-in defaults:" << parseError.errorString();
        return {};
    }
    const QJsonObject root = document.object();
    if (root.value(QString::fromLatin1(requirementsVersionKey)).toInt(-1) != expectedRequirementsVersion) {
        qWarning() << "ComfyRequirements unsupported version, using built-in defaults:" << path;
        return {};
    }
    qInfo() << "ComfyRequirements loaded:" << path;
    return root;
}

struct RequirementsCache
{
    bool loaded = false;
    QJsonObject document;
};

RequirementsCache &requirementsCache()
{
    static RequirementsCache cache;
    return cache;
}

QJsonObject cachedRequirementsDocument()
{
    RequirementsCache &cache = requirementsCache();
    if (!cache.loaded) {
        cache.document = loadRequirementsDocument();
        cache.loaded = true;
        if (!cache.document.isEmpty()) {
            qInfo() << "ComfyRequirements ready, nodes="
                    << cache.document.value(QString::fromLatin1(requirementsNodesKey)).toObject().count()
                    << "models=" << cache.document.value(QString::fromLatin1(requirementsModelsKey)).toObject().count();
        }
    }
    return cache.document;
}

QJsonObject nodeSection()
{
    const QJsonObject root = cachedRequirementsDocument();
    const QJsonValue nodes = root.value(QString::fromLatin1(requirementsNodesKey));
    return nodes.isObject() ? nodes.toObject() : QJsonObject();
}

QJsonObject modelSection()
{
    const QJsonObject root = cachedRequirementsDocument();
    const QJsonValue models = root.value(QString::fromLatin1(requirementsModelsKey));
    return models.isObject() ? models.toObject() : QJsonObject();
}

QString readNodeField(const QString &nodeKey, const char *field, const QString &fallback)
{
    const QJsonObject entry = nodeSection().value(nodeKey).toObject();
    const QString value = entry.value(QString::fromLatin1(field)).toString().trimmed();
    if (value.isEmpty()) {
        return fallback;
    }
    return value;
}

QString readModelField(const QString &modelKey, const char *field, const QString &fallback)
{
    const QJsonObject entry = modelSection().value(modelKey).toObject();
    const QString value = entry.value(QString::fromLatin1(field)).toString().trimmed();
    if (value.isEmpty()) {
        return fallback;
    }
    return value;
}

QStringList readTargets(const QJsonObject &entry, const QStringList &fallback)
{
    const QJsonValue targets = entry.value(QString::fromLatin1(fieldRequiredFor));
    if (!targets.isArray() || targets.toArray().isEmpty()) {
        return fallback;
    }
    QStringList result;
    for (const QJsonValue &item : targets.toArray()) {
        const QString text = item.toString().trimmed();
        if (!text.isEmpty()) {
            result.append(text);
        }
    }
    return result.isEmpty() ? fallback : result;
}

} // namespace

QString ComfyRequirements::requirementsFilePath()
{
    return resolveRequirementsFilePath();
}

void ComfyRequirements::reload()
{
    qInfo() << "ComfyRequirements reload requested";
    RequirementsCache &cache = requirementsCache();
    cache.document = loadRequirementsDocument();
    cache.loaded = true;
}

QStringList ComfyRequirements::nodeKeys()
{
    QStringList keys = defaultNodeClassTypes().keys();
    keys.sort();
    return keys;
}

QStringList ComfyRequirements::modelKeys()
{
    QStringList keys = defaultModelFileNames().keys();
    keys.sort();
    return keys;
}

QVariantList ComfyRequirements::nodeEntries()
{
    qInfo() << "ComfyRequirements node entries requested";
    QVariantList entries;
    const QJsonObject nodes = nodeSection();
    const QMap<QString, QString> defaults = defaultNodeClassTypes();
    const QMap<QString, QString> packages = defaultNodePackageIds();
    const QMap<QString, QString> repositories = defaultNodeRepositories();
    const QMap<QString, QStringList> targets = defaultNodeTargets();
    for (const QString &key : nodeKeys()) {
        const QJsonObject entry = nodes.value(key).toObject();
        QVariantMap item;
        item.insert(QStringLiteral("key"), key);
        const QString classType = entry.value(QString::fromLatin1(fieldClassType)).toString().trimmed();
        item.insert(QStringLiteral("classType"), classType.isEmpty() ? defaults.value(key) : classType);
        const QString packageId = entry.value(QString::fromLatin1(fieldPackageId)).toString().trimmed();
        item.insert(QStringLiteral("packageId"),
                    packageId.isEmpty() ? packages.value(key, QString::fromLatin1(fallbackPackageCore)) : packageId);
        const QString repositoryUrl = entry.value(QString::fromLatin1(fieldRepositoryUrl)).toString().trimmed();
        item.insert(QStringLiteral("repositoryUrl"),
                    repositoryUrl.isEmpty() ? repositories.value(key) : repositoryUrl);
        item.insert(QStringLiteral("requiredFor"), QVariant(readTargets(entry, targets.value(key))));
        entries.append(item);
    }
    return entries;
}

QVariantList ComfyRequirements::modelEntries()
{
    qInfo() << "ComfyRequirements model entries requested";
    QVariantList entries;
    const QJsonObject models = modelSection();
    const QMap<QString, QString> fileNames = defaultModelFileNames();
    const QMap<QString, QString> folders = defaultModelFolders();
    const QMap<QString, QStringList> targets = defaultModelTargets();
    for (const QString &key : modelKeys()) {
        const QJsonObject entry = models.value(key).toObject();
        QVariantMap item;
        item.insert(QStringLiteral("key"), key);
        const QString fileName = entry.value(QString::fromLatin1(fieldFileName)).toString().trimmed();
        item.insert(QStringLiteral("fileName"), fileName.isEmpty() ? fileNames.value(key) : fileName);
        const QString folder = entry.value(QString::fromLatin1(fieldFolder)).toString().trimmed();
        item.insert(QStringLiteral("folder"), folder.isEmpty() ? folders.value(key) : folder);
        item.insert(QStringLiteral("downloadUrl"),
                    entry.value(QString::fromLatin1(fieldDownloadUrl)).toString().trimmed());
        item.insert(QStringLiteral("requiredFor"), QVariant(readTargets(entry, targets.value(key))));
        entries.append(item);
    }
    return entries;
}

QString ComfyRequirements::nodeClassType(const QString &nodeKey)
{
    return readNodeField(nodeKey, fieldClassType, defaultNodeClassTypes().value(nodeKey));
}

QString ComfyRequirements::nodePackageId(const QString &nodeKey)
{
    return readNodeField(nodeKey, fieldPackageId, defaultNodePackageIds().value(nodeKey, QString::fromLatin1(fallbackPackageCore)));
}

QString ComfyRequirements::nodeRepositoryUrl(const QString &nodeKey)
{
    return readNodeField(nodeKey, fieldRepositoryUrl, defaultNodeRepositories().value(nodeKey));
}

QString ComfyRequirements::modelFileName(const QString &modelKey)
{
    return readModelField(modelKey, fieldFileName, defaultModelFileNames().value(modelKey));
}

QString ComfyRequirements::modelFolder(const QString &modelKey)
{
    return readModelField(modelKey, fieldFolder, defaultModelFolders().value(modelKey));
}

QString ComfyRequirements::modelDownloadUrl(const QString &modelKey)
{
    return readModelField(modelKey, fieldDownloadUrl, QString());
}

QString ComfyRequirements::checkpointLoaderClassType()
{
    return nodeClassType(QStringLiteral("checkpointLoader"));
}

QString ComfyRequirements::upscaleModelLoaderClassType()
{
    return nodeClassType(QStringLiteral("upscaleModelLoader"));
}

QString ComfyRequirements::clipTextEncodeClassType()
{
    return nodeClassType(QStringLiteral("clipTextEncode"));
}

QString ComfyRequirements::emptyLatentImageClassType()
{
    return nodeClassType(QStringLiteral("emptyLatentImage"));
}

QString ComfyRequirements::samplerClassType()
{
    return nodeClassType(QStringLiteral("sampler"));
}

QString ComfyRequirements::vaeDecodeClassType()
{
    return nodeClassType(QStringLiteral("vaeDecode"));
}

QString ComfyRequirements::imageUpscaleWithModelClassType()
{
    return nodeClassType(QStringLiteral("imageUpscaleWithModel"));
}

QString ComfyRequirements::vaeEncodeClassType()
{
    return nodeClassType(QStringLiteral("vaeEncode"));
}

QString ComfyRequirements::tiledSamplerClassType()
{
    return nodeClassType(QStringLiteral("tiledSampler"));
}

QString ComfyRequirements::faceDetailerClassType()
{
    return nodeClassType(QStringLiteral("faceDetailer"));
}

QString ComfyRequirements::saveImageClassType()
{
    return nodeClassType(QStringLiteral("saveImage"));
}

QString ComfyRequirements::unetLoaderClassType()
{
    return nodeClassType(QStringLiteral("unetLoader"));
}

QString ComfyRequirements::modelSamplingClassType()
{
    return nodeClassType(QStringLiteral("modelSampling"));
}

QString ComfyRequirements::clipLoaderClassType()
{
    return nodeClassType(QStringLiteral("clipLoader"));
}

QString ComfyRequirements::clipVisionLoaderClassType()
{
    return nodeClassType(QStringLiteral("clipVisionLoader"));
}

QString ComfyRequirements::vaeLoaderClassType()
{
    return nodeClassType(QStringLiteral("vaeLoader"));
}

QString ComfyRequirements::loadImageClassType()
{
    return nodeClassType(QStringLiteral("loadImage"));
}

QString ComfyRequirements::imageScaleClassType()
{
    return nodeClassType(QStringLiteral("imageScale"));
}

QString ComfyRequirements::clipVisionEncodeClassType()
{
    return nodeClassType(QStringLiteral("clipVisionEncode"));
}

QString ComfyRequirements::wanImageToVideoClassType()
{
    return nodeClassType(QStringLiteral("wanImageToVideo"));
}

QString ComfyRequirements::videoCombineClassType()
{
    return nodeClassType(QStringLiteral("videoCombine"));
}

QString ComfyRequirements::checkpointFileName()
{
    return modelFileName(QStringLiteral("checkpoint"));
}

QString ComfyRequirements::upscaleFileName()
{
    return modelFileName(QStringLiteral("upscale"));
}

QString ComfyRequirements::videoUnetFileName()
{
    return modelFileName(QStringLiteral("videoUnet"));
}

QString ComfyRequirements::videoClipFileName()
{
    return modelFileName(QStringLiteral("videoClip"));
}

QString ComfyRequirements::videoClipVisionFileName()
{
    return modelFileName(QStringLiteral("videoClipVision"));
}

QString ComfyRequirements::videoVaeFileName()
{
    return modelFileName(QStringLiteral("videoVae"));
}
