#pragma once

#include <QJsonObject>
#include <QString>

class ComfyWorkflowParser
{
public:
    static QJsonObject load(const QString &path, QString *error);
    static bool injectLoadImage(QJsonObject &workflow, const QString &imagePath, QString *error);
    static QJsonObject toPrompt(const QJsonObject &workflow);

private:
    static void normalizeNodes(QJsonObject &workflow);
    static QJsonObject findLoadImageNode(QJsonObject &workflow);
    static QJsonObject buildWidgetMap(const QString &classType, const QJsonArray &widgets);
};
