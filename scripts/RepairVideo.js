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
        description: "Repair selected videos with ffmpeg (libx264 + audio copy).",
        run: function(params, selection) {
            const items = Array.isArray(selection) ? selection : []
            const files = items.filter(item => item && item.path && !item.isDir && isVideoFilePath(item.path))
            const results = []

            for (const item of files) {
                const inputPath = String(item.path)
                const outputPath = outputPathFromInputPath(inputPath)
                const arguments = [
                    "-fflags", "+genpts",
                    "-i", inputPath,
                    "-c:v", "libx264",
                    "-preset", "fast",
                    "-crf", "18",
                    "-c:a", "copy",
                    outputPath
                ]

                const processResult = scriptEngine.runProcess("ffmpeg", arguments)
                results.push({
                    from: inputPath,
                    to: outputPath,
                    ok: !!processResult.ok,
                    error: processResult.error || "",
                    exitCode: processResult.exitCode !== undefined ? processResult.exitCode : -1,
                    stderr: processResult.stderr || ""
                })
            }

            return results
        }
    }
}
