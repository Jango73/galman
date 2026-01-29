/************************************************************************\

    Galman - Picture gallery manager
    Copyright (C) 2026 Jango73

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

\************************************************************************/

#include "TrashWorker.h"

#include <QFileInfo>

#include "PlatformUtils.h"

namespace {
struct TrashWorkerConstants {
    static constexpr int emptyCount = 0;
    static constexpr int singleStep = 1;
    static constexpr int notCancelled = 0;
    static constexpr int cancelled = 1;
};
} // namespace

TrashWorker::TrashWorker(const QStringList &paths, RemovalMode mode, QObject *parent)
    : QObject(parent)
    , m_paths(paths)
    , m_mode(mode)
{
}

void TrashWorker::cancel()
{
    m_cancelled.storeRelaxed(TrashWorkerConstants::cancelled);
}

bool TrashWorker::isCancelled() const
{
    return m_cancelled.loadRelaxed() != TrashWorkerConstants::notCancelled;
}

void TrashWorker::tick(int &completed, int total)
{
    completed += TrashWorkerConstants::singleStep;
    emit progress(completed, total);
}

void TrashWorker::start()
{
    QVariantMap result;
    result.insert("ok", false);

    const int total = m_paths.size();
    if (total <= TrashWorkerConstants::emptyCount) {
        result.insert("error", tr("Nothing to delete"));
        emit finished(result);
        return;
    }

    int completed = TrashWorkerConstants::emptyCount;
    int moved = TrashWorkerConstants::emptyCount;
    int failed = TrashWorkerConstants::emptyCount;
    QString firstError;
    QStringList touchedPaths;
    touchedPaths.reserve(total);

    emit progress(completed, total);

    for (const QString &path : m_paths) {
        if (isCancelled()) {
            if (firstError.isEmpty()) {
                firstError = tr("Delete cancelled");
            }
            break;
        }
        if (!QFileInfo::exists(path)) {
            failed += TrashWorkerConstants::singleStep;
            if (firstError.isEmpty()) {
                firstError = tr("Source not found");
            }
            tick(completed, total);
            continue;
        }

        QString error;
        const bool ok = (m_mode == DeletePermanently)
            ? PlatformUtils::deletePermanently(path, &error)
            : PlatformUtils::moveToTrashOrDelete(path, &error);
        if (!ok) {
            failed += TrashWorkerConstants::singleStep;
            if (firstError.isEmpty()) {
                const QString fallback = (m_mode == DeletePermanently)
                    ? tr("Failed to delete")
                    : tr("Failed to move to trash");
                firstError = error.isEmpty() ? fallback : error;
            }
            tick(completed, total);
            continue;
        }

        touchedPaths.append(path);
        moved += TrashWorkerConstants::singleStep;
        tick(completed, total);
    }

    result.insert("moved", moved);
    result.insert("failed", failed);
    if (!firstError.isEmpty()) {
        result.insert("error", firstError);
    }
    const bool cancelled = isCancelled();
    if (cancelled) {
        result.insert("cancelled", true);
    }
    result.insert("touchedPaths", touchedPaths);
    result.insert("ok", failed == TrashWorkerConstants::emptyCount && !cancelled);

    emit finished(result);
}
