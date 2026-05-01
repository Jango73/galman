function scriptDefinition() {
    const rotationOptions = [
        { value: 90, label: "+090° (turn right)" },
        { value: -90, label: "-090° (turn left)" },
        { value: 180, label: "+180° (turn 180)" }
    ]
    return {
        name: "Rotate images",
        description: "Rotate selected images in place.",
        controls: [
            {
                id: "rotation",
                type: "combo",
                label: "Rotation",
                options: rotationOptions.map(o => o.label),
                values: rotationOptions.map(o => o.value),
                default: 0
            }
        ],
        run: function(params, selection) {
            const items = Array.isArray(selection) ? selection : []
            const images = items.filter(item => item && item.path && item.isImage)

            const rotationValue = parseInt(params.rotation, 10) || 90

            const results = []
            for (const item of images) {
                const res = scriptEngine.adjustImage(item.path, rotationValue, false, false)
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