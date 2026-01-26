
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

#include "ComfyWorkflowParser.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMap>

/**
 * @brief Loads a workflow JSON file from disk and normalizes its nodes structure.
 * @param path File system path to the workflow JSON file.
 * @param error Optional output error message.
 * @return Parsed workflow JSON object, or an empty object on failure.
 */
QJsonObject ComfyWorkflowParser::load(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowParser", "Cannot open workflow: %1").arg(path);
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowParser", "Invalid workflow JSON: %1")
                .arg(parseError.errorString());
        }
        return {};
    }

    QJsonObject workflow = doc.object();
    normalizeNodes(workflow);
    return workflow;
}

/**
 * @brief Injects the image path into the workflow Load Image node inputs.
 * @param workflow Workflow JSON object to modify in place.
 * @param imagePath Image file path to inject.
 * @param error Optional output error message.
 * @return True if the Load Image node was found and updated, false otherwise.
 */
bool ComfyWorkflowParser::injectLoadImage(QJsonObject &workflow, const QString &imagePath, QString *error)
{
    normalizeNodes(workflow);
    QJsonObject node = findLoadImageNode(workflow);
    if (node.isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("ComfyWorkflowParser", "Load Image node not found");
        }
        return false;
    }

    QJsonValue inputsVal = node.value("inputs");
    if (inputsVal.isObject()) {
        QJsonObject inputs = inputsVal.toObject();
        inputs.insert("image", imagePath);
        node.insert("inputs", inputs);
    } else if (inputsVal.isArray()) {
        QJsonArray inputs = inputsVal.toArray();
        for (int i = 0; i < inputs.size(); ++i) {
            if (!inputs[i].isObject()) {
                continue;
            }
            QJsonObject entry = inputs[i].toObject();
            if (entry.value("name").toString() == "image") {
                entry.insert("value", imagePath);
                inputs[i] = entry;
                break;
            }
        }
        node.insert("inputs", inputs);
        QJsonArray widgets = node.value("widgets_values").toArray();
        if (!widgets.isEmpty()) {
            widgets[0] = imagePath;
            node.insert("widgets_values", widgets);
        }
    } else {
        QJsonArray widgets = node.value("widgets_values").toArray();
        if (!widgets.isEmpty()) {
            widgets[0] = imagePath;
            node.insert("widgets_values", widgets);
        }
    }

    QJsonObject nodes = workflow.value("nodes").toObject();
    QString nodeId = node.value("id").toVariant().toString();
    if (nodeId.isEmpty()) {
        nodeId = "Load Image";
    }
    nodes.insert(nodeId, node);
    workflow.insert("nodes", nodes);
    return true;
}

/**
 * @brief Converts a workflow JSON object into a ComfyUI prompt payload.
 * @param workflow Workflow JSON object to convert.
 * @return Prompt JSON object ready for submission.
 */
QJsonObject ComfyWorkflowParser::toPrompt(const QJsonObject &workflow)
{
    QJsonObject wf = workflow;
    normalizeNodes(wf);

    QJsonObject nodesObj = wf.value("nodes").toObject();
    QJsonArray linksArray = wf.value("links").toArray();
    QMap<QString, QJsonObject> linkMap;

    for (const QJsonValue &entryVal : linksArray) {
        if (!entryVal.isArray()) {
            continue;
        }
        QJsonArray entry = entryVal.toArray();
        if (entry.size() < 5) {
            continue;
        }
        const QString linkId = entry.at(0).toVariant().toString();
        QJsonObject link;
        link.insert("from_node", entry.at(1).toVariant().toString());
        link.insert("from_slot", entry.at(2));
        link.insert("to_node", entry.at(3).toVariant().toString());
        link.insert("to_slot", entry.at(4));
        linkMap.insert(linkId, link);
    }

    QJsonObject prompt;
    for (auto it = nodesObj.begin(); it != nodesObj.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        QJsonObject node = it.value().toObject();
        QString nodeId = node.value("id").toVariant().toString();
        if (nodeId.isEmpty()) {
            nodeId = it.key();
        }
        const QString classType = node.value("type").toString().isEmpty()
            ? node.value("class_type").toString()
            : node.value("type").toString();

        QJsonObject inputsObj;
        QJsonArray widgets = node.value("widgets_values").toArray();
        QJsonObject widgetMap = buildWidgetMap(classType, widgets);
        int widgetIdx = 0;

        QJsonValue inputsVal = node.value("inputs");
        if (inputsVal.isArray()) {
            QJsonArray inputs = inputsVal.toArray();
            for (const QJsonValue &inputVal : inputs) {
                if (!inputVal.isObject()) {
                    continue;
                }
                QJsonObject input = inputVal.toObject();
                const QString name = input.value("name").toString();
                const QString linkId = input.value("link").toVariant().toString();
                const bool hasWidget = input.value("widget").isObject();
                const bool linked = !linkId.isEmpty() && linkMap.contains(linkId);

                if (linked) {
                    const QJsonObject link = linkMap.value(linkId);
                    QJsonArray linkValue;
                    linkValue.append(link.value("from_node"));
                    linkValue.append(link.value("from_slot"));
                    inputsObj.insert(name, linkValue);
                } else if (!name.isEmpty() && widgetMap.contains(name)) {
                    inputsObj.insert(name, widgetMap.value(name));
                } else if (hasWidget && widgetIdx < widgets.size()) {
                    inputsObj.insert(name, widgets.at(widgetIdx));
                } else if (input.contains("value")) {
                    inputsObj.insert(name, input.value("value"));
                }

                if (hasWidget) {
                    widgetIdx += 1;
                }
            }
        } else if (inputsVal.isObject()) {
            inputsObj = inputsVal.toObject();
        }

        QJsonObject promptNode;
        promptNode.insert("class_type", classType);
        promptNode.insert("inputs", inputsObj);
        prompt.insert(nodeId, promptNode);
    }

    return prompt;
}

/**
 * @brief Normalizes workflow nodes so they are stored as an object keyed by node id.
 * @param workflow Workflow JSON object to normalize in place.
 */
void ComfyWorkflowParser::normalizeNodes(QJsonObject &workflow)
{
    const QJsonValue nodesVal = workflow.value("nodes");
    if (nodesVal.isObject()) {
        return;
    }
    if (!nodesVal.isArray()) {
        return;
    }

    QJsonObject normalized;
    QJsonArray nodes = nodesVal.toArray();
    for (int i = 0; i < nodes.size(); ++i) {
        if (!nodes[i].isObject()) {
            continue;
        }
        QJsonObject node = nodes[i].toObject();
        const QString key = node.value("id").toVariant().toString();
        normalized.insert(key.isEmpty() ? QString::number(i) : key, node);
    }
    workflow.insert("nodes", normalized);
}

/**
 * @brief Finds the Load Image node in the workflow by id, name, or input signature.
 * @param workflow Workflow JSON object to search.
 * @return The Load Image node as a JSON object, or an empty object if not found.
 */
QJsonObject ComfyWorkflowParser::findLoadImageNode(QJsonObject &workflow)
{
    QJsonObject nodes = workflow.value("nodes").toObject();
    if (nodes.isEmpty()) {
        return {};
    }

    if (nodes.contains("Load Image")) {
        QJsonValue nodeVal = nodes.value("Load Image");
        if (nodeVal.isObject()) {
            return nodeVal.toObject();
        }
    }

    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        QJsonObject node = it.value().toObject();
        const QString name = node.value("name").toString();
        if (name == "Load Image") {
            return node;
        }
    }

    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        QJsonObject node = it.value().toObject();
        const QString type = node.value("class_type").toString().isEmpty()
            ? node.value("type").toString()
            : node.value("class_type").toString();
        const QString lowerType = type.toLower();
        if (lowerType.contains("loadimage") || lowerType.contains("load image")) {
            return node;
        }
        QJsonValue inputsVal = node.value("inputs");
        if (inputsVal.isObject() && inputsVal.toObject().contains("image")) {
            return node;
        }
        if (inputsVal.isArray()) {
            for (const QJsonValue &entryVal : inputsVal.toArray()) {
                if (!entryVal.isObject()) {
                    continue;
                }
                if (entryVal.toObject().value("name").toString() == "image") {
                    return node;
                }
            }
        }
    }
    return {};
}

/**
 * @brief Builds a widget value map based on class type and widget ordering.
 * @param classType ComfyUI class type identifier.
 * @param widgets Widget values array from the workflow node.
 * @return Mapping from input names to widget values, or an empty object if unsupported.
 */
QJsonObject ComfyWorkflowParser::buildWidgetMap(const QString &classType, const QJsonArray &widgets)
{
    if (classType.isEmpty() || widgets.isEmpty()) {
        return {};
    }

    const QMap<QString, QStringList> sequences = {
        {"KSampler", {"seed", "seed_mode", "steps", "cfg", "sampler_name", "scheduler", "denoise"}},
        {"BNK_TiledKSampler", {"seed", "seed_mode", "tile_width", "tile_height", "tiling_strategy", "steps", "cfg", "sampler_name", "scheduler", "denoise"}},
        {"DZ_Face_Detailer", {"seed", "seed_mode", "steps", "cfg", "sampler_name", "scheduler", "denoise", "mask_blur", "mask_type", "mask_control", "dilate_mask_value", "erode_mask_value"}},
        {"ImageScale", {"upscale_method", "width", "height", "crop"}},
        {"ImageScaleBy", {"upscale_method", "scale_by"}},
        {"PrimitiveInt", {"value"}},
        {"PrimitiveFloat", {"value"}},
        {"PrimitiveBoolean", {"value"}},
        {"CheckpointLoaderSimple", {"ckpt_name"}},
        {"UpscaleModelLoader", {"model_name"}},
        {"LoadImage", {"image", "upload"}},
        {"SaveImage", {"filename_prefix"}},
        {"ImpactCompare", {"cmp"}},
        {"ImpactLogicalOperators", {"operator"}},
        {"EmptyLatentImage", {"width", "height", "batch_size"}},
        {"CLIPTextEncode", {"text"}},
    };

    if (!sequences.contains(classType)) {
        return {};
    }

    QJsonObject mapping;
    const QStringList keys = sequences.value(classType);
    for (int i = 0; i < keys.size() && i < widgets.size(); ++i) {
        mapping.insert(keys[i], widgets.at(i));
    }
    return mapping;
}
