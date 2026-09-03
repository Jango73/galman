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
    return QStringLiteral("realisticVisionV60B1_v51HyperVAE.safetensors");
}

QString ComfyWorkflowBuilder::ModelConstants::upscaleModelName()
{
    return QStringLiteral("RealESRGAN_x2plus.pth");
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
    return QStringLiteral("Galman");
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
    insertPromptNode(prompt, checkpointId, QStringLiteral("CheckpointLoaderSimple"), checkpointInputs);

    QJsonObject upscaleLoaderInputs;
    upscaleLoaderInputs.insert(QStringLiteral("model_name"), ModelConstants::upscaleModelName());
    insertPromptNode(prompt, upscaleLoaderId, QStringLiteral("UpscaleModelLoader"), upscaleLoaderInputs);

    QJsonObject positiveInputs;
    QJsonArray positiveClip;
    positiveClip.append(checkpointId);
    positiveClip.append(1);
    positiveInputs.insert(QStringLiteral("clip"), positiveClip);
    positiveInputs.insert(QStringLiteral("text"), params.positivePrompt);
    insertPromptNode(prompt, positiveId, QStringLiteral("CLIPTextEncode"), positiveInputs);

    QJsonObject negativeInputs;
    QJsonArray negativeClip;
    negativeClip.append(checkpointId);
    negativeClip.append(1);
    negativeInputs.insert(QStringLiteral("clip"), negativeClip);
    negativeInputs.insert(QStringLiteral("text"), params.negativePrompt);
    insertPromptNode(prompt, negativeId, QStringLiteral("CLIPTextEncode"), negativeInputs);

    QString refinePositiveId = positiveId;
    QString refineNegativeId = negativeId;
    if (params.emptyRefinePrompt && params.refineCount > 0) {
        refinePositiveId = newId();
        refineNegativeId = newId();
        QJsonObject emptyPositiveInputs;
        emptyPositiveInputs.insert(QStringLiteral("clip"), positiveClip);
        emptyPositiveInputs.insert(QStringLiteral("text"), QString());
        insertPromptNode(prompt, refinePositiveId, QStringLiteral("CLIPTextEncode"), emptyPositiveInputs);
        QJsonObject emptyNegativeInputs;
        emptyNegativeInputs.insert(QStringLiteral("clip"), negativeClip);
        emptyNegativeInputs.insert(QStringLiteral("text"), QString());
        insertPromptNode(prompt, refineNegativeId, QStringLiteral("CLIPTextEncode"), emptyNegativeInputs);
    }

    QJsonObject latentInputs;
    latentInputs.insert(QStringLiteral("width"), params.canvasWidth);
    latentInputs.insert(QStringLiteral("height"), params.canvasHeight);
    latentInputs.insert(QStringLiteral("batch_size"), BuilderLimits::latentBatchSize);
    insertPromptNode(prompt, latentId, QStringLiteral("EmptyLatentImage"), latentInputs);

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
    insertPromptNode(prompt, samplerId, QStringLiteral("KSampler"), samplerInputs);

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
        insertPromptNode(prompt, decodeId, QStringLiteral("VAEDecode"), decodeInputs);

        QJsonObject upscaleInputs;
        QJsonArray upscaleModelLink;
        upscaleModelLink.append(upscaleLoaderId);
        upscaleModelLink.append(0);
        upscaleInputs.insert(QStringLiteral("upscale_model"), upscaleModelLink);
        upscaleInputs.insert(QStringLiteral("image"), latentLink(decodeId));
        insertPromptNode(prompt, upscaleId, QStringLiteral("ImageUpscaleWithModel"), upscaleInputs);

        QJsonObject encodeInputs;
        encodeInputs.insert(QStringLiteral("pixels"), latentLink(upscaleId));
        QJsonArray encodeVae;
        encodeVae.append(checkpointId);
        encodeVae.append(2);
        encodeInputs.insert(QStringLiteral("vae"), encodeVae);
        insertPromptNode(prompt, encodeId, QStringLiteral("VAEEncode"), encodeInputs);

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
        insertPromptNode(prompt, tiledId, QStringLiteral("BNK_TiledKSampler"), tiledInputs);

        currentLatentId = tiledId;
    }

    if (params.faceDetail) {
        const QString faceId = newId();
        QJsonObject faceInputs;
        faceInputs.insert(QStringLiteral("model"), modelLink);
        faceInputs.insert(QStringLiteral("positive"), latentLink(positiveId));
        faceInputs.insert(QStringLiteral("negative"), latentLink(negativeId));
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
        insertPromptNode(prompt, faceId, QStringLiteral("DZ_Face_Detailer"), faceInputs);
        currentLatentId = faceId;
    }

    const QString finalDecodeId = newId();
    QJsonObject finalDecodeInputs;
    finalDecodeInputs.insert(QStringLiteral("samples"), latentLink(currentLatentId));
    QJsonArray finalVae;
    finalVae.append(checkpointId);
    finalVae.append(2);
    finalDecodeInputs.insert(QStringLiteral("vae"), finalVae);
    insertPromptNode(prompt, finalDecodeId, QStringLiteral("VAEDecode"), finalDecodeInputs);

    const QString saveId = newId();
    QJsonObject saveInputs;
    saveInputs.insert(QStringLiteral("images"), latentLink(finalDecodeId));
    saveInputs.insert(QStringLiteral("filename_prefix"),
                      savePrefixOverride.isEmpty() ? ModelConstants::savePrefix() : savePrefixOverride);
    insertPromptNode(prompt, saveId, QStringLiteral("SaveImage"), saveInputs);

    qInfo() << "ComfyWorkflowBuilder build done: nodes=" << prompt.count()
            << "refines=" << params.refineCount << "face=" << params.faceDetail;
    return prompt;
}
