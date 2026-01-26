function scriptDefinition() {
    const formats = scriptEngine.supportedImageFormats
        ? scriptEngine.supportedImageFormats()
        : ["jpg", "png"]
    const defaultFormat = formats.indexOf("jpg") >= 0
        ? "jpg"
        : (formats.length > 0 ? formats[0] : "jpg")
    return {
        name: "Convert images",
        description: "Convert selected images to another format.",
        controls: [
            {
                id: "format",
                type: "combo",
                label: "Format",
                options: formats,
                default: defaultFormat
            }
        ],
        run: function(params, selection) {
            const format = String(params.format || "jpg").toLowerCase()
            const items = Array.isArray(selection) ? selection : []
            const images = items.filter(item => item && item.path && item.isImage)

            const results = []
            for (const item of images) {
                const res = scriptEngine.convertImage(item.path, format)
                results.push({
                    from: item.path,
                    to: res.target || "",
                    ok: !!res.ok,
                    error: res.error || ""
                })
            }

            return results
        }
    }
}
