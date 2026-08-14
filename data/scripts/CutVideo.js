function normalizePath(path) {
    return String(path || "").replace(/\\/g, "/")
}

function folderPathFromFilePath(path) {
    const normalizedPath = normalizePath(path)
    const separatorIndex = normalizedPath.lastIndexOf("/")
    if (separatorIndex < 0) {
        return ""
    }
    return normalizedPath.slice(0, separatorIndex)
}

function fileNameFromPath(path) {
    const normalizedPath = normalizePath(path)
    const separatorIndex = normalizedPath.lastIndexOf("/")
    if (separatorIndex < 0) {
        return normalizedPath
    }
    return normalizedPath.slice(separatorIndex + 1)
}

function fileNameWithoutExtension(fileName) {
    const name = String(fileName || "")
    const extensionSeparatorIndex = name.lastIndexOf(".")
    if (extensionSeparatorIndex <= 0) {
        return name
    }
    return name.slice(0, extensionSeparatorIndex)
}

function outputPathFromInputPath(inputPath) {
    const folderPath = folderPathFromFilePath(inputPath)
    const fileName = fileNameFromPath(inputPath)
    const outputFileName = fileNameWithoutExtension(fileName) + "-cut.mp4"
    return folderPath ? (folderPath + "/" + outputFileName) : outputFileName
}

function temporaryOutputPathFromOutputPath(outputPath) {
    return outputPath + ".temporary-" + Date.now() + ".mp4"
}

function fileExtension(fileName) {
    const name = String(fileName || "")
    const extensionSeparatorIndex = name.lastIndexOf(".")
    if (extensionSeparatorIndex <= 0 || extensionSeparatorIndex >= name.length - 1) {
        return ""
    }
    return name.slice(extensionSeparatorIndex + 1).toLowerCase()
}

function isVideoFilePath(path) {
    const extension = fileExtension(fileNameFromPath(path))
    const supportedVideoExtensions = [
        "mp4", "mkv", "mov", "avi", "webm", "m4v", "wmv", "mpg", "mpeg", "ts", "mts", "m2ts"
    ]
    return supportedVideoExtensions.indexOf(extension) >= 0
}

function scriptDefinition() {
    return {
        name: "Cut video",
        description: "Cut selected videos with ffmpeg (stream copy, no re-encoding).",
        controls: [
            {
                id: "startTime",
                type: "text",
                label: "Start time (h:m:s)",
                default: "00:00:00"
            },
            {
                id: "endTime",
                type: "text",
                label: "End time (h:m:s)",
                default: "00:00:10"
            }
        ],
        run: function(params, selection) {
            const startTime = String(params.startTime || "00:00:00")
            const endTime = String(params.endTime || "00:00:10")
            const items = Array.isArray(selection) ? selection : []
            const files = items.filter(item => item && item.path && !item.isDir && isVideoFilePath(item.path))
            const results = []

            for (const item of files) {
                const inputPath = String(item.path)
                const outputPath = outputPathFromInputPath(inputPath)
                const temporaryOutputPath = temporaryOutputPathFromOutputPath(outputPath)
                const arguments = [
                    "-y",
                    "-ss", startTime,
                    "-to", endTime,
                    "-i", inputPath,
                    "-c", "copy",
                    "-movflags", "+faststart",
                    temporaryOutputPath
                ]

                const processResult = scriptEngine.runProcess("ffmpeg", arguments)
                let finalOk = !!processResult.ok
                let finalError = processResult.error || ""
                let finalStderr = processResult.stderr || ""
                let finalExitCode = processResult.exitCode !== undefined ? processResult.exitCode : -1

                if (finalOk) {
                    scriptEngine.moveToTrash(outputPath)
                    const renameResult = scriptEngine.renameFile(temporaryOutputPath, outputPath)
                    if (!renameResult.ok) {
                        finalOk = false
                        finalError = renameResult.error || "Cannot finalize output file"
                    }
                } else {
                    scriptEngine.moveToTrash(temporaryOutputPath)
                }

                results.push({
                    from: inputPath,
                    to: outputPath,
                    ok: finalOk,
                    error: finalError,
                    exitCode: finalExitCode,
                    stderr: finalStderr
                })
            }

            return results
        }
    }
}