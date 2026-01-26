
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

#include "ScriptEngine.h"

#include "ComfyClient.h"
#include "ComfyWorkflowParser.h"
#include "PlatformUtils.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QImageReader>
#include <QImageWriter>
#include <QJsonObject>
#include <QMetaType>
#include <QSettings>
#include <QVariantMap>

/**
 * @brief Constructs the script engine and exposes it to the JS runtime.
 * @param parent Parent QObject for ownership.
 */
ScriptEngine::ScriptEngine(QObject *parent)
    : QObject(parent)
{
    QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
    m_engine.globalObject().setProperty("scriptEngine", m_engine.newQObject(this));
    updateEngineSelection();
}

/**
 * @brief Returns the current selection exposed to scripts.
 * @return List of selection entries.
 */
QVariantList ScriptEngine::selection() const
{
    return m_selection;
}

/**
 * @brief Builds the script selection metadata from a list of paths.
 * @param paths File or folder paths to include in the selection.
 */
void ScriptEngine::setSelection(const QStringList &paths)
{
    QVariantList nextSelection;
    nextSelection.reserve(paths.size());

    for (const QString &path : paths) {
        QFileInfo info(path);
        QVariantMap entry;
        entry.insert("path", info.absoluteFilePath());
        entry.insert("name", info.fileName());
        entry.insert("isDir", info.isDir());
        entry.insert("modifiedMs", info.lastModified().toMSecsSinceEpoch());

        static const QSet<QString> kFormats = []() {
            QSet<QString> formats;
            const auto supported = QImageReader::supportedImageFormats();
            for (const QByteArray &fmt : supported) {
                formats.insert(QString::fromLatin1(fmt).toLower());
            }
            return formats;
        }();

        QImageReader reader(path);
        const QSize size = reader.size();
        entry.insert("width", size.isValid() ? size.width() : 0);
        entry.insert("height", size.isValid() ? size.height() : 0);
        entry.insert("isImage", kFormats.contains(info.suffix().toLower()));

        nextSelection.append(entry);
    }

    if (m_selection == nextSelection) {
        return;
    }

    m_selection = nextSelection;
    updateEngineSelection();
    emit selectionChanged();
}

/**
 * @brief Evaluates a script string in the JS engine.
 * @param script JavaScript source string to evaluate.
 * @return Script result or an error string.
 */
QVariant ScriptEngine::run(const QString &script)
{
    QJSValue result = m_engine.evaluate(script);
    if (result.isError()) {
        return tr("Error: %1").arg(result.toString());
    }
    return result.toVariant();
}

/**
 * @brief Loads and evaluates a script file in the JS engine.
 * @param path File path to the script.
 * @return Script result or an error string.
 */
QVariant ScriptEngine::runFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return tr("Error: cannot open script %1").arg(path);
    }

    const QString source = QString::fromUtf8(file.readAll());
    QJSValue result = m_engine.evaluate(source, path);
    if (result.isError()) {
        return tr("Error: %1").arg(result.toString());
    }
    return result.toVariant();
}

/**
 * @brief Renames a file or folder on disk.
 * @param from Source path to rename.
 * @param to Target path to rename to.
 * @return Result map including ok and error fields.
 */
QVariantMap ScriptEngine::renameFile(const QString &from, const QString &to)
{
    QVariantMap result;
    result.insert("ok", false);

    if (from == to) {
        result.insert("ok", true);
        return result;
    }

    QFileInfo sourceInfo(from);
    if (!sourceInfo.exists()) {
        result.insert("error", tr("Source not found"));
        return result;
    }

    QFileInfo targetInfo(to);
    QDir targetDir = targetInfo.dir();
    if (!targetDir.exists()) {
        result.insert("error", tr("Target folder not found"));
        return result;
    }

    if (targetInfo.exists()) {
        result.insert("error", tr("Target already exists"));
        return result;
    }

    QFile file(from);
    if (!file.rename(to)) {
        result.insert("error", tr("Rename failed"));
        return result;
    }

    result.insert("ok", true);
    return result;
}

/**
 * @brief Moves a path to trash or deletes it when unsupported.
 * @param path File or folder path to remove.
 * @return Result map including ok and error fields.
 */
QVariantMap ScriptEngine::moveToTrash(const QString &path)
{
    QVariantMap result;
    result.insert("ok", false);

    if (path.isEmpty()) {
        result.insert("error", tr("Source not found"));
        return result;
    }

    QFileInfo sourceInfo(path);
    if (!sourceInfo.exists()) {
        result.insert("error", tr("Source not found"));
        return result;
    }

    QString error;
    if (!PlatformUtils::moveToTrashOrDelete(path, &error)) {
        result.insert("error", error.isEmpty() ? tr("Failed to move source to trash") : error);
        return result;
    }

    result.insert("ok", true);
    return result;
}

/**
 * @brief Loads a script file and returns its scriptDefinition metadata.
 * @param path File path to the script.
 * @return Result map including ok and error fields plus metadata.
 */
QVariantMap ScriptEngine::loadScript(const QString &path)
{
    QVariantMap response;
    response.insert("ok", false);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        response.insert("error", tr("Cannot open script %1").arg(path));
        return response;
    }

    const QString source = QString::fromUtf8(file.readAll());
    QJSValue result = m_engine.evaluate(source, path);
    if (result.isError()) {
        response.insert("error", result.toString());
        return response;
    }

    QJSValue defFn = m_engine.globalObject().property("scriptDefinition");
    if (!defFn.isCallable()) {
        response.insert("error", tr("scriptDefinition() is not defined"));
        return response;
    }

    QJSValue def = defFn.call();
    if (def.isError()) {
        response.insert("error", def.toString());
        return response;
    }

    response = def.toVariant().toMap();
    response.insert("ok", true);
    return response;
}

/**
 * @brief Loads and runs a script with parameters and current selection.
 * @param path File path to the script.
 * @param params Parameter map passed to the script.
 * @return Script output or an error string.
 */
QVariant ScriptEngine::runScript(const QString &path, const QVariantMap &params)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return tr("Error: cannot open script %1").arg(path);
    }

    const QString source = QString::fromUtf8(file.readAll());
    QJSValue result = m_engine.evaluate(source, path);
    if (result.isError()) {
        return tr("Error: %1").arg(result.toString());
    }

    QJSValue defFn = m_engine.globalObject().property("scriptDefinition");
    if (!defFn.isCallable()) {
        return tr("Error: scriptDefinition() is not defined");
    }

    QJSValue def = defFn.call();
    if (def.isError()) {
        return tr("Error: %1").arg(def.toString());
    }

    QJSValue runFn = def.property("run");
    if (!runFn.isCallable()) {
        return tr("Error: run() is not defined");
    }

    QJSValue paramsValue = m_engine.toScriptValue(params);
    QJSValue selectionValue = m_engine.globalObject().property("selection");
    QJSValueList args;
    args << paramsValue << selectionValue;

    QJSValue output = runFn.callWithInstance(def, args);
    if (output.isError()) {
        return tr("Error: %1").arg(output.toString());
    }

    return output.toVariant();
}

/**
 * @brief Returns the list of supported image formats, lowercased and sorted.
 * @return List of format strings (example: jpg, png).
 */
QStringList ScriptEngine::supportedImageFormats() const
{
    QStringList formats;
    const QList<QByteArray> supported = QImageReader::supportedImageFormats();
    formats.reserve(supported.size());
    for (const QByteArray &format : supported) {
        formats.append(QString::fromLatin1(format).toLower());
    }
    formats.removeDuplicates();
    formats.sort();
    return formats;
}

/**
 * @brief Saves an adjusted image over the original, moving the original to trash first.
 * @param image Adjusted image to save.
 * @param path Target file path to overwrite.
 * @return Result map including ok and error fields.
 */
QVariantMap ScriptEngine::saveAdjustedImage(const QImage &image, const QString &path)
{
    QVariantMap result;
    result.insert("ok", false);

    if (path.isEmpty()) {
        result.insert("error", tr("Target path is empty"));
        return result;
    }
    if (image.isNull()) {
        result.insert("error", tr("Adjusted image is empty"));
        return result;
    }

    QString error;
    if (!PlatformUtils::moveToTrashOrDelete(path, &error)) {
        result.insert("error", error.isEmpty() ? tr("Failed to move source to trash") : error);
        return result;
    }

    QImageWriter writer(path);
    if (!writer.write(image)) {
        result.insert("error", writer.errorString().isEmpty()
                          ? tr("Failed to write image")
                          : writer.errorString());
        return result;
    }

    result.insert("ok", true);
    return result;
}

/**
 * @brief Converts an image to a different format and replaces the original.
 * @param path Source image file path.
 * @param format Target image format extension.
 * @return Result map including ok, error, and target fields.
 */
QVariantMap ScriptEngine::convertImage(const QString &path, const QString &format)
{
    QVariantMap result;
    result.insert("ok", false);

    QFileInfo sourceInfo(path);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        result.insert("error", tr("Source not found"));
        return result;
    }

    QImageReader reader(path);
    QImage image = reader.read();
    if (image.isNull()) {
        result.insert("error", tr("Failed to read image"));
        return result;
    }

    const QString ext = format.toLower();
    const QString baseName = sourceInfo.completeBaseName();
    const QString dir = sourceInfo.absolutePath();
    const QString targetPath = QDir(dir).filePath(baseName + "." + ext);

    if (QFileInfo::exists(targetPath)) {
        result.insert("error", tr("Target already exists"));
        return result;
    }

    QImageWriter writer(targetPath, ext.toLatin1());
    if (!writer.write(image)) {
        result.insert("error", tr("Failed to write image"));
        return result;
    }

    QString error;
    if (!PlatformUtils::moveToTrashOrDelete(path, &error)) {
        QFile::remove(targetPath);
        result.insert("error", error.isEmpty() ? tr("Failed to move source to trash") : error);
        return result;
    }

    result.insert("ok", true);
    result.insert("target", targetPath);
    return result;
}

/**
 * @brief Runs a ComfyUI workflow for an image and returns the output path.
 * @param workflowPath Path to the workflow JSON file.
 * @param imagePath Input image path.
 * @param serverUrl ComfyUI server base URL.
 * @param outputDir Preferred output folder for downloaded images.
 * @return Result map including ok, error, and output fields.
 */
QVariantMap ScriptEngine::runComfyWorkflow(const QString &workflowPath,
                                           const QString &imagePath,
                                           const QString &serverUrl,
                                           const QString &outputDir)
{
    QVariantMap result;
    result.insert("ok", false);

    if (imagePath.isEmpty() || !QFileInfo::exists(imagePath)) {
        result.insert("error", tr("Image not found"));
        return result;
    }

    QString resolvedServer = serverUrl;
    if (resolvedServer.isEmpty()) {
        resolvedServer = "http://127.0.0.1:8188";
    }

    QString error;
    QJsonObject workflow = ComfyWorkflowParser::load(workflowPath, &error);
    if (!error.isEmpty()) {
        result.insert("error", error);
        return result;
    }

    if (!ComfyWorkflowParser::injectLoadImage(workflow, imagePath, &error)) {
        result.insert("error", error);
        return result;
    }

    QJsonObject prompt = ComfyWorkflowParser::toPrompt(workflow);
    QJsonObject payload;
    payload.insert("prompt", prompt);

    ComfyClient client;
    QVariantMap comfyResult = client.runWorkflow(
        resolvedServer,
        payload,
        "galman",
        30000,
        1000,
        180000
    );

    if (!comfyResult.value("ok").toBool()) {
        result.insert("error", comfyResult.value("error"));
        return result;
    }

    const QVariantMap data = comfyResult.value("data").toMap();
    const QVariant outputsValue = data.value("outputs");
    QVariantList outputEntries;
    if (outputsValue.typeId() == QMetaType::QVariantList) {
        outputEntries = outputsValue.toList();
    } else if (outputsValue.typeId() == QMetaType::QVariantMap) {
        outputEntries = outputsValue.toMap().values();
    }

    QString resolvedOutputDir = outputDir;
    if (resolvedOutputDir.isEmpty()) {
        resolvedOutputDir = PlatformUtils::comfyDefaultOutputDir();
    }
    const QVariantMap meta = data.value("meta").toMap();
    const QVariantMap paths = meta.value("paths").toMap();
    const QString serverOutputDir = paths.value("output").toString();
    QString lastDownloadError;
    for (const QVariant &entryVar : outputEntries) {
        const QVariantMap entry = entryVar.toMap();
        QVariant imagesVar = entry.value("images");
        QVariantList images;
        if (imagesVar.typeId() == QMetaType::QVariantList) {
            images = imagesVar.toList();
        } else if (imagesVar.typeId() == QMetaType::QVariantMap) {
            images = imagesVar.toMap().values();
        }
        if (images.isEmpty()) {
            continue;
        }
        const QVariantMap imgMeta = images.first().toMap();
        const QString filename = imgMeta.value("filename").toString();
        const QString subfolder = imgMeta.value("subfolder").toString();
        const QString imgType = imgMeta.value("type").toString();
        if (QFileInfo(filename).isAbsolute() && QFileInfo::exists(filename)) {
            result.insert("ok", true);
            result.insert("output", filename);
            return result;
        }

        if (!filename.isEmpty()) {
            QString relativePath = filename;
            if (!subfolder.isEmpty()) {
                relativePath = QDir(subfolder).filePath(filename);
            }
            QString candidate;
            if (!resolvedOutputDir.isEmpty()) {
                candidate = QDir(resolvedOutputDir).filePath(relativePath);
                if (!QFileInfo::exists(candidate)) {
                    candidate.clear();
                }
            }
            if (candidate.isEmpty() && !serverOutputDir.isEmpty() && serverOutputDir != resolvedOutputDir) {
                const QString serverCandidate = QDir(serverOutputDir).filePath(relativePath);
                if (QFileInfo::exists(serverCandidate)) {
                    candidate = serverCandidate;
                }
            }
            if (!candidate.isEmpty()) {
                result.insert("ok", true);
                result.insert("output", candidate);
                return result;
            }

            const QString targetPath = QDir(resolvedOutputDir).filePath(relativePath);
            QString downloadError;
            if (client.downloadImage(resolvedServer, filename, subfolder, imgType, targetPath, 30000, &downloadError)) {
                result.insert("ok", true);
                result.insert("output", targetPath);
                return result;
            }
            if (!downloadError.isEmpty()) {
                lastDownloadError = downloadError;
            }
        }
    }

    if (!lastDownloadError.isEmpty()) {
        result.insert("error", lastDownloadError);
    } else {
        result.insert("error", tr("No output image found"));
    }
    return result;
}

/**
 * @brief Returns the default ComfyUI output folder for the platform.
 * @return Default output folder path.
 */
QString ScriptEngine::comfyDefaultOutputDir() const
{
    return PlatformUtils::comfyDefaultOutputDir();
}

/**
 * @brief Loads persisted parameters for a script.
 * @param scriptPath Script file path.
 * @return Map of stored parameter values.
 */
QVariantMap ScriptEngine::loadScriptParams(const QString &scriptPath)
{
    QVariantMap values;
    if (scriptPath.isEmpty()) {
        return values;
    }

    const QFileInfo info(scriptPath);
    const QString scriptKey = info.completeBaseName();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Galman", "Galman");
    settings.beginGroup("scripts");
    settings.beginGroup(scriptKey);

    const QStringList keys = settings.childKeys();
    for (const QString &key : keys) {
        values.insert(key, settings.value(key));
    }

    settings.endGroup();
    settings.endGroup();
    return values;
}

/**
 * @brief Saves parameters for a script into settings storage.
 * @param scriptPath Script file path.
 * @param params Map of parameter values to persist.
 */
void ScriptEngine::saveScriptParams(const QString &scriptPath, const QVariantMap &params)
{
    if (scriptPath.isEmpty()) {
        return;
    }
    const QFileInfo info(scriptPath);
    const QString scriptKey = info.completeBaseName();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Galman", "Galman");
    settings.beginGroup("scripts");
    settings.beginGroup(scriptKey);

    settings.remove("");
    for (auto it = params.begin(); it != params.end(); ++it) {
        settings.setValue(it.key(), it.value());
    }

    settings.endGroup();
    settings.endGroup();
    settings.sync();
}

/**
 * @brief Updates the JS engine selection array from the cached selection.
 */
void ScriptEngine::updateEngineSelection()
{
    QJSValue array = m_engine.newArray(m_selection.size());
    for (int i = 0; i < m_selection.size(); ++i) {
        array.setProperty(i, m_engine.toScriptValue(m_selection.at(i)));
    }
    m_engine.globalObject().setProperty("selection", array);
}
