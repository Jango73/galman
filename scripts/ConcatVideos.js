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

function escapeForConcatFile(path) {
    return String(path || "").replace(/'/g, "'\\''")
}

function buildConcatListContent(files) {
    const lines = []
    for (const file of files) {
        lines.push("file '" + escapeForConcatFile(file.name) + "'")
    }
    return lines.join("\r\n") + "\r\n"
}

function sameFolderForAllFiles(files) {
    if (files.length === 0) {
        return ""
    }

    const firstFolderPath = folderPathFromFilePath(files[0].path)
    for (const file of files) {
        if (folderPathFromFilePath(file.path) !== firstFolderPath) {
            return ""
        }
    }
    return firstFolderPath
}

function scriptDefinition() {
    return {
        name: "Concat videos",
        description: "Concatenate selected videos with ffmpeg.",
        run: function(params, selection) {
            const items = Array.isArray(selection) ? selection : []
            const files = items
                .filter(item => item && item.path && !item.isDir && isVideoFilePath(item.path))
                .map(item => ({
                    path: String(item.path),
                    name: fileNameFromPath(item.path)
                }))

            if (files.length === 0) {
                return [{
                    ok: false,
                    error: "No video selected"
                }]
            }

            const workingFolder = sameFolderForAllFiles(files)
            if (!workingFolder) {
                return [{
                    ok: false,
                    error: "All selected videos must be in the same folder"
                }]
            }

            const listPath = workingFolder + "/list.txt"
            const outputPath = workingFolder + "/output.mp4"
            const writeResult = scriptEngine.writeTextFile(listPath, buildConcatListContent(files))
            if (!writeResult.ok) {
                return [{
                    ok: false,
                    error: writeResult.error || "Cannot write list.txt"
                }]
            }

            const arguments = [
                "-f", "concat",
                "-safe", "0",
                "-i", "list.txt",
                "-vf", "pad=ceil(iw/2)*2:ceil(ih/2)*2",
                "-c:v", "libx264",
                "-preset", "medium",
                "-crf", "20",
                "-c:a", "aac",
                "-b:a", "192k",
                "output.mp4"
            ]

            const processResult = scriptEngine.runProcess("ffmpeg", arguments, workingFolder)
            return [{
                from: listPath,
                to: outputPath,
                ok: !!processResult.ok,
                error: processResult.error || "",
                exitCode: processResult.exitCode !== undefined ? processResult.exitCode : -1,
                stderr: processResult.stderr || ""
            }]
        }
    }
}
