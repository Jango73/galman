function padNumber(value, digits) {
    const text = String(value)
    if (text.length >= digits) {
        return text
    }
    return "0".repeat(digits - text.length) + text
}

function scriptDefinition() {
    return {
        name: "Pad numbers",
        description: "Pad all numbers in selected filenames to a fixed digit count.",
        controls: [
            {
                id: "digits",
                type: "number",
                label: "Digits",
                min: 1,
                max: 12,
                default: 3
            }
        ],
        run: function(params, selection) {
            const digits = Math.max(1, Number(params.digits || 1))
            const items = Array.isArray(selection) ? selection : []
            const files = items.filter(item => item && item.path && !item.isDir)

            const results = []
            for (const item of files) {
                const path = item.path
                const lastSlash = path.lastIndexOf("/")
                const dir = lastSlash >= 0 ? path.slice(0, lastSlash) : ""
                const name = lastSlash >= 0 ? path.slice(lastSlash + 1) : path
                const dot = name.lastIndexOf(".")
                const ext = dot > 0 ? name.slice(dot) : ""
                const base = dot > 0 ? name.slice(0, dot) : name
                const padded = base.replace(/\d+/g, match => padNumber(match, digits))
                const target = (dir ? dir + "/" : "") + padded + ext

                if (target === path) {
                    results.push({
                        from: path,
                        to: target,
                        ok: true,
                        error: ""
                    })
                    continue
                }

                const result = scriptEngine.renameFile(path, target)
                results.push({
                    from: path,
                    to: target,
                    ok: !!result.ok,
                    error: result.error || ""
                })
            }

            return results
        }
    }
}
