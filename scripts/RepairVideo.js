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
    const outputFileName = fileNameWithoutExtension(fileName) + "_repaired.mp4"
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
        name: "Repair video",
        description: "Repair selected videos with ffmpeg (libx264 + AAC audio).",
        run: function(params, selection) {
            const items = Array.isArray(selection) ? selection : []
            const files = items.filter(item => item && item.path && !item.isDir && isVideoFilePath(item.path))
            const results = []

            for (const item of files) {
                const inputPath = String(item.path)
                const outputPath = outputPathFromInputPath(inputPath)
                const temporaryOutputPath = temporaryOutputPathFromOutputPath(outputPath)
                const arguments = [
                    "-y",
                    "-fflags", "+genpts",
                    "-i", inputPath,
                    "-map", "0:v:0",
                    "-map", "0:a?",
                    "-c:v", "libx264",
                    "-vf", "scale=ceil(iw/2)*2:ceil(ih/2)*2",
                    "-preset", "fast",
                    "-crf", "18",
                    "-c:a", "aac",
                    "-b:a", "192k",
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
