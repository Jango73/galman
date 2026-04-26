function scriptDefinition() {
    const flipOptions = [
        { value: "h", label: "Horizontal flip" },
        { value: "v", label: "Vertical flip" }
    ]
    return {
        name: "Flip images",
        description: "Flip selected images in place.",
        controls: [
            {
                id: "flipType",
                type: "combo",
                label: "Flip",
                options: flipOptions.map(o => o.label),
                values: flipOptions.map(o => o.value),
                default: 0
            }
        ],
        run: function(params, selection) {
            const items = Array.isArray(selection) ? selection : []
            const images = items.filter(item => item && item.path && item.isImage)

            const flipType = String(params.flipType || "h")
            const flipH = flipType === "h"
            const flipV = flipType === "v"

            const results = []
            for (const item of images) {
                const res = scriptEngine.adjustImage(item.path, 0, flipH, flipV)
                results.push({
                    from: item.path,
                    ok: !!res.ok,
                    error: res.error || ""
                })
            }

            return results
        }
    }
}