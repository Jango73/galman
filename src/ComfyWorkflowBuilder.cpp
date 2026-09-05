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

#include "ComfyWorkflowBuilder.h"
#include "ApplicationInfo.h"
#include "ComfyRequirements.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QLoggingCategory>

namespace
{

struct BuilderLimits
{
    static constexpr int tileSize = 384;
    static constexpr int latentBatchSize = 1;
    static constexpr int faceMaskBlur = 0;
    static constexpr int faceDilateValue = 3;
    static constexpr int faceErodeValue = 3;
    static constexpr double videoModelShift = 8.0;
    static constexpr double videoDenoise = 1.0;
    static constexpr int videoBatchSize = 1;
    static constexpr int videoLoopCount = 0;
    static constexpr int videoCombineCrf = 19;
    static constexpr int videoFrameBlock = 4;
};

QJsonArray latentLink(const QString &nodeId)
{
    QJsonArray link;
    link.append(nodeId);
    link.append(0);
    return link;
}

void insertPromptNode(QJsonObject &prompt,
                      const QString &nodeId,
                      const QString &classType,
                      const QJsonObject &inputs)
{
    QJsonObject node;
    node.insert(QStringLiteral("class_type"), classType);
    node.insert(QStringLiteral("inputs"), inputs);
    prompt.insert(nodeId, node);
}

} // namespace

QString ComfyPilotDefaults::serverUrl()
{
    return QStringLiteral("http://127.0.0.1:8188");
}

QString ComfyPilotDefaults::actionPreview()
{
    return QStringLiteral("preview");
}

QString ComfyPilotDefaults::actionNextSeed()
{
    return QStringLiteral("nextSeed");
}

QString ComfyPilotDefaults::actionGenerate()
{
    return QStringLiteral("generate");
}

QString ComfyWorkflowBuilder::ModelConstants::checkpointName()
{
    return ComfyRequirements::checkpointFileName();
}

QString ComfyWorkflowBuilder::ModelConstants::upscaleModelName()
{
    return ComfyRequirements::upscaleFileName();
}

QString ComfyWorkflowBuilder::ModelConstants::samplerName()
{
    return QStringLiteral("euler_ancestral");
}

QString ComfyWorkflowBuilder::ModelConstants::schedulerName()
{
    return QStringLiteral("normal");
}

QString ComfyWorkflowBuilder::ModelConstants::tilingStrategy()
{
    return QStringLiteral("padded");
}

QString ComfyWorkflowBuilder::ModelConstants::faceMaskType()
{
    return QStringLiteral("face");
}

QString ComfyWorkflowBuilder::ModelConstants::faceMaskControl()
{
    return QStringLiteral("dilate");
}

QString ComfyWorkflowBuilder::ModelConstants::savePrefix()
{
    return ApplicationInfo::applicationName();
}

QString ComfyWorkflowBuilder::ModelConstants::videoUnetName()
{
    return ComfyRequirements::videoUnetFileName();
}

QString ComfyWorkflowBuilder::ModelConstants::videoClipName()
{
    return ComfyRequirements::videoClipFileName();
}

QString ComfyWorkflowBuilder::ModelConstants::videoClipVisionName()
{
    return ComfyRequirements::videoClipVisionFileName();
}

QString ComfyWorkflowBuilder::ModelConstants::videoVaeName()
{
    return ComfyRequirements::videoVaeFileName();
}

QString ComfyWorkflowBuilder::ModelConstants::videoWeightDtype()
{
    return QStringLiteral("default");
}

QString ComfyWorkflowBuilder::ModelConstants::videoClipType()
{
    return QStringLiteral("wan");
}

QString ComfyWorkflowBuilder::ModelConstants::videoClipDevice()
{
    return QStringLiteral("default");
}

QString ComfyWorkflowBuilder::ModelConstants::videoSamplerName()
{
    return QStringLiteral("uni_pc");
}

QString ComfyWorkflowBuilder::ModelConstants::videoSchedulerName()
{
    return QStringLiteral("simple");
}

QString ComfyWorkflowBuilder::ModelConstants::videoUpscaleMethod()
{
    return QStringLiteral("nearest-exact");
}

QString ComfyWorkflowBuilder::ModelConstants::videoUpscaleCrop()
{
    return QStringLiteral("disabled");
}

QString ComfyWorkflowBuilder::ModelConstants::videoVisionCrop()
{
    return QStringLiteral("none");
}

QString ComfyWorkflowBuilder::ModelConstants::videoCombineFormat()
{
    return QStringLiteral("video/h264-mp4");
}

QString ComfyWorkflowBuilder::ModelConstants::videoCombinePixelFormat()
{
    return QStringLiteral("yuv420p");
}

QString ComfyWorkflowBuilder::ModelConstants::videoCombinePrefix()
{
    return QStringLiteral("wan");
}

/**
 * @brief Returns the default filename prefix for generated outputs.
 * @return Default save prefix.
 */
QString ComfyWorkflowBuilder::defaultSavePrefix()
{
    return ModelConstants::savePrefix();
}

/**
 * @brief Returns the filename prefix forcing cache-busting re-execution.
 * @return Retry save prefix, differing from the default to miss the server cache.
 */
QString ComfyWorkflowBuilder::retrySavePrefix()
{
    return ModelConstants::savePrefix() + QStringLiteral("-regen");
}

/**
 * @brief Builds a minimal ComfyUI API prompt from pilot parameters.
 * @param params Validated pilot parameters.
 * @param error Optional output error message.
 * @param savePrefixOverride Optional filename prefix replacing the default one.
 * @return Prompt object keyed by node id, or empty object on validation failure.
 */
QJsonObject ComfyWorkflowBuilder::buildPrompt(const ComfyPilotParameters &params,
                                              QString *error,
                                              const QString &savePrefixOverride)
{
    if (params.videoEnabled) {
        return buildVideoPrompt(params, error, savePrefixOverride);
    }
    if (params.canvasWidth < ComfyPilotDefaults::minCanvasSize
        || params.canvasWidth > ComfyPilotDefaults::maxCanvasSize
        || params.canvasHeight < ComfyPilotDefaults::minCanvasSize
        || params.canvasHeight > ComfyPilotDefaults::maxCanvasSize) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid canvas size");
        }
        return {};
    }
    if (params.refineCount < ComfyPilotDefaults::minRefineCount
        || params.refineCount > ComfyPilotDefaults::maxRefineCount) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid refine count");
        }
        return {};
    }
    if (params.initialSteps < ComfyPilotDefaults::minSteps || params.initialSteps > ComfyPilotDefaults::maxSteps
        || params.refineSteps < ComfyPilotDefaults::minSteps || params.refineSteps > ComfyPilotDefaults::maxSteps) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid steps value");
        }
        return {};
    }
    if (params.initialGuidance < ComfyPilotDefaults::minGuidance
        || params.initialGuidance > ComfyPilotDefaults::maxGuidance
        || params.refineGuidance < ComfyPilotDefaults::minGuidance
        || params.refineGuidance > ComfyPilotDefaults::maxGuidance) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid guidance value");
        }
        return {};
    }
    if (params.initialDenoise < ComfyPilotDefaults::minDenoise
        || params.initialDenoise > ComfyPilotDefaults::maxDenoise
        || params.refineDenoise < ComfyPilotDefaults::minDenoise
        || params.refineDenoise > ComfyPilotDefaults::maxDenoise) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid denoise value");
        }
        return {};
    }

    qInfo() << "ComfyWorkflowBuilder build start:"
            << "canvas=" << params.canvasWidth << "x" << params.canvasHeight
            << "refines=" << params.refineCount
            << "face=" << params.faceDetail
            << "emptyRefine=" << params.emptyRefinePrompt
            << "seed=" << params.seed;

    QJsonObject prompt;
    int nextId = 1;
    const auto newId = [&nextId]() {
        return QString::number(nextId++);
    };

    const QString checkpointId = newId();
    const QString upscaleLoaderId = newId();
    const QString positiveId = newId();
    const QString negativeId = newId();
    const QString latentId = newId();
    const QString samplerId = newId();

    QJsonObject checkpointInputs;
    checkpointInputs.insert(QStringLiteral("ckpt_name"), ModelConstants::checkpointName());
    insertPromptNode(prompt, checkpointId, ComfyRequirements::checkpointLoaderClassType(), checkpointInputs);

    QJsonObject upscaleLoaderInputs;
    upscaleLoaderInputs.insert(QStringLiteral("model_name"), ModelConstants::upscaleModelName());
    insertPromptNode(prompt, upscaleLoaderId, ComfyRequirements::upscaleModelLoaderClassType(), upscaleLoaderInputs);

    QJsonObject positiveInputs;
    QJsonArray positiveClip;
    positiveClip.append(checkpointId);
    positiveClip.append(1);
    positiveInputs.insert(QStringLiteral("clip"), positiveClip);
    positiveInputs.insert(QStringLiteral("text"), params.positivePrompt);
    insertPromptNode(prompt, positiveId, ComfyRequirements::clipTextEncodeClassType(), positiveInputs);

    QJsonObject negativeInputs;
    QJsonArray negativeClip;
    negativeClip.append(checkpointId);
    negativeClip.append(1);
    negativeInputs.insert(QStringLiteral("clip"), negativeClip);
    negativeInputs.insert(QStringLiteral("text"), params.negativePrompt);
    insertPromptNode(prompt, negativeId, ComfyRequirements::clipTextEncodeClassType(), negativeInputs);

    QString emptyClipId;
    if (params.emptyRefinePrompt && (params.refineCount > 0 || params.faceDetail)) {
        emptyClipId = newId();
        QJsonObject emptyInputs;
        emptyInputs.insert(QStringLiteral("clip"), positiveClip);
        emptyInputs.insert(QStringLiteral("text"), QString());
        insertPromptNode(prompt, emptyClipId, ComfyRequirements::clipTextEncodeClassType(), emptyInputs);
    }
    const QString refinePositiveId = emptyClipId.isEmpty() ? positiveId : emptyClipId;
    const QString refineNegativeId = emptyClipId.isEmpty() ? negativeId : emptyClipId;

    QJsonObject latentInputs;
    latentInputs.insert(QStringLiteral("width"), params.canvasWidth);
    latentInputs.insert(QStringLiteral("height"), params.canvasHeight);
    latentInputs.insert(QStringLiteral("batch_size"), BuilderLimits::latentBatchSize);
    insertPromptNode(prompt, latentId, ComfyRequirements::emptyLatentImageClassType(), latentInputs);

    QJsonObject samplerInputs;
    QJsonArray modelLink;
    modelLink.append(checkpointId);
    modelLink.append(0);
    samplerInputs.insert(QStringLiteral("model"), modelLink);
    samplerInputs.insert(QStringLiteral("positive"), latentLink(positiveId));
    samplerInputs.insert(QStringLiteral("negative"), latentLink(negativeId));
    samplerInputs.insert(QStringLiteral("latent_image"), latentLink(latentId));
    samplerInputs.insert(QStringLiteral("seed"), params.seed);
    samplerInputs.insert(QStringLiteral("steps"), params.initialSteps);
    samplerInputs.insert(QStringLiteral("cfg"), params.initialGuidance);
    samplerInputs.insert(QStringLiteral("sampler_name"), ModelConstants::samplerName());
    samplerInputs.insert(QStringLiteral("scheduler"), ModelConstants::schedulerName());
    samplerInputs.insert(QStringLiteral("denoise"), params.initialDenoise);
    insertPromptNode(prompt, samplerId, ComfyRequirements::samplerClassType(), samplerInputs);

    QString currentLatentId = samplerId;

    for (int index = 0; index < params.refineCount; ++index) {
        const QString decodeId = newId();
        const QString upscaleId = newId();
        const QString encodeId = newId();
        const QString tiledId = newId();

        QJsonObject decodeInputs;
        decodeInputs.insert(QStringLiteral("samples"), latentLink(currentLatentId));
        QJsonArray decodeVae;
        decodeVae.append(checkpointId);
        decodeVae.append(2);
        decodeInputs.insert(QStringLiteral("vae"), decodeVae);
        insertPromptNode(prompt, decodeId, ComfyRequirements::vaeDecodeClassType(), decodeInputs);

        QJsonObject upscaleInputs;
        QJsonArray upscaleModelLink;
        upscaleModelLink.append(upscaleLoaderId);
        upscaleModelLink.append(0);
        upscaleInputs.insert(QStringLiteral("upscale_model"), upscaleModelLink);
        upscaleInputs.insert(QStringLiteral("image"), latentLink(decodeId));
        insertPromptNode(prompt, upscaleId, ComfyRequirements::imageUpscaleWithModelClassType(), upscaleInputs);

        QJsonObject encodeInputs;
        encodeInputs.insert(QStringLiteral("pixels"), latentLink(upscaleId));
        QJsonArray encodeVae;
        encodeVae.append(checkpointId);
        encodeVae.append(2);
        encodeInputs.insert(QStringLiteral("vae"), encodeVae);
        insertPromptNode(prompt, encodeId, ComfyRequirements::vaeEncodeClassType(), encodeInputs);

        QJsonObject tiledInputs;
        tiledInputs.insert(QStringLiteral("model"), modelLink);
        tiledInputs.insert(QStringLiteral("positive"), latentLink(refinePositiveId));
        tiledInputs.insert(QStringLiteral("negative"), latentLink(refineNegativeId));
        tiledInputs.insert(QStringLiteral("latent_image"), latentLink(encodeId));
        tiledInputs.insert(QStringLiteral("seed"), params.seed);
        tiledInputs.insert(QStringLiteral("tile_width"), BuilderLimits::tileSize);
        tiledInputs.insert(QStringLiteral("tile_height"), BuilderLimits::tileSize);
        tiledInputs.insert(QStringLiteral("tiling_strategy"), ModelConstants::tilingStrategy());
        tiledInputs.insert(QStringLiteral("steps"), params.refineSteps);
        tiledInputs.insert(QStringLiteral("cfg"), params.refineGuidance);
        tiledInputs.insert(QStringLiteral("sampler_name"), ModelConstants::samplerName());
        tiledInputs.insert(QStringLiteral("scheduler"), ModelConstants::schedulerName());
        tiledInputs.insert(QStringLiteral("denoise"), params.refineDenoise);
        insertPromptNode(prompt, tiledId, ComfyRequirements::tiledSamplerClassType(), tiledInputs);

        currentLatentId = tiledId;
    }

    if (params.faceDetail) {
        const QString faceId = newId();
        QJsonObject faceInputs;
        faceInputs.insert(QStringLiteral("model"), modelLink);
        faceInputs.insert(QStringLiteral("positive"), latentLink(refinePositiveId));
        faceInputs.insert(QStringLiteral("negative"), latentLink(refineNegativeId));
        faceInputs.insert(QStringLiteral("latent_image"), latentLink(currentLatentId));
        QJsonArray faceVae;
        faceVae.append(checkpointId);
        faceVae.append(2);
        faceInputs.insert(QStringLiteral("vae"), faceVae);
        faceInputs.insert(QStringLiteral("seed"), params.seed);
        faceInputs.insert(QStringLiteral("steps"), params.refineSteps);
        faceInputs.insert(QStringLiteral("cfg"), params.refineGuidance);
        faceInputs.insert(QStringLiteral("sampler_name"), ModelConstants::samplerName());
        faceInputs.insert(QStringLiteral("scheduler"), ModelConstants::schedulerName());
        faceInputs.insert(QStringLiteral("denoise"), params.refineDenoise);
        faceInputs.insert(QStringLiteral("mask_blur"), BuilderLimits::faceMaskBlur);
        faceInputs.insert(QStringLiteral("mask_type"), ModelConstants::faceMaskType());
        faceInputs.insert(QStringLiteral("mask_control"), ModelConstants::faceMaskControl());
        faceInputs.insert(QStringLiteral("dilate_mask_value"), BuilderLimits::faceDilateValue);
        faceInputs.insert(QStringLiteral("erode_mask_value"), BuilderLimits::faceErodeValue);
        insertPromptNode(prompt, faceId, ComfyRequirements::faceDetailerClassType(), faceInputs);
        currentLatentId = faceId;
    }

    const QString finalDecodeId = newId();
    QJsonObject finalDecodeInputs;
    finalDecodeInputs.insert(QStringLiteral("samples"), latentLink(currentLatentId));
    QJsonArray finalVae;
    finalVae.append(checkpointId);
    finalVae.append(2);
    finalDecodeInputs.insert(QStringLiteral("vae"), finalVae);
    insertPromptNode(prompt, finalDecodeId, ComfyRequirements::vaeDecodeClassType(), finalDecodeInputs);

    const QString saveId = newId();
    QJsonObject saveInputs;
    saveInputs.insert(QStringLiteral("images"), latentLink(finalDecodeId));
    saveInputs.insert(QStringLiteral("filename_prefix"),
                      savePrefixOverride.isEmpty() ? ModelConstants::savePrefix() : savePrefixOverride);
    insertPromptNode(prompt, saveId, ComfyRequirements::saveImageClassType(), saveInputs);

    qInfo() << "ComfyWorkflowBuilder build done: nodes=" << prompt.count()
            << "refines=" << params.refineCount << "face=" << params.faceDetail;
    return prompt;
}

/**
 * @brief Builds an image-to-video ComfyUI API prompt from pilot parameters.
 * @param params Validated pilot parameters, video mode must be enabled.
 * @param error Optional output error message.
 * @param savePrefixOverride Optional filename prefix replacing the default one.
 * @return Prompt object keyed by node id, or empty object on validation failure.
 */
QJsonObject ComfyWorkflowBuilder::buildVideoPrompt(const ComfyPilotParameters &params,
                                                  QString *error,
                                                  const QString &savePrefixOverride)
{
    if (params.canvasWidth < ComfyPilotDefaults::minCanvasSize
        || params.canvasWidth > ComfyPilotDefaults::maxCanvasSize
        || params.canvasHeight < ComfyPilotDefaults::minCanvasSize
        || params.canvasHeight > ComfyPilotDefaults::maxCanvasSize) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid canvas size");
        }
        return {};
    }
    if (params.refineSteps < ComfyPilotDefaults::minSteps || params.refineSteps > ComfyPilotDefaults::maxSteps) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid steps value");
        }
        return {};
    }
    if (params.refineGuidance < ComfyPilotDefaults::minGuidance
        || params.refineGuidance > ComfyPilotDefaults::maxGuidance) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid guidance value");
        }
        return {};
    }
    if (params.videoDuration < ComfyPilotDefaults::minVideoDuration
        || params.videoDuration > ComfyPilotDefaults::maxVideoDuration) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid video duration");
        }
        return {};
    }
    if (params.videoFrameRate < ComfyPilotDefaults::minVideoFrameRate
        || params.videoFrameRate > ComfyPilotDefaults::maxVideoFrameRate) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid video frame rate");
        }
        return {};
    }
    if (params.canvasSize < ComfyPilotDefaults::minVideoCanvasSize
        || params.canvasSize > ComfyPilotDefaults::maxVideoCanvasSize) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Invalid canvas size");
        }
        return {};
    }
    if (!params.useCurrentImage) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Video input image is not enabled");
        }
        return {};
    }
    if (params.videoInputFileName.trimmed().isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowBuilder", "Missing video input image");
        }
        return {};
    }

    const int requestedFrames = params.videoDuration * params.videoFrameRate;
    const int frameRemainder = (requestedFrames - 1) % BuilderLimits::videoFrameBlock;
    const int frameCount = requestedFrames
        + (BuilderLimits::videoFrameBlock - frameRemainder) % BuilderLimits::videoFrameBlock;
    qInfo() << "ComfyWorkflowBuilder video build start:"
            << "canvas=" << params.canvasWidth << "x" << params.canvasHeight
            << "frames=" << frameCount
            << "rate=" << params.videoFrameRate
            << "seed=" << params.seed;

    QJsonObject prompt;
    int nextId = 1;
    const auto newId = [&nextId]() {
        return QString::number(nextId++);
    };

    const QString unetId = newId();
    const QString samplingId = newId();
    const QString clipId = newId();
    const QString clipVisionLoaderId = newId();
    const QString vaeId = newId();
    const QString positiveId = newId();
    const QString negativeId = newId();
    const QString loadImageId = newId();
    const QString scaleImageId = newId();
    const QString clipVisionEncodeId = newId();
    const QString wanId = newId();
    const QString samplerId = newId();
    const QString decodeId = newId();
    const QString combineId = newId();

    QJsonObject unetInputs;
    unetInputs.insert(QStringLiteral("unet_name"), ModelConstants::videoUnetName());
    unetInputs.insert(QStringLiteral("weight_dtype"), ModelConstants::videoWeightDtype());
    insertPromptNode(prompt, unetId, ComfyRequirements::unetLoaderClassType(), unetInputs);

    QJsonObject samplingInputs;
    samplingInputs.insert(QStringLiteral("model"), latentLink(unetId));
    samplingInputs.insert(QStringLiteral("shift"), BuilderLimits::videoModelShift);
    insertPromptNode(prompt, samplingId, ComfyRequirements::modelSamplingClassType(), samplingInputs);

    QJsonObject clipInputs;
    clipInputs.insert(QStringLiteral("clip_name"), ModelConstants::videoClipName());
    clipInputs.insert(QStringLiteral("type"), ModelConstants::videoClipType());
    clipInputs.insert(QStringLiteral("device"), ModelConstants::videoClipDevice());
    insertPromptNode(prompt, clipId, ComfyRequirements::clipLoaderClassType(), clipInputs);

    QJsonObject clipVisionLoaderInputs;
    clipVisionLoaderInputs.insert(QStringLiteral("clip_name"), ModelConstants::videoClipVisionName());
    insertPromptNode(prompt, clipVisionLoaderId, ComfyRequirements::clipVisionLoaderClassType(), clipVisionLoaderInputs);

    QJsonObject vaeInputs;
    vaeInputs.insert(QStringLiteral("vae_name"), ModelConstants::videoVaeName());
    insertPromptNode(prompt, vaeId, ComfyRequirements::vaeLoaderClassType(), vaeInputs);

    QJsonObject positiveInputs;
    positiveInputs.insert(QStringLiteral("clip"), latentLink(clipId));
    positiveInputs.insert(QStringLiteral("text"), params.positivePrompt);
    insertPromptNode(prompt, positiveId, ComfyRequirements::clipTextEncodeClassType(), positiveInputs);

    QJsonObject negativeInputs;
    negativeInputs.insert(QStringLiteral("clip"), latentLink(clipId));
    negativeInputs.insert(QStringLiteral("text"), params.negativePrompt);
    insertPromptNode(prompt, negativeId, ComfyRequirements::clipTextEncodeClassType(), negativeInputs);

    QJsonObject loadImageInputs;
    loadImageInputs.insert(QStringLiteral("image"), params.videoInputFileName.trimmed());
    insertPromptNode(prompt, loadImageId, ComfyRequirements::loadImageClassType(), loadImageInputs);

    QJsonObject scaleImageInputs;
    scaleImageInputs.insert(QStringLiteral("image"), latentLink(loadImageId));
    scaleImageInputs.insert(QStringLiteral("upscale_method"), ModelConstants::videoUpscaleMethod());
    scaleImageInputs.insert(QStringLiteral("width"), params.canvasWidth);
    scaleImageInputs.insert(QStringLiteral("height"), params.canvasHeight);
    scaleImageInputs.insert(QStringLiteral("crop"), ModelConstants::videoUpscaleCrop());
    insertPromptNode(prompt, scaleImageId, ComfyRequirements::imageScaleClassType(), scaleImageInputs);

    QJsonObject clipVisionEncodeInputs;
    clipVisionEncodeInputs.insert(QStringLiteral("clip_vision"), latentLink(clipVisionLoaderId));
    clipVisionEncodeInputs.insert(QStringLiteral("image"), latentLink(scaleImageId));
    clipVisionEncodeInputs.insert(QStringLiteral("crop"), ModelConstants::videoVisionCrop());
    insertPromptNode(prompt, clipVisionEncodeId, ComfyRequirements::clipVisionEncodeClassType(), clipVisionEncodeInputs);

    QJsonObject wanInputs;
    wanInputs.insert(QStringLiteral("positive"), latentLink(positiveId));
    wanInputs.insert(QStringLiteral("negative"), latentLink(negativeId));
    wanInputs.insert(QStringLiteral("vae"), latentLink(vaeId));
    wanInputs.insert(QStringLiteral("clip_vision_output"), latentLink(clipVisionEncodeId));
    wanInputs.insert(QStringLiteral("start_image"), latentLink(scaleImageId));
    wanInputs.insert(QStringLiteral("width"), params.canvasWidth);
    wanInputs.insert(QStringLiteral("height"), params.canvasHeight);
    wanInputs.insert(QStringLiteral("length"), frameCount);
    wanInputs.insert(QStringLiteral("batch_size"), BuilderLimits::videoBatchSize);
    insertPromptNode(prompt, wanId, ComfyRequirements::wanImageToVideoClassType(), wanInputs);

    QJsonArray wanPositiveLink;
    wanPositiveLink.append(wanId);
    wanPositiveLink.append(0);
    QJsonArray wanNegativeLink;
    wanNegativeLink.append(wanId);
    wanNegativeLink.append(1);
    QJsonArray wanLatentLink;
    wanLatentLink.append(wanId);
    wanLatentLink.append(2);
    QJsonObject samplerInputs;
    samplerInputs.insert(QStringLiteral("model"), latentLink(samplingId));
    samplerInputs.insert(QStringLiteral("positive"), wanPositiveLink);
    samplerInputs.insert(QStringLiteral("negative"), wanNegativeLink);
    samplerInputs.insert(QStringLiteral("latent_image"), wanLatentLink);
    samplerInputs.insert(QStringLiteral("seed"), params.seed);
    samplerInputs.insert(QStringLiteral("steps"), params.refineSteps);
    samplerInputs.insert(QStringLiteral("cfg"), params.refineGuidance);
    samplerInputs.insert(QStringLiteral("sampler_name"), ModelConstants::videoSamplerName());
    samplerInputs.insert(QStringLiteral("scheduler"), ModelConstants::videoSchedulerName());
    samplerInputs.insert(QStringLiteral("denoise"), BuilderLimits::videoDenoise);
    insertPromptNode(prompt, samplerId, ComfyRequirements::samplerClassType(), samplerInputs);

    QJsonObject decodeInputs;
    decodeInputs.insert(QStringLiteral("samples"), latentLink(samplerId));
    decodeInputs.insert(QStringLiteral("vae"), latentLink(vaeId));
    insertPromptNode(prompt, decodeId, ComfyRequirements::vaeDecodeClassType(), decodeInputs);

    QJsonObject combineInputs;
    combineInputs.insert(QStringLiteral("images"), latentLink(decodeId));
    combineInputs.insert(QStringLiteral("frame_rate"), params.videoFrameRate);
    combineInputs.insert(QStringLiteral("loop_count"), BuilderLimits::videoLoopCount);
    combineInputs.insert(QStringLiteral("filename_prefix"),
                         savePrefixOverride.isEmpty() ? ModelConstants::videoCombinePrefix()
                                                      : savePrefixOverride);
    combineInputs.insert(QStringLiteral("format"), ModelConstants::videoCombineFormat());
    combineInputs.insert(QStringLiteral("pingpong"), false);
    combineInputs.insert(QStringLiteral("save_output"), true);
    combineInputs.insert(QStringLiteral("pix_fmt"), ModelConstants::videoCombinePixelFormat());
    combineInputs.insert(QStringLiteral("crf"), BuilderLimits::videoCombineCrf);
    combineInputs.insert(QStringLiteral("save_metadata"), true);
    combineInputs.insert(QStringLiteral("trim_to_audio"), false);
    insertPromptNode(prompt, combineId, ComfyRequirements::videoCombineClassType(), combineInputs);

    qInfo() << "ComfyWorkflowBuilder video build done: nodes=" << prompt.count()
            << "frames=" << frameCount;
    return prompt;
}
