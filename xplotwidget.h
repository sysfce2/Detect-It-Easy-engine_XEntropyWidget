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
#ifndef XPLOTWIDGET_H
#define XPLOTWIDGET_H

#include <QImage>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QVector>
#include <QWidget>

class XPlotWidget : public QWidget {
    Q_OBJECT

public:
    struct ZONE {
        double dBegin;
        double dEnd;
        QColor colorPen;
        QColor colorBrush;
        bool bVisible;
    };

    explicit XPlotWidget(QWidget *pParent = nullptr);

    void setAxisScaleX(double dMin, double dMax, double dStep = 0);
    void setAxisScaleY(double dMin, double dMax, double dStep = 0);
    void setCurveData(const QVector<double> &vecX, const QVector<double> &vecY);
    void setCurveColor(const QColor &color);
    void setHistogramData(const QVector<double> &vecValues, double dStart, double dInterval);
    void setHistogramColor(const QColor &color);
    void setGridVisible(bool bVisible);
    void setTrackerVisible(bool bVisible);
    void setZones(const QList<ZONE> &listZones);
    void setZoneVisible(qint32 nIndex, bool bVisible);
    void clearZonesVisible();
    bool saveToFile(const QString &sFileName, qint32 nWidth = 1004, qint32 nHeight = 669, qint32 nQuality = 85);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

protected:
    virtual void paintEvent(QPaintEvent *pEvent);
    virtual void mouseMoveEvent(QMouseEvent *pEvent);
    virtual void leaveEvent(QEvent *pEvent);

private:
    struct AXIS {
        bool bManual;
        double dMin;
        double dMax;
        double dStep;
    };

    struct SCALE {
        double dMin;
        double dMax;
        double dStep;
    };

    static double niceStep(double dRange, qint32 nMaxIntervals);
    static QList<double> generateTicks(double dMin, double dMax, double dStep);
    SCALE resolveScaleX() const;
    SCALE resolveScaleY() const;
    QString formatValue(double dValue) const;
    static double mapValue(double dValue, double dMin, double dMax, double dPixelBegin, double dPixelEnd);
    void drawPlot(QPainter *pPainter, const QRectF &rectWidget, bool bTracker, bool bDocument);

    AXIS m_axisX;
    AXIS m_axisY;
    QVector<double> m_vecCurveX;
    QVector<double> m_vecCurveY;
    QColor m_colorCurve;
    QVector<double> m_vecHistValues;
    double m_dHistStart;
    double m_dHistInterval;
    QColor m_colorHistogram;
    QList<ZONE> m_listZones;
    bool m_bGridVisible;
    bool m_bTrackerVisible;
    bool m_bMouseInside;
    QPointF m_ptCursor;
};

#endif  // XPLOTWIDGET_H
