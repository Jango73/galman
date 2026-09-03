#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

#include "ComfyWorkflowBuilder.h"

class ComfyPromptImporter
{
public:
    static QJsonObject readEmbeddedPrompt(const QString &imagePath, QString *error);
    static QStringList extractParameters(const QJsonObject &prompt,
                                         ComfyPilotParameters *parameters,
                                         QString *error);
};
