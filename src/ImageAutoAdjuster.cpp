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

#include "ImageAutoAdjuster.h"

#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QImage>
#include <QImageReader>
#include <QUrl>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace {
struct ImageAutoAdjusterDefaults {
    static constexpr bool autoEnabled = false;
    static constexpr bool busy = false;
    static constexpr bool busyEnabled = true;
    static constexpr double brightnessValue = 0.0;
    static constexpr double contrastValue = 0.0;
    static constexpr quint64 requestToken = 0;
};

struct ImageAutoAdjusterConstants {
    static constexpr int histogramBinCount = 256;
    static constexpr int zeroIndex = 0;
    static constexpr int oneIndex = 1;
    static constexpr quint64 zeroCount = 0;
    static constexpr double zero = 0.0;
    static constexpr double one = 1.0;
    static constexpr double percentileLower = 0.005;
    static constexpr double percentileUpper = 0.995;
    static constexpr double percentileUpperSafety = 0.995;
    static constexpr double targetLower = 0.05;
    static constexpr double targetUpper = 0.9;
    static constexpr double gainMinimum = 0.6;
    static constexpr double gainMaximum = 1.4;
    static constexpr double pivot = 0.5;
    static constexpr double minimumRange = 0.000001;
    static constexpr double balancedLower = 0.01;
    static constexpr double balancedUpper = 0.99;
    static constexpr double balancedClippedFraction = 0.01;
    static constexpr double brightnessMinimum = -1.0;
    static constexpr double brightnessMaximum = 1.0;
    static constexpr double contrastMinimum = -1.0;
    static constexpr double contrastMaximum = 1.0;
};

struct AdjustmentResult {
    double brightnessValue = ImageAutoAdjusterDefaults::brightnessValue;
    double contrastValue = ImageAutoAdjusterDefaults::contrastValue;
    bool valid = false;
};

QString normalizeImagePath(const QString &imagePath)
{
    if (imagePath.isEmpty()) {
        return imagePath;
    }
    const QUrl url(imagePath);
    if (url.isLocalFile()) {
        const QUrl withoutQuery = url.adjusted(QUrl::RemoveQuery);
        const QString localPath = withoutQuery.toLocalFile();
        if (!localPath.isEmpty()) {
            return localPath;
        }
    }
    return imagePath;
}

QImage loadImage(const QString &imagePath)
{
    QImageReader reader(imagePath);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull()) {
        return image;
    }
    if (image.format() != QImage::Format_Grayscale8) {
        image = image.convertToFormat(QImage::Format_Grayscale8);
    }
    return image;
}

int findPercentileIndex(const QVector<quint64> &histogram, quint64 targetCount)
{
    quint64 runningTotal = ImageAutoAdjusterConstants::zeroCount;
    const int histogramSize = histogram.size();
    for (int index = ImageAutoAdjusterConstants::zeroIndex; index < histogramSize; ++index) {
        runningTotal += histogram.at(index);
        if (runningTotal >= targetCount) {
            return index;
        }
    }
    return histogramSize - ImageAutoAdjusterConstants::oneIndex;
}

double computeClippedFraction(const QVector<quint64> &histogram, quint64 pixelCount, double gain, double brightness)
{
    const double histogramDenominator = static_cast<double>(ImageAutoAdjusterConstants::histogramBinCount
                                                            - ImageAutoAdjusterConstants::oneIndex);
    quint64 clippedCount = ImageAutoAdjusterConstants::zeroCount;
    const int histogramSize = histogram.size();
    for (int index = ImageAutoAdjusterConstants::zeroIndex; index < histogramSize; ++index) {
        const double value = static_cast<double>(index) / histogramDenominator;
        const double mapped = (value - ImageAutoAdjusterConstants::pivot) * gain
            + ImageAutoAdjusterConstants::pivot
            + brightness;
        if (mapped <= ImageAutoAdjusterConstants::zero || mapped >= ImageAutoAdjusterConstants::one) {
            clippedCount += histogram.at(index);
        }
    }
    if (pixelCount == ImageAutoAdjusterConstants::zeroCount) {
        return ImageAutoAdjusterConstants::zero;
    }
    return static_cast<double>(clippedCount) / static_cast<double>(pixelCount);
}

AdjustmentResult calculateAdjustments(const QString &imagePath)
{
    const QString normalizedPath = normalizeImagePath(imagePath);
    const QImage image = loadImage(normalizedPath);
    if (image.isNull()) {
        return {};
    }

    const int imageWidth = image.width();
    const int imageHeight = image.height();
    const quint64 pixelCount = static_cast<quint64>(imageWidth) * static_cast<quint64>(imageHeight);
    if (pixelCount == ImageAutoAdjusterConstants::zeroCount) {
        return {};
    }

    QVector<quint64> histogram(ImageAutoAdjusterConstants::histogramBinCount,
                               ImageAutoAdjusterConstants::zeroCount);
    for (int rowIndex = ImageAutoAdjusterConstants::zeroIndex; rowIndex < imageHeight; ++rowIndex) {
        const uchar *line = image.constScanLine(rowIndex);
        for (int columnIndex = ImageAutoAdjusterConstants::zeroIndex;
             columnIndex < imageWidth;
             ++columnIndex) {
            const int value = line[columnIndex];
            histogram[value] += ImageAutoAdjusterConstants::oneIndex;
        }
    }

    const double percentileLowerTarget = static_cast<double>(pixelCount) * ImageAutoAdjusterConstants::percentileLower;
    const double percentileUpperTarget = static_cast<double>(pixelCount) * ImageAutoAdjusterConstants::percentileUpper;
    const double percentileUpperSafetyTarget =
        static_cast<double>(pixelCount) * ImageAutoAdjusterConstants::percentileUpperSafety;
    const quint64 percentileLowerCount = static_cast<quint64>(std::ceil(percentileLowerTarget));
    const quint64 percentileUpperCount = static_cast<quint64>(std::ceil(percentileUpperTarget));
    const quint64 percentileUpperSafetyCount = static_cast<quint64>(std::ceil(percentileUpperSafetyTarget));

    const int lowerIndex = findPercentileIndex(histogram, percentileLowerCount);
    const int upperIndex = findPercentileIndex(histogram, percentileUpperCount);
    const int upperSafetyIndex = findPercentileIndex(histogram, percentileUpperSafetyCount);

    const double histogramDenominator = static_cast<double>(ImageAutoAdjusterConstants::histogramBinCount
                                                            - ImageAutoAdjusterConstants::oneIndex);
    const double lowerValue = static_cast<double>(lowerIndex) / histogramDenominator;
    const double upperValue = static_cast<double>(upperIndex) / histogramDenominator;
    const double upperSafetyValue = static_cast<double>(upperSafetyIndex) / histogramDenominator;
    const double rangeValue = std::max(upperValue - lowerValue, ImageAutoAdjusterConstants::minimumRange);

    double gain = (ImageAutoAdjusterConstants::targetUpper - ImageAutoAdjusterConstants::targetLower) / rangeValue;
    gain = std::clamp(gain, ImageAutoAdjusterConstants::gainMinimum, ImageAutoAdjusterConstants::gainMaximum);

    double brightnessValue = ImageAutoAdjusterConstants::targetLower
        - (lowerValue - ImageAutoAdjusterConstants::pivot) * gain
        - ImageAutoAdjusterConstants::pivot;

    if (gain > ImageAutoAdjusterConstants::one
        && brightnessValue > ImageAutoAdjusterConstants::zero
        && upperSafetyValue > upperValue) {
        const double safetyRange = std::max(upperSafetyValue - lowerValue, ImageAutoAdjusterConstants::minimumRange);
        const double safetyGain =
            (ImageAutoAdjusterConstants::targetUpper - ImageAutoAdjusterConstants::targetLower) / safetyRange;
        const double clampedSafetyGain =
            std::clamp(safetyGain, ImageAutoAdjusterConstants::gainMinimum, gain);
        if (clampedSafetyGain < gain) {
            gain = clampedSafetyGain;
            brightnessValue = ImageAutoAdjusterConstants::targetLower
                - (lowerValue - ImageAutoAdjusterConstants::pivot) * gain
                - ImageAutoAdjusterConstants::pivot;
        }
    }

    const double clippedFraction = computeClippedFraction(histogram, pixelCount, gain, brightnessValue);
    const bool balancedRange = lowerValue >= ImageAutoAdjusterConstants::balancedLower
        && upperValue <= ImageAutoAdjusterConstants::balancedUpper;
    if (balancedRange && clippedFraction <= ImageAutoAdjusterConstants::balancedClippedFraction) {
        gain = ImageAutoAdjusterConstants::one;
        brightnessValue = ImageAutoAdjusterConstants::zero;
    }
    const double contrastValue = gain - ImageAutoAdjusterConstants::one;

    AdjustmentResult result;
    result.brightnessValue = std::clamp(brightnessValue,
                                        ImageAutoAdjusterConstants::brightnessMinimum,
                                        ImageAutoAdjusterConstants::brightnessMaximum);
    result.contrastValue = std::clamp(contrastValue,
                                      ImageAutoAdjusterConstants::contrastMinimum,
                                      ImageAutoAdjusterConstants::contrastMaximum);
    result.valid = true;
    return result;
}
} // namespace

ImageAutoAdjuster::ImageAutoAdjuster(QObject *parent)
    : QObject(parent)
    , m_autoEnabled(ImageAutoAdjusterDefaults::autoEnabled)
    , m_brightnessValue(ImageAutoAdjusterDefaults::brightnessValue)
    , m_contrastValue(ImageAutoAdjusterDefaults::contrastValue)
    , m_busy(ImageAutoAdjusterDefaults::busy)
    , m_requestToken(ImageAutoAdjusterDefaults::requestToken)
{
}

QString ImageAutoAdjuster::imagePath() const
{
    return m_imagePath;
}

void ImageAutoAdjuster::setImagePath(const QString &imagePath)
{
    if (m_imagePath == imagePath) {
        return;
    }
    m_imagePath = imagePath;
    emit imagePathChanged();
    requestAdjustment();
}

bool ImageAutoAdjuster::autoEnabled() const
{
    return m_autoEnabled;
}

void ImageAutoAdjuster::setAutoEnabled(bool autoEnabled)
{
    if (m_autoEnabled == autoEnabled) {
        return;
    }
    m_autoEnabled = autoEnabled;
    emit autoEnabledChanged();
    requestAdjustment();
}

double ImageAutoAdjuster::brightnessValue() const
{
    return m_brightnessValue;
}

double ImageAutoAdjuster::contrastValue() const
{
    return m_contrastValue;
}

bool ImageAutoAdjuster::busy() const
{
    return m_busy;
}

void ImageAutoAdjuster::refresh()
{
    requestAdjustment();
}

void ImageAutoAdjuster::requestAdjustment()
{
    if (!m_autoEnabled || m_imagePath.isEmpty()) {
        if (m_busy) {
            m_busy = ImageAutoAdjusterDefaults::busy;
            emit busyChanged();
        }
        return;
    }

    const quint64 requestToken = m_requestToken + ImageAutoAdjusterConstants::oneIndex;
    m_requestToken = requestToken;

    if (!m_busy) {
        m_busy = ImageAutoAdjusterDefaults::busyEnabled;
        emit busyChanged();
    }

    auto *watcher = new QFutureWatcher<AdjustmentResult>(this);
    const QString imagePath = m_imagePath;
    QFuture<AdjustmentResult> future = QtConcurrent::run([imagePath]() {
        return calculateAdjustments(imagePath);
    });

    connect(watcher, &QFutureWatcher<AdjustmentResult>::finished, this, [this, watcher, requestToken]() {
        const AdjustmentResult result = watcher->result();
        watcher->deleteLater();

        if (m_requestToken != requestToken) {
            return;
        }

        if (m_busy) {
            m_busy = ImageAutoAdjusterDefaults::busy;
            emit busyChanged();
        }

        if (!result.valid) {
            return;
        }

        if (!qFuzzyCompare(m_brightnessValue, result.brightnessValue)) {
            m_brightnessValue = result.brightnessValue;
            emit brightnessValueChanged();
        }
        if (!qFuzzyCompare(m_contrastValue, result.contrastValue)) {
            m_contrastValue = result.contrastValue;
            emit contrastValueChanged();
        }
    });

    watcher->setFuture(future);
}
