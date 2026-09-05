#pragma once

#include <QCollator>
#include <QString>
#include <QStringList>
#include <QVector>

#include <QtQml/qqml.h>

class FolderFilterSettings
{
    Q_GADGET
    QML_VALUE_TYPE(folderFilterSettings)

public:
    enum SortKey {
        SortByName = 0,
        SortByExtension,
        SortByCreated,
        SortByModified,
        SortBySignature
    };
    Q_ENUM(SortKey)

    static constexpr qint64 unsetByteSize = -1;
    static constexpr int unsetDimension = -1;
    static constexpr int signatureDimension = 32;

    FolderFilterSettings() = default;

    QString nameFilter() const;
    void setNameFilter(const QString &filter);
    Qt::SortOrder sortOrder() const;
    void setSortOrder(Qt::SortOrder order);
    int sortKeyValue() const;
    void setSortKeyValue(int key);
    bool showFoldersFirst() const;
    void setShowFoldersFirst(bool enabled);
    bool hideJunkFiles() const;
    void setHideJunkFiles(bool enabled);
    qint64 minimumByteSize() const;
    void setMinimumByteSize(qint64 value);
    qint64 maximumByteSize() const;
    void setMaximumByteSize(qint64 value);
    int minimumImageWidth() const;
    void setMinimumImageWidth(int value);
    int maximumImageWidth() const;
    void setMaximumImageWidth(int value);
    int minimumImageHeight() const;
    void setMinimumImageHeight(int value);
    int maximumImageHeight() const;
    void setMaximumImageHeight(int value);
    bool byteSizeFiltersActive() const;
    bool imageSizeFiltersActive() const;

    static QStringList junkExtensions();
    static QString junkExtensionsString();
    static void setJunkExtensionsList(const QString &extensions);

    static qint64 normalizedByteSize(qint64 value);
    static int normalizedDimension(int value);

private:
    QString m_nameFilter;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    int m_sortKeyValue = SortByName;
    bool m_showFoldersFirst = true;
    bool m_hideJunkFiles = true;
    qint64 m_minimumByteSize = unsetByteSize;
    qint64 m_maximumByteSize = unsetByteSize;
    int m_minimumImageWidth = unsetDimension;
    int m_maximumImageWidth = unsetDimension;
    int m_minimumImageHeight = unsetDimension;
    int m_maximumImageHeight = unsetDimension;
};
