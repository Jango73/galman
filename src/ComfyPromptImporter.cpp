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

#include "ComfyPromptImporter.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

#include "ComfyWorkflowParser.h"

namespace
{

const char promptChunkKey[] = "prompt";
const char workflowChunkKey[] = "workflow";
const int maxResolveDepth = 4;

bool nodeHasClass(const QJsonObject &node, const char *classType)
{
    return node.value(QStringLiteral("class_type")).toString() == QString::fromLatin1(classType);
}

QJsonObject nodeInputs(const QJsonObject &node)
{
    const QJsonValue inputs = node.value(QStringLiteral("inputs"));
    return inputs.isObject() ? inputs.toObject() : QJsonObject();
}

QString linkedNodeId(const QJsonObject &inputs, const char *key)
{
    const QJsonValue link = inputs.value(QString::fromLatin1(key));
    if (!link.isArray() || link.toArray().isEmpty()) {
        return QString();
    }
    return link.toArray().first().toVariant().toString();
}

QJsonObject findFirstNode(const QJsonObject &prompt, const char *classType)
{
    for (auto it = prompt.begin(); it != prompt.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        const QJsonObject node = it.value().toObject();
        if (nodeHasClass(node, classType)) {
            return node;
        }
    }
    return {};
}

QList<QJsonObject> findAllNodes(const QJsonObject &prompt, const char *classType)
{
    QList<QJsonObject> matches;
    for (auto it = prompt.begin(); it != prompt.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        const QJsonObject node = it.value().toObject();
        if (nodeHasClass(node, classType)) {
            matches.append(node);
        }
    }
    return matches;
}

/**
 * @brief Follows an input link to the value held by a value node.
 * @param prompt Prompt object in ComfyUI API format.
 * @param entry Input entry, either a literal value or a link array.
 * @param depth Current resolution depth, guards against reference cycles.
 * @return Resolved literal value, or the original entry when not resolvable.
 */
QJsonValue resolveInputValue(const QJsonObject &prompt, const QJsonValue &entry, int depth = 0)
{
    if (depth > maxResolveDepth || !entry.isArray() || entry.toArray().isEmpty()) {
        return entry;
    }
    const QString nodeId = entry.toArray().first().toVariant().toString();
    const QJsonValue nodeValue = prompt.value(nodeId);
    if (!nodeValue.isObject()) {
        return entry;
    }
    const QJsonObject targetInputs = nodeInputs(nodeValue.toObject());
    if (!targetInputs.contains(QStringLiteral("value"))) {
        return entry;
    }
    return resolveInputValue(prompt, targetInputs.value(QStringLiteral("value")), depth + 1);
}

bool readInt(const QJsonObject &prompt, const QJsonObject &inputs, const char *key, int *value)
{
    QJsonValue entry = inputs.value(QString::fromLatin1(key));
    if (entry.isArray()) {
        entry = resolveInputValue(prompt, entry);
    }
    if (entry.isUndefined() || entry.isNull() || entry.isArray() || entry.isObject()) {
        return false;
    }
    *value = entry.toInt();
    return true;
}

bool readDouble(const QJsonObject &prompt, const QJsonObject &inputs, const char *key, double *value)
{
    QJsonValue entry = inputs.value(QString::fromLatin1(key));
    if (entry.isArray()) {
        entry = resolveInputValue(prompt, entry);
    }
    if (entry.isUndefined() || entry.isNull() || entry.isArray() || entry.isObject()) {
        return false;
    }
    *value = entry.toDouble();
    return true;
}

bool readText(const QJsonObject &prompt, const QJsonObject &inputs, const char *key, QString *value)
{
    QJsonValue entry = inputs.value(QString::fromLatin1(key));
    if (entry.isArray()) {
        entry = resolveInputValue(prompt, entry);
    }
    if (!entry.isString()) {
        return false;
    }
    *value = entry.toString();
    return true;
}

bool clipTextIsEmpty(const QJsonObject &prompt, const QString &nodeId)
{
    if (nodeId.isEmpty() || !prompt.value(nodeId).isObject()) {
        return false;
    }
    const QJsonObject node = prompt.value(nodeId).toObject();
    if (!nodeHasClass(node, "CLIPTextEncode")) {
        return false;
    }
    QString text;
    if (!readText(prompt, nodeInputs(node), "text", &text)) {
        return false;
    }
    return text.trimmed().isEmpty();
}

} // namespace

/**
 * @brief Reads the embedded ComfyUI prompt from an image file.
 * @param imagePath Image file path to inspect.
 * @param error Optional output error message.
 * @return Prompt object in ComfyUI API format, or an empty object on failure.
 */
QJsonObject ComfyPromptImporter::readEmbeddedPrompt(const QString &imagePath, QString *error)
{
    if (imagePath.isEmpty() || !QFileInfo::exists(imagePath)) {
        if (error) {
            *error = QCoreApplication::translate("ComfyPromptImporter", "Image not found");
        }
        return {};
    }

    QImageReader reader(imagePath);
    if (!reader.canRead()) {
        if (error) {
            *error = QCoreApplication::translate("ComfyPromptImporter", "Cannot read image");
        }
        return {};
    }

    const QStringList keys = reader.textKeys();
    QString payload;
    if (keys.contains(QString::fromLatin1(promptChunkKey))) {
        payload = reader.text(QString::fromLatin1(promptChunkKey));
    } else if (keys.contains(QString::fromLatin1(workflowChunkKey))) {
        const QByteArray raw = reader.text(QString::fromLatin1(workflowChunkKey)).toUtf8();
        const QJsonDocument workflowDoc = QJsonDocument::fromJson(raw);
        if (!workflowDoc.isObject()) {
            if (error) {
                *error = QCoreApplication::translate("ComfyPromptImporter", "Invalid embedded workflow");
            }
            return {};
        }
        return ComfyWorkflowParser::toPrompt(workflowDoc.object());
    } else {
        if (error) {
            *error = QCoreApplication::translate("ComfyPromptImporter", "No ComfyUI workflow found in image");
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = QCoreApplication::translate("ComfyPromptImporter", "Invalid embedded prompt");
        }
        return {};
    }
    return doc.object();
}

/**
 * @brief Maps ComfyUI prompt nodes onto pilot parameters in place.
 * @param prompt Prompt object in ComfyUI API format.
 * @param parameters Parameters updated for every recognized value.
 * @param error Optional output error message.
 * @return Names of the filled controls, or an empty list on failure.
 */
QStringList ComfyPromptImporter::extractParameters(const QJsonObject &prompt,
                                                  ComfyPilotParameters *parameters,
                                                  QString *error)
{
    QStringList filled;
    if (parameters == nullptr) {
        return filled;
    }

    const QJsonObject sampler = findFirstNode(prompt, "KSampler");
    if (sampler.isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("ComfyPromptImporter", "Unsupported workflow: KSampler not found");
        }
        return filled;
    }

    const QJsonObject samplerInputs = nodeInputs(sampler);
    int intValue = 0;
    double doubleValue = 0.0;
    if (readInt(prompt, samplerInputs, "seed", &intValue)) {
        parameters->seed = intValue;
        filled.append(QStringLiteral("seed"));
    }
    if (readInt(prompt, samplerInputs, "steps", &intValue)) {
        parameters->initialSteps = intValue;
        filled.append(QStringLiteral("initialSteps"));
    }
    if (readDouble(prompt, samplerInputs, "cfg", &doubleValue)) {
        parameters->initialGuidance = doubleValue;
        filled.append(QStringLiteral("initialGuidance"));
    }
    if (readDouble(prompt, samplerInputs, "denoise", &doubleValue)) {
        parameters->initialDenoise = doubleValue;
        filled.append(QStringLiteral("initialDenoise"));
    }

    const QString positiveId = linkedNodeId(samplerInputs, "positive");
    const QString negativeId = linkedNodeId(samplerInputs, "negative");
    if (!positiveId.isEmpty() && prompt.value(positiveId).isObject()) {
        const QJsonObject positiveInputs = nodeInputs(prompt.value(positiveId).toObject());
        QString text;
        if (readText(prompt, positiveInputs, "text", &text)) {
            parameters->positivePrompt = text;
            filled.append(QStringLiteral("positivePrompt"));
        }
    }
    if (!negativeId.isEmpty() && prompt.value(negativeId).isObject()) {
        const QJsonObject negativeInputs = nodeInputs(prompt.value(negativeId).toObject());
        QString text;
        if (readText(prompt, negativeInputs, "text", &text)) {
            parameters->negativePrompt = text;
            filled.append(QStringLiteral("negativePrompt"));
        }
    }

    const QString latentId = linkedNodeId(samplerInputs, "latent_image");
    if (!latentId.isEmpty() && prompt.value(latentId).isObject()) {
        const QJsonObject latentNode = prompt.value(latentId).toObject();
        if (nodeHasClass(latentNode, "EmptyLatentImage")) {
            const QJsonObject latentInputs = nodeInputs(latentNode);
            if (readInt(prompt, latentInputs, "width", &intValue)) {
                parameters->canvasWidth = intValue;
                filled.append(QStringLiteral("canvasWidth"));
            }
            if (readInt(prompt, latentInputs, "height", &intValue)) {
                parameters->canvasHeight = intValue;
                filled.append(QStringLiteral("canvasHeight"));
            }
        }
    }
    if (!filled.contains(QStringLiteral("canvasWidth"))) {
        const QJsonObject latent = findFirstNode(prompt, "EmptyLatentImage");
        if (!latent.isEmpty()) {
            const QJsonObject latentInputs = nodeInputs(latent);
            if (readInt(prompt, latentInputs, "width", &intValue)) {
                parameters->canvasWidth = intValue;
                filled.append(QStringLiteral("canvasWidth"));
            }
            if (readInt(prompt, latentInputs, "height", &intValue)) {
                parameters->canvasHeight = intValue;
                filled.append(QStringLiteral("canvasHeight"));
            }
        }
    }

    const QList<QJsonObject> tiledSamplers = findAllNodes(prompt, "BNK_TiledKSampler");
    parameters->refineCount = tiledSamplers.size();
    filled.append(QStringLiteral("refineCount"));

    QJsonObject refineSource;
    if (!tiledSamplers.isEmpty()) {
        refineSource = nodeInputs(tiledSamplers.first());
    } else {
        const QJsonObject faceNode = findFirstNode(prompt, "DZ_Face_Detailer");
        if (!faceNode.isEmpty()) {
            refineSource = nodeInputs(faceNode);
        }
    }
    if (!refineSource.isEmpty()) {
        if (readInt(prompt, refineSource, "steps", &intValue)) {
            parameters->refineSteps = intValue;
            filled.append(QStringLiteral("refineSteps"));
        }
        if (readDouble(prompt, refineSource, "cfg", &doubleValue)) {
            parameters->refineGuidance = doubleValue;
            filled.append(QStringLiteral("refineGuidance"));
        }
        if (readDouble(prompt, refineSource, "denoise", &doubleValue)) {
            parameters->refineDenoise = doubleValue;
            filled.append(QStringLiteral("refineDenoise"));
        }
    }

    parameters->faceDetail = !findFirstNode(prompt, "DZ_Face_Detailer").isEmpty();
    filled.append(QStringLiteral("faceDetail"));

    bool emptyRefine = false;
    if (!tiledSamplers.isEmpty()) {
        const QJsonObject tiledInputs = nodeInputs(tiledSamplers.first());
        const QString tiledPositiveId = linkedNodeId(tiledInputs, "positive");
        const QString tiledNegativeId = linkedNodeId(tiledInputs, "negative");
        if (!tiledPositiveId.isEmpty() && !tiledNegativeId.isEmpty()
            && (tiledPositiveId != positiveId || tiledNegativeId != negativeId)) {
            emptyRefine = clipTextIsEmpty(prompt, tiledPositiveId)
                && clipTextIsEmpty(prompt, tiledNegativeId);
        }
    }
    if (!emptyRefine) {
        const QJsonObject faceNode = findFirstNode(prompt, "DZ_Face_Detailer");
        if (!faceNode.isEmpty()) {
            const QJsonObject faceInputs = nodeInputs(faceNode);
            const QString facePositiveId = linkedNodeId(faceInputs, "positive");
            const QString faceNegativeId = linkedNodeId(faceInputs, "negative");
            if (!facePositiveId.isEmpty() && !faceNegativeId.isEmpty()
                && (facePositiveId != positiveId || faceNegativeId != negativeId)) {
                emptyRefine = clipTextIsEmpty(prompt, facePositiveId)
                    && clipTextIsEmpty(prompt, faceNegativeId);
            }
        }
    }
    parameters->emptyRefinePrompt = emptyRefine;
    filled.append(QStringLiteral("emptyRefinePrompt"));

    qInfo() << "ComfyPromptImporter extracted controls:" << filled;
    return filled;
}
