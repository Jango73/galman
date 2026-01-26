function _scriptDir(scriptPath) {
    if (!scriptPath) {
        return ""
    }
    const normalized = String(scriptPath).replace(/\\/g, "/")
    const idx = normalized.lastIndexOf("/")
    return idx >= 0 ? normalized.slice(0, idx) : ""
}

function _splitPath(path) {
    const normalized = String(path || "").replace(/\\/g, "/")
    const idx = normalized.lastIndexOf("/")
    const dir = idx >= 0 ? normalized.slice(0, idx) : ""
    const name = idx >= 0 ? normalized.slice(idx + 1) : normalized
    return { dir: dir, name: name }
}

function _extName(name) {
    const idx = name.lastIndexOf(".")
    return idx > 0 ? name.slice(idx) : ""
}

function _baseName(name) {
    const idx = name.lastIndexOf(".")
    return idx > 0 ? name.slice(0, idx) : name
}

function _targetPathForReplace(originalPath, outputPath) {
    const orig = _splitPath(originalPath)
    const out = _splitPath(outputPath)
    const origExt = _extName(orig.name).toLowerCase()
    const outExt = _extName(out.name).toLowerCase()
    let targetName = orig.name
    if (outExt && outExt !== origExt) {
        targetName = _baseName(orig.name) + outExt
    }
    return (orig.dir ? orig.dir + "/" : "") + targetName
}

function scriptDefinition() {
    return {
        name: "ComfyUpscale4",
        description: "Upscale selected images 4x with ComfyUI, replacing the originals.",
        controls: [
            {
                id: "serverUrl",
                type: "text",
                label: "Server URL",
                default: "http://127.0.0.1:8188",
                scope: "global"
            },
            {
                id: "outputDir",
                type: "path",
                label: "Output directory",
                default: scriptEngine.comfyDefaultOutputDir(),
                scope: "global"
            }
        ],
        run: function(params, selection) {
            const baseDir = _scriptDir(params._scriptPath)
            const workflowPath = (baseDir ? baseDir + "/" : "") + "ComfyUpscale4.json"
            const serverUrl = params.serverUrl || "http://127.0.0.1:8188"
            const outputDir = params.outputDir || scriptEngine.comfyDefaultOutputDir()

            const items = Array.isArray(selection) ? selection : []
            const images = items.filter(item => item && item.path && item.isImage)

            const results = []
            for (const item of images) {
                const res = scriptEngine.runComfyWorkflow(workflowPath, item.path, serverUrl, outputDir)
                let finalOutput = res.output || ""
                if (res.ok && finalOutput) {
                    const orig = _splitPath(item.path)
                    const origExt = _extName(orig.name).toLowerCase()
                    const outExt = _extName(_splitPath(finalOutput).name).toLowerCase()
                    if (origExt && outExt && origExt !== outExt) {
                        const format = origExt.slice(1)
                        const convertRes = scriptEngine.convertImage(finalOutput, format)
                        if (!convertRes.ok) {
                            results.push({
                                from: item.path,
                                ok: false,
                                output: "",
                                error: convertRes.error || "Failed to convert output"
                            })
                            continue
                        }
                        finalOutput = convertRes.target || finalOutput
                    }
                    const trashRes = scriptEngine.moveToTrash(item.path)
                    if (!trashRes.ok) {
                        results.push({
                            from: item.path,
                            ok: false,
                            output: "",
                            error: trashRes.error || "Failed to move source to trash"
                        })
                        continue
                    }
                    const targetPath = _targetPathForReplace(item.path, finalOutput)
                    const renameRes = scriptEngine.renameFile(finalOutput, targetPath)
                    if (!renameRes.ok) {
                        results.push({
                            from: item.path,
                            ok: false,
                            output: "",
                            error: renameRes.error || "Failed to replace source"
                        })
                        continue
                    }
                    finalOutput = targetPath
                }
                results.push({
                    from: item.path,
                    ok: !!res.ok,
                    output: finalOutput,
                    error: res.error || ""
                })
            }

            return results
        }
    }
}
