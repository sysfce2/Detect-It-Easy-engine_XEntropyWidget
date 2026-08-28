/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xplotwidget.h"

#include <cmath>

XPlotWidget::XPlotWidget(QWidget *pParent) : QWidget(pParent)
{
    m_axisX = {};
    m_axisY = {};
    m_colorCurve = Qt::red;
    m_dHistStart = 0;
    m_dHistInterval = 1.0;
    m_colorHistogram = Qt::blue;
    m_bGridVisible = false;
    m_bTrackerVisible = false;
    m_bMouseInside = false;

    setMouseTracking(true);
}

void XPlotWidget::setAxisScaleX(double dMin, double dMax, double dStep)
{
    m_axisX.bManual = true;
    m_axisX.dMin = dMin;
    m_axisX.dMax = dMax;
    m_axisX.dStep = dStep;

    update();
}

void XPlotWidget::setAxisScaleY(double dMin, double dMax, double dStep)
{
    m_axisY.bManual = true;
    m_axisY.dMin = dMin;
    m_axisY.dMax = dMax;
    m_axisY.dStep = dStep;

    update();
}

void XPlotWidget::setCurveData(const QVector<double> &vecX, const QVector<double> &vecY)
{
    m_vecCurveX = vecX;
    m_vecCurveY = vecY;

    qint32 nNumberOfPoints = qMin(m_vecCurveX.count(), m_vecCurveY.count());

    m_vecCurveX.resize(nNumberOfPoints);
    m_vecCurveY.resize(nNumberOfPoints);

    update();
}

void XPlotWidget::setCurveColor(const QColor &color)
{
    m_colorCurve = color;

    update();
}

void XPlotWidget::setHistogramData(const QVector<double> &vecValues, double dStart, double dInterval)
{
    m_vecHistValues = vecValues;
    m_dHistStart = dStart;

    if (dInterval > 0) {
        m_dHistInterval = dInterval;
    } else {
        m_dHistInterval = 1.0;
    }

    update();
}

void XPlotWidget::setHistogramColor(const QColor &color)
{
    m_colorHistogram = color;

    update();
}

void XPlotWidget::setGridVisible(bool bVisible)
{
    m_bGridVisible = bVisible;

    update();
}

void XPlotWidget::setTrackerVisible(bool bVisible)
{
    m_bTrackerVisible = bVisible;

    update();
}

void XPlotWidget::setZones(const QList<ZONE> &listZones)
{
    m_listZones = listZones;

    update();
}

void XPlotWidget::setZoneVisible(qint32 nIndex, bool bVisible)
{
    if ((nIndex >= 0) && (nIndex < m_listZones.count())) {
        m_listZones[nIndex].bVisible = bVisible;

        update();
    }
}

void XPlotWidget::clearZonesVisible()
{
    qint32 nNumberOfZones = m_listZones.count();

    for (qint32 i = 0; i < nNumberOfZones; i++) {
        m_listZones[i].bVisible = false;
    }

    update();
}

bool XPlotWidget::saveToFile(const QString &sFileName, qint32 nWidth, qint32 nHeight, qint32 nQuality)
{
    bool bResult = false;

    if ((nWidth > 0) && (nHeight > 0)) {
        QImage image(nWidth, nHeight, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);

        QPainter painter(&image);
        drawPlot(&painter, QRectF(0, 0, nWidth, nHeight), false, true);
        painter.end();

        bResult = image.save(sFileName, nullptr, nQuality);
    }

    return bResult;
}

QSize XPlotWidget::sizeHint() const
{
    return QSize(400, 200);
}

QSize XPlotWidget::minimumSizeHint() const
{
    return QSize(100, 50);
}

void XPlotWidget::paintEvent(QPaintEvent *pEvent)
{
    Q_UNUSED(pEvent)

    QPainter painter(this);
    drawPlot(&painter, QRectF(rect()), m_bTrackerVisible && m_bMouseInside, false);
}

void XPlotWidget::mouseMoveEvent(QMouseEvent *pEvent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_ptCursor = pEvent->position();
#else
    m_ptCursor = pEvent->localPos();
#endif
    m_bMouseInside = true;

    if (m_bTrackerVisible) {
        update();
    }

    QWidget::mouseMoveEvent(pEvent);
}

void XPlotWidget::leaveEvent(QEvent *pEvent)
{
    m_bMouseInside = false;

    if (m_bTrackerVisible) {
        update();
    }

    QWidget::leaveEvent(pEvent);
}

double XPlotWidget::niceStep(double dRange, qint32 nMaxIntervals)
{
    double dResult = 1.0;

    if ((dRange > 0) && (nMaxIntervals > 0)) {
        double dRawStep = dRange / nMaxIntervals;
        double dPower = std::pow(10.0, std::floor(std::log10(dRawStep)));
        double dNorm = dRawStep / dPower;

        double dFactor = 10.0;

        if (dNorm <= 1.0) {
            dFactor = 1.0;
        } else if (dNorm <= 2.0) {
            dFactor = 2.0;
        } else if (dNorm <= 5.0) {
            dFactor = 5.0;
        }

        dResult = dFactor * dPower;
    }

    return dResult;
}

QList<double> XPlotWidget::generateTicks(double dMin, double dMax, double dStep)
{
    QList<double> listResult;

    if ((dStep > 0) && (dMax > dMin)) {
        // Iterate by integer tick index: no int overflow (std::ceil/floor keep double),
        // no floating-point accumulation drift, no infinite loop when dStep < ULP(value).
        double dFirstIndex = std::ceil((dMin - dStep * 0.001) / dStep);
        double dLastIndex = std::floor((dMax + dStep * 0.001) / dStep);
        double dNumberOfTicks = dLastIndex - dFirstIndex + 1;

        if ((dNumberOfTicks > 0) && (dNumberOfTicks <= 1000)) {
            qint32 nNumberOfTicks = (qint32)dNumberOfTicks;

            for (qint32 i = 0; i < nNumberOfTicks; i++) {
                listResult.append((dFirstIndex + i) * dStep);
            }
        }
    }

    return listResult;
}

#ifdef _MSC_VER
// MSVC 14.44 /O2 miscompiles the auto-scale min/max path when these functions are
// inlined into drawPlot (axis collapses to 0..1); keep them out of the optimizer.
#pragma optimize("", off)
#endif
XPlotWidget::SCALE XPlotWidget::resolveScaleX() const
{
    SCALE result = {};

    if (m_axisX.bManual) {
        result.dMin = m_axisX.dMin;
        result.dMax = m_axisX.dMax;
        result.dStep = m_axisX.dStep;
    } else if (m_vecCurveX.count() > 0) {
        // Plain-comparison scan over constData: the qMin/qMax reference-select form
        // miscompiles under MSVC 14.44 /O2 in this translation unit (wrong axis range)
        const double *pData = m_vecCurveX.constData();
        qint32 nNumberOfPoints = (qint32)m_vecCurveX.count();

        result.dMin = pData[0];
        result.dMax = pData[0];

        for (qint32 i = 1; i < nNumberOfPoints; i++) {
            if (pData[i] < result.dMin) {
                result.dMin = pData[i];
            }
            if (pData[i] > result.dMax) {
                result.dMax = pData[i];
            }
        }
    } else if (m_vecHistValues.count() > 0) {
        result.dMin = m_dHistStart;
        result.dMax = m_dHistStart + m_dHistInterval * m_vecHistValues.count();
    } else {
        result.dMin = 0;
        result.dMax = 1000;
    }

    if (result.dMax <= result.dMin) {
        result.dMax = result.dMin + 1.0;
    }

    if (result.dStep <= 0) {
        result.dStep = niceStep(result.dMax - result.dMin, 7);
    }

    if (!m_axisX.bManual) {
        result.dMin = std::floor(result.dMin / result.dStep) * result.dStep;
        result.dMax = std::ceil(result.dMax / result.dStep) * result.dStep;
    }

    return result;
}

XPlotWidget::SCALE XPlotWidget::resolveScaleY() const
{
    SCALE result = {};

    if (m_axisY.bManual) {
        result.dMin = m_axisY.dMin;
        result.dMax = m_axisY.dMax;
        result.dStep = m_axisY.dStep;
    } else if (m_vecCurveY.count() > 0) {
        // Plain-comparison scan over constData: see resolveScaleX
        const double *pData = m_vecCurveY.constData();
        qint32 nNumberOfPoints = (qint32)m_vecCurveY.count();

        result.dMin = pData[0];
        result.dMax = pData[0];

        for (qint32 i = 1; i < nNumberOfPoints; i++) {
            if (pData[i] < result.dMin) {
                result.dMin = pData[i];
            }
            if (pData[i] > result.dMax) {
                result.dMax = pData[i];
            }
        }
    } else if (m_vecHistValues.count() > 0) {
        const double *pData = m_vecHistValues.constData();
        qint32 nNumberOfValues = (qint32)m_vecHistValues.count();

        result.dMin = 0;
        result.dMax = pData[0];

        for (qint32 i = 0; i < nNumberOfValues; i++) {
            if (pData[i] < result.dMin) {
                result.dMin = pData[i];
            }
            if (pData[i] > result.dMax) {
                result.dMax = pData[i];
            }
        }
    } else {
        result.dMin = 0;
        result.dMax = 1000;
    }

    if (result.dMax <= result.dMin) {
        result.dMax = result.dMin + 1.0;
    }

    if (result.dStep <= 0) {
        result.dStep = niceStep(result.dMax - result.dMin, 8);
    }

    if (!m_axisY.bManual) {
        result.dMin = std::floor(result.dMin / result.dStep) * result.dStep;
        result.dMax = std::ceil(result.dMax / result.dStep) * result.dStep;
    }

    return result;
}

#ifdef _MSC_VER
#pragma optimize("", on)
#endif

QString XPlotWidget::formatValue(double dValue) const
{
    if (qAbs(dValue) < 1.0e-12) {
        dValue = 0;
    }

    return locale().toString(dValue, 'g', 6);
}

double XPlotWidget::mapValue(double dValue, double dMin, double dMax, double dPixelBegin, double dPixelEnd)
{
    double dResult = dPixelBegin;

    if (dMax > dMin) {
        dResult = dPixelBegin + ((dValue - dMin) / (dMax - dMin)) * (dPixelEnd - dPixelBegin);
    }

    return dResult;
}

void XPlotWidget::drawPlot(QPainter *pPainter, const QRectF &rectWidget, bool bTracker, bool bDocument)
{
    SCALE scaleX = resolveScaleX();
    SCALE scaleY = resolveScaleY();

    QFontMetricsF fontMetrics(font());

    const double dTickLength = 5;
    const double dLabelGap = 3;

    // Collect tick positions
    QList<double> listTicksX = generateTicks(scaleX.dMin, scaleX.dMax, scaleX.dStep);
    QList<double> listTicksY = generateTicks(scaleY.dMin, scaleY.dMax, scaleY.dStep);
    QList<double> listMinorTicksX = generateTicks(scaleX.dMin, scaleX.dMax, scaleX.dStep / 5);
    QList<double> listMinorTicksY = generateTicks(scaleY.dMin, scaleY.dMax, scaleY.dStep / 5);

    // Margins
    double dMaxLabelWidthY = 0;

    for (qint32 i = 0; i < listTicksY.count(); i++) {
        dMaxLabelWidthY = qMax(dMaxLabelWidthY, fontMetrics.horizontalAdvance(formatValue(listTicksY.at(i))));
    }

    double dLastLabelWidthX = 0;

    if (listTicksX.count() > 0) {
        dLastLabelWidthX = fontMetrics.horizontalAdvance(formatValue(listTicksX.at(listTicksX.count() - 1)));
    }

    double dMarginLeft = dMaxLabelWidthY + dTickLength + dLabelGap + 2;
    double dMarginBottom = fontMetrics.height() + dTickLength + dLabelGap + 2;
    double dMarginTop = fontMetrics.height() / 2 + 2;
    double dMarginRight = dLastLabelWidthX / 2 + 4;

    QRectF rectCanvas(rectWidget.left() + dMarginLeft, rectWidget.top() + dMarginTop, rectWidget.width() - dMarginLeft - dMarginRight,
                      rectWidget.height() - dMarginTop - dMarginBottom);

    if ((rectCanvas.width() < 10) || (rectCanvas.height() < 10)) {
        return;
    }

    QColor colorText;
    QColor colorCanvas;
    QColor colorFrame;

    if (bDocument) {
        // Fixed light colors for exported images, independent of the application theme
        colorText = Qt::black;
        colorCanvas = Qt::white;
        colorFrame = Qt::gray;
    } else {
        colorText = palette().color(QPalette::WindowText);
        colorCanvas = palette().color(QPalette::Base);
        colorFrame = palette().color(QPalette::Mid);
    }

    // Canvas background
    pPainter->fillRect(rectCanvas, colorCanvas);

    // Grid
    if (m_bGridVisible) {
        QColor colorGridMajor = colorFrame;
        QColor colorGridMinor = colorFrame;
        colorGridMinor.setAlpha(100);

        QPen penGridMinor(colorGridMinor);
        penGridMinor.setStyle(Qt::DotLine);
        pPainter->setPen(penGridMinor);

        for (qint32 i = 0; i < listMinorTicksX.count(); i++) {
            double dPixelX = mapValue(listMinorTicksX.at(i), scaleX.dMin, scaleX.dMax, rectCanvas.left(), rectCanvas.right());
            pPainter->drawLine(QPointF(dPixelX, rectCanvas.top()), QPointF(dPixelX, rectCanvas.bottom()));
        }

        QPen penGridMajor(colorGridMajor);
        penGridMajor.setStyle(Qt::DotLine);
        pPainter->setPen(penGridMajor);

        for (qint32 i = 0; i < listTicksX.count(); i++) {
            double dPixelX = mapValue(listTicksX.at(i), scaleX.dMin, scaleX.dMax, rectCanvas.left(), rectCanvas.right());
            pPainter->drawLine(QPointF(dPixelX, rectCanvas.top()), QPointF(dPixelX, rectCanvas.bottom()));
        }

        for (qint32 i = 0; i < listTicksY.count(); i++) {
            double dPixelY = mapValue(listTicksY.at(i), scaleY.dMin, scaleY.dMax, rectCanvas.bottom(), rectCanvas.top());
            pPainter->drawLine(QPointF(rectCanvas.left(), dPixelY), QPointF(rectCanvas.right(), dPixelY));
        }
    }

    // Histogram
    if (m_vecHistValues.count() > 0) {
        pPainter->save();
        pPainter->setClipRect(rectCanvas);

        double dBaseValue = qBound(scaleY.dMin, 0.0, scaleY.dMax);
        double dPixelBase = mapValue(dBaseValue, scaleY.dMin, scaleY.dMax, rectCanvas.bottom(), rectCanvas.top());

        qint32 nNumberOfValues = m_vecHistValues.count();

        for (qint32 i = 0; i < nNumberOfValues; i++) {
            double dValueBegin = m_dHistStart + i * m_dHistInterval;
            double dValueEnd = dValueBegin + m_dHistInterval;

            double dPixelX1 = mapValue(dValueBegin, scaleX.dMin, scaleX.dMax, rectCanvas.left(), rectCanvas.right());
            double dPixelX2 = mapValue(dValueEnd, scaleX.dMin, scaleX.dMax, rectCanvas.left(), rectCanvas.right());
            double dPixelY = mapValue(m_vecHistValues.at(i), scaleY.dMin, scaleY.dMax, rectCanvas.bottom(), rectCanvas.top());

            double dBarWidth = qMax(dPixelX2 - dPixelX1 - 1.0, 1.0);

            QRectF rectBar(dPixelX1, qMin(dPixelY, dPixelBase), dBarWidth, qAbs(dPixelBase - dPixelY));
            pPainter->fillRect(rectBar, m_colorHistogram);
        }

        pPainter->restore();
    }

    // Curve
    if (m_vecCurveX.count() > 1) {
        pPainter->save();
        pPainter->setClipRect(rectCanvas);
        pPainter->setRenderHint(QPainter::Antialiasing, true);
        pPainter->setPen(QPen(m_colorCurve));

        qint32 nNumberOfPoints = m_vecCurveX.count();

        QPolygonF polygon;
        polygon.reserve(nNumberOfPoints);

        for (qint32 i = 0; i < nNumberOfPoints; i++) {
            double dPixelX = mapValue(m_vecCurveX.at(i), scaleX.dMin, scaleX.dMax, rectCanvas.left(), rectCanvas.right());
            double dPixelY = mapValue(m_vecCurveY.at(i), scaleY.dMin, scaleY.dMax, rectCanvas.bottom(), rectCanvas.top());
            polygon.append(QPointF(dPixelX, dPixelY));
        }

        pPainter->drawPolyline(polygon);
        pPainter->restore();
    }

    // Zones
    {
        qint32 nNumberOfZones = m_listZones.count();

        for (qint32 i = 0; i < nNumberOfZones; i++) {
            if (m_listZones.at(i).bVisible) {
                double dPixelX1 = mapValue(m_listZones.at(i).dBegin, scaleX.dMin, scaleX.dMax, rectCanvas.left(), rectCanvas.right());
                double dPixelX2 = mapValue(m_listZones.at(i).dEnd, scaleX.dMin, scaleX.dMax, rectCanvas.left(), rectCanvas.right());

                pPainter->save();
                pPainter->setClipRect(rectCanvas);

                QRectF rectZone(dPixelX1, rectCanvas.top(), dPixelX2 - dPixelX1, rectCanvas.height());
                pPainter->fillRect(rectZone, m_listZones.at(i).colorBrush);

                pPainter->setPen(QPen(m_listZones.at(i).colorPen));
                pPainter->drawLine(QPointF(dPixelX1, rectCanvas.top()), QPointF(dPixelX1, rectCanvas.bottom()));
                pPainter->drawLine(QPointF(dPixelX2, rectCanvas.top()), QPointF(dPixelX2, rectCanvas.bottom()));

                pPainter->restore();
            }
        }
    }

    // Frame
    pPainter->setPen(QPen(colorFrame));
    pPainter->drawRect(rectCanvas);

    // Axis ticks and labels
    pPainter->setPen(QPen(colorText));

    for (qint32 i = 0; i < listTicksX.count(); i++) {
        double dPixelX = mapValue(listTicksX.at(i), scaleX.dMin, scaleX.dMax, rectCanvas.left(), rectCanvas.right());
        pPainter->drawLine(QPointF(dPixelX, rectCanvas.bottom()), QPointF(dPixelX, rectCanvas.bottom() + dTickLength));

        QString sLabel = formatValue(listTicksX.at(i));
        double dLabelWidth = fontMetrics.horizontalAdvance(sLabel);
        pPainter->drawText(QPointF(dPixelX - dLabelWidth / 2, rectCanvas.bottom() + dTickLength + dLabelGap + fontMetrics.ascent()), sLabel);
    }

    for (qint32 i = 0; i < listMinorTicksX.count(); i++) {
        double dPixelX = mapValue(listMinorTicksX.at(i), scaleX.dMin, scaleX.dMax, rectCanvas.left(), rectCanvas.right());
        pPainter->drawLine(QPointF(dPixelX, rectCanvas.bottom()), QPointF(dPixelX, rectCanvas.bottom() + dTickLength / 2));
    }

    for (qint32 i = 0; i < listTicksY.count(); i++) {
        double dPixelY = mapValue(listTicksY.at(i), scaleY.dMin, scaleY.dMax, rectCanvas.bottom(), rectCanvas.top());
        pPainter->drawLine(QPointF(rectCanvas.left() - dTickLength, dPixelY), QPointF(rectCanvas.left(), dPixelY));

        QString sLabel = formatValue(listTicksY.at(i));
        double dLabelWidth = fontMetrics.horizontalAdvance(sLabel);
        pPainter->drawText(QPointF(rectCanvas.left() - dTickLength - dLabelGap - dLabelWidth, dPixelY + fontMetrics.ascent() / 2 - 1), sLabel);
    }

    for (qint32 i = 0; i < listMinorTicksY.count(); i++) {
        double dPixelY = mapValue(listMinorTicksY.at(i), scaleY.dMin, scaleY.dMax, rectCanvas.bottom(), rectCanvas.top());
        pPainter->drawLine(QPointF(rectCanvas.left() - dTickLength / 2, dPixelY), QPointF(rectCanvas.left(), dPixelY));
    }

    // Tracker
    if (bTracker && rectCanvas.contains(m_ptCursor)) {
        pPainter->save();
        pPainter->setClipRect(rectCanvas);

        pPainter->setPen(QPen(Qt::green));
        pPainter->drawLine(QPointF(rectCanvas.left(), m_ptCursor.y()), QPointF(rectCanvas.right(), m_ptCursor.y()));
        pPainter->drawLine(QPointF(m_ptCursor.x(), rectCanvas.top()), QPointF(m_ptCursor.x(), rectCanvas.bottom()));

        double dValueX = scaleX.dMin + ((m_ptCursor.x() - rectCanvas.left()) / rectCanvas.width()) * (scaleX.dMax - scaleX.dMin);
        double dValueY = scaleY.dMin + ((rectCanvas.bottom() - m_ptCursor.y()) / rectCanvas.height()) * (scaleY.dMax - scaleY.dMin);

        QString sTracker = QString("%1, %2").arg(formatValue(dValueX), formatValue(dValueY));

        double dTextWidth = fontMetrics.horizontalAdvance(sTracker);
        double dTextX = m_ptCursor.x() + 6;
        double dTextY = m_ptCursor.y() - 6;

        if ((dTextX + dTextWidth) > rectCanvas.right()) {
            dTextX = m_ptCursor.x() - 6 - dTextWidth;
        }

        if ((dTextY - fontMetrics.height()) < rectCanvas.top()) {
            dTextY = m_ptCursor.y() + 6 + fontMetrics.ascent();
        }

        pPainter->setPen(QPen(Qt::red));
        pPainter->drawText(QPointF(dTextX, dTextY), sTracker);

        pPainter->restore();
    }
}
