function padNumber(value, digits) {
    const text = String(value)
    if (text.length >= digits) {
        return text
    }
    return "0".repeat(digits - text.length) + text
}

function scriptDefinition() {
    return {
        name: "Rename by date",
        description: "Rename selected files by modified date order.",
        controls: [
            {
                id: "prefix",
                type: "text",
                label: "Prefix",
                placeholder: "Optional"
            },
            {
                id: "suffix",
                type: "text",
                label: "Suffix",
                placeholder: "Optional"
            },
            {
                id: "digits",
                type: "number",
                label: "Digits",
                min: 1,
                max: 8,
                default: 3
            }
        ],
        run: function(params, selection) {
            const prefix = params.prefix || ""
            const suffix = params.suffix || ""
            const digits = Number(params.digits || 1)

            const files = (selection || []).filter(item => item && item.path && !item.isDir)
            files.sort((a, b) => (a.modifiedMs || 0) - (b.modifiedMs || 0))

            const results = []
            let counter = 1

            for (const item of files) {
                const path = item.path
                const lastSlash = path.lastIndexOf("/")
                const dir = lastSlash >= 0 ? path.slice(0, lastSlash) : ""
                const dot = path.lastIndexOf(".")
                const ext = dot > lastSlash ? path.slice(dot) : ""
                const base = prefix + padNumber(counter, digits) + suffix
                const target = (dir ? dir + "/" : "") + base + ext

                const result = scriptEngine.renameFile(path, target)
                results.push({
                    from: path,
                    to: target,
                    ok: !!result.ok,
                    error: result.error || ""
                })

                if (result.ok) {
                    counter += 1
                }
            }

            return results
        }
    }
}
