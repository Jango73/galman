#pragma once

#include <QCollator>
#include <QSize>
#include <QString>

#include "FolderEntryView.h"
#include "FolderFilterSettings.h"

namespace FolderFilterSortUtils {

struct ImageSizeLookup {
    const QHash<QString, QSize> *sizes = nullptr;
};

bool matchesByteSize(qint64 byteSize, bool isFolder, const FolderFilterSettings &settings);

bool matchesImageSize(const QString &imageKey,
                      bool isImage,
                      bool isFolder,
                      bool isGhost,
                      const FolderFilterSettings &settings,
                      const QHash<QString, QSize> &imageSizes);

bool matchesNameAndJunk(const FolderEntryView::EntryView &view,
                        const QString &needle,
                        bool nameFilterActive,
                        bool junkFilterActive,
                        const QStringList &junkExtensions);

template <typename EntryType, typename ViewForEntry>
void applyNameJunkSizeFilter(QVector<EntryType> &entries,
                             const FolderFilterSettings &settings,
                             const QHash<QString, QSize> &imageSizes,
                             ViewForEntry viewForEntry)
{
    const QString trimmed = settings.nameFilter().trimmed();
    const bool nameFilterActive = !trimmed.isEmpty();
    const bool byteSizeActive = settings.byteSizeFiltersActive();
    const bool imageSizeActive = settings.imageSizeFiltersActive();
    const QString needle = trimmed.toLower();
    const bool junkFilterActive = settings.hideJunkFiles();
    const QStringList junkList = FolderFilterSettings::junkExtensions();

    if (!nameFilterActive && !byteSizeActive && !imageSizeActive && !junkFilterActive) {
        return;
    }

    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const EntryType &entry) {
        const FolderEntryView::EntryView view = viewForEntry(entry);
        if (!matchesNameAndJunk(view, needle, nameFilterActive, junkFilterActive, junkList)) {
            return true;
        }
        if (byteSizeActive) {
            if (view.isGhost || view.filePath.isEmpty()) {
                return true;
            }
            if (!matchesByteSize(view.byteSize, view.isFolder, settings)) {
                return true;
            }
        }
        if (imageSizeActive) {
            if (!matchesImageSize(view.filePath, view.isImageFlag, view.isFolder, view.isGhost, settings, imageSizes)) {
                return true;
            }
        }
        return false;
    }),
                  entries.end());
}

template <typename EntryType, typename ViewForEntry, typename SignatureForPath>
void sortEntries(QVector<EntryType> &entries,
                 const FolderFilterSettings &settings,
                 ViewForEntry viewForEntry,
                 SignatureForPath signatureForPath)
{
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    const int sortKey = settings.sortKeyValue();
    const bool foldersFirst = settings.showFoldersFirst();

    std::sort(entries.begin(), entries.end(), [&](const EntryType &leftEntry, const EntryType &rightEntry) {
        const FolderEntryView::EntryView left = viewForEntry(leftEntry);
        const FolderEntryView::EntryView right = viewForEntry(rightEntry);
        if (foldersFirst && left.isFolder != right.isFolder) {
            return left.isFolder;
        }
        bool result = false;
        switch (sortKey) {
        case FolderFilterSettings::SortByExtension:
            result = collator.compare(FolderEntryView::suffixForFileName(left.fileName),
                                      FolderEntryView::suffixForFileName(right.fileName))
                < 0;
            break;
        case FolderFilterSettings::SortByCreated:
            result = left.createdTime < right.createdTime;
            break;
        case FolderFilterSettings::SortByModified:
            result = left.modifiedTime < right.modifiedTime;
            break;
        case FolderFilterSettings::SortBySignature: {
            if (!left.isFolder && !right.isFolder) {
                const bool leftHas = signatureForPath(left.filePath, nullptr);
                const bool rightHas = signatureForPath(right.filePath, nullptr);
                if (leftHas && rightHas) {
                    quint64 leftValue = 0;
                    quint64 rightValue = 0;
                    signatureForPath(left.filePath, &leftValue);
                    signatureForPath(right.filePath, &rightValue);
                    result = leftValue < rightValue;
                } else if (leftHas != rightHas) {
                    result = leftHas;
                } else {
                    result = collator.compare(left.fileName, right.fileName) < 0;
                }
            } else {
                result = collator.compare(left.fileName, right.fileName) < 0;
            }
            break;
        }
        case FolderFilterSettings::SortByName:
        default:
            result = collator.compare(left.fileName, right.fileName) < 0;
            break;
        }
        if (settings.sortOrder() == Qt::DescendingOrder) {
            return !result && left.fileName != right.fileName;
        }
        return result;
    });
}

} // namespace FolderFilterSortUtils
