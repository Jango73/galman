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

function temporaryFilePath(workingFolder, prefix, index) {
    return workingFolder + "/" + prefix + "_" + String(index) + "_" + String(Date.now()) + ".mp4"
}

function videoDimensions(path) {
    const probeArguments = [
        "-v", "error",
        "-select_streams", "v:0",
        "-show_entries", "stream=width,height",
        "-of", "csv=p=0",
        path
    ]
    const result = scriptEngine.runProcess("ffprobe", probeArguments)
    if (!result.ok) {
        return null
    }
    const output = String(result.stdout || "").trim()
    if (output.length === 0) {
        return null
    }
    const parts = output.split(",")
    if (parts.length < 2) {
        return null
    }
    const w = parseInt(parts[0], 10)
    const h = parseInt(parts[1], 10)
    if (isNaN(w) || isNaN(h) || w === 0 || h === 0) {
        return null
    }
    return { width: w, height: h }
}

function videoFrameRate(path) {
    const probeArguments = [
        "-v", "error",
        "-select_streams", "v:0",
        "-show_entries", "stream=r_frame_rate",
        "-of", "csv=p=0",
        path
    ]
    const result = scriptEngine.runProcess("ffprobe", probeArguments)
    if (!result.ok) {
        return null
    }
    const output = String(result.stdout || "").trim()
    if (output.length === 0) {
        return null
    }
    const parts = output.split("/")
    if (parts.length !== 2) {
        return null
    }
    const numerator = parseInt(parts[0], 10)
    const denominator = parseInt(parts[1], 10)
    if (isNaN(numerator) || isNaN(denominator) || denominator === 0) {
        return null
    }
    return numerator / denominator
}

function videoHasAudioTrack(path) {
    const probeArguments = [
        "-v", "error",
        "-select_streams", "a:0",
        "-show_entries", "stream=index",
        "-of", "csv=p=0",
        path
    ]
    const result = scriptEngine.runProcess("ffprobe", probeArguments)
    if (!result.ok) {
        return false
    }
    const output = String(result.stdout || "").trim()
    return output.length > 0
}

function buildAlignmentVideoFilter(alignment, maxWidth, maxHeight, inputWidth, inputHeight) {
    if (inputWidth === maxWidth && inputHeight === maxHeight) {
        return "pad=ceil(iw/2)*2:ceil(ih/2)*2"
    }
    if (alignment === "Center") {
        return "pad=" + maxWidth + ":" + maxHeight + ":(" + maxWidth + "-" + inputWidth + ")/2:(" + maxHeight + "-" + inputHeight + ")/2"
    }
    return "scale=" + maxWidth + ":" + maxHeight + ":force_original_aspect_ratio=decrease" +
        ",pad=" + maxWidth + ":" + maxHeight + ":(" + maxWidth + "-iw)/2:(" + maxHeight + "-ih)/2"
}

function scriptDefinition() {
    return {
        name: "Concat videos",
        description: "Concatenate selected videos into one. Different sizes are normalized to the largest dimensions.",
        controls: [
            {
                id: "alignment",
                type: "combo",
                label: "Smaller video alignment",
                options: ["Fit", "Center"],
                default: "Fit"
            }
        ],
        run: function(params, selection) {
            const alignment = String(params.alignment || "Fit")
            const items = Array.isArray(selection) ? selection : []
            const files = items.filter(function(item) {
                return item && item.path && !item.isDir && isVideoFilePath(item.path)
            })

            if (files.length < 2) {
                return [{
                    ok: false,
                    error: "Select at least 2 videos to concatenate"
                }]
            }

            const workingFolder = sameFolderForAllFiles(files)
            if (!workingFolder) {
                return [{
                    ok: false,
                    error: "All selected videos must be in the same folder"
                }]
            }

            var fileInfos = []
            var maxWidth = 0
            var maxHeight = 0
            var maxFrameRate = 0
            for (var i = 0; i < files.length; i++) {
                var inputPath = String(files[i].path)
                var dimensions = videoDimensions(inputPath)
                if (!dimensions) {
                    return [{
                        ok: false,
                        error: "Could not determine dimensions for " + fileNameFromPath(inputPath)
                    }]
                }
                var hasAudio = videoHasAudioTrack(inputPath)
                var frameRate = videoFrameRate(inputPath)
                fileInfos.push({
                    path: inputPath,
                    width: dimensions.width,
                    height: dimensions.height,
                    hasAudio: hasAudio,
                    frameRate: frameRate
                })
                if (frameRate !== null && frameRate > maxFrameRate) {
                    maxFrameRate = frameRate
                }
                if (dimensions.width > maxWidth) {
                    maxWidth = dimensions.width
                }
                if (dimensions.height > maxHeight) {
                    maxHeight = dimensions.height
                }
            }

            var targetFrameRate = maxFrameRate > 0 ? String(Math.round(maxFrameRate)) : "30"

            var temporaryFiles = []
            var temporaryItems = []
            var failed = false
            var errorMessage = ""
            var errorExitCode = -1
            var errorStderr = ""

            for (var k = 0; k < fileInfos.length; k++) {
                var fileInfo = fileInfos[k]
                var inputPath = fileInfo.path
                var inputW = fileInfo.width
                var inputH = fileInfo.height
                var audioPresent = fileInfo.hasAudio
                var prefix = fileNameWithoutExtension(fileNameFromPath(inputPath))
                var temporaryPath = temporaryFilePath(workingFolder, prefix, k)

                var videoFilter = buildAlignmentVideoFilter(alignment, maxWidth, maxHeight, inputW, inputH)

                var processArgs = ["-y", "-i", inputPath]

                if (!audioPresent) {
                    processArgs.push("-f", "lavfi", "-i", "anullsrc=r=48000:cl=stereo")
                }

                processArgs.push(
                    "-vf", videoFilter,
                    "-r", targetFrameRate,
                    "-c:v", "libx264",
                    "-preset", "fast",
                    "-crf", "20",
                    "-c:a", "aac",
                    "-b:a", "192k",
                    "-map", "0:v:0"
                )

                if (audioPresent) {
                    processArgs.push("-map", "0:a:0")
                } else {
                    processArgs.push("-map", "1:a", "-shortest")
                }

                processArgs.push(temporaryPath)

                var processResult = scriptEngine.runProcess("ffmpeg", processArgs)
                if (!processResult.ok) {
                    for (var t = 0; t < temporaryFiles.length; t++) {
                        scriptEngine.deleteFile(temporaryFiles[t])
                    }
                    failed = true
                    errorMessage = "Failed to normalize " + fileNameFromPath(inputPath) + ": " + (processResult.error || "unknown error")
                    errorExitCode = processResult.exitCode !== undefined ? processResult.exitCode : -1
                    errorStderr = processResult.stderr || ""
                    break
                }

                temporaryFiles.push(temporaryPath)
                temporaryItems.push({
                    path: temporaryPath,
                    name: fileNameFromPath(temporaryPath)
                })
            }

            if (failed) {
                return [{
                    ok: false,
                    error: errorMessage,
                    exitCode: errorExitCode,
                    stderr: errorStderr
                }]
            }

            var outputName = fileNameWithoutExtension(fileNameFromPath(files[0].path)) + "_concat.mp4"
            var outputPath = workingFolder + "/" + outputName

            var listFileName = "concat_list_" + String(Date.now()) + ".txt"
            var listPath = workingFolder + "/" + listFileName
            var writeResult = scriptEngine.writeTextFile(listPath, buildConcatListContent(temporaryItems))
            if (!writeResult.ok) {
                for (var u = 0; u < temporaryFiles.length; u++) {
                    scriptEngine.deleteFile(temporaryFiles[u])
                }
                return [{
                    ok: false,
                    error: writeResult.error || "Cannot write concat list"
                }]
            }

            var concatArguments = [
                "-fflags", "+genpts",
                "-f", "concat",
                "-safe", "0",
                "-i", listFileName,
                "-c:v", "libx264",
                "-preset", "medium",
                "-crf", "20",
                "-c:a", "aac",
                "-b:a", "192k",
                "-vsync", "2",
                "-movflags", "+faststart",
                outputName
            ]

            var concatResult = scriptEngine.runProcess("ffmpeg", concatArguments, workingFolder)

            scriptEngine.deleteFile(listPath)
            for (var v = 0; v < temporaryFiles.length; v++) {
                scriptEngine.deleteFile(temporaryFiles[v])
            }

            if (!concatResult.ok) {
                var ffmpegStderr = String(concatResult.stderr || "").trim()
                var errorLines = ffmpegStderr.split("\n")
                var lastLine = ""
                for (var el = errorLines.length - 1; el >= 0; el--) {
                    var trimmed = String(errorLines[el]).trim()
                    if (trimmed.length > 0) {
                        lastLine = trimmed
                        break
                    }
                }
                return [{
                    ok: false,
                    error: lastLine || concatResult.error || "Concat failed",
                    exitCode: concatResult.exitCode !== undefined ? concatResult.exitCode : -1,
                    stderr: ffmpegStderr
                }]
            }

            return [{
                from: workingFolder,
                to: outputPath,
                ok: true
            }]
        }
    }
}
