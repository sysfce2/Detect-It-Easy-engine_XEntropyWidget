/* Copyright (c) 2020-2026 hors<horsicq@gmail.com>
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
#include "xentropywidget.h"

#include "ui_xentropywidget.h"

class XPercentageWidgetItem : public QTableWidgetItem  // TODO move to Controls !!!
{
public:
    bool operator<(const QTableWidgetItem &other) const
    {
        return text().toDouble() < other.text().toDouble();
    }
};

XEntropyWidget::XEntropyWidget(QWidget *pParent) : XShortcutsWidget(pParent), ui(new Ui::XEntropyWidget)
{
    ui->setupUi(this);

    XOptions::adjustToolButton(ui->toolButtonReload, XOptions::ICONTYPE_RELOAD);
    XOptions::adjustToolButton(ui->toolButtonSaveEntropyTable, XOptions::ICONTYPE_SAVE);
    XOptions::adjustToolButton(ui->toolButtonSaveEntropyDiagram, XOptions::ICONTYPE_SAVE);

    ui->comboBoxType->setToolTip(tr("Type"));
    ui->comboBoxMapMode->setToolTip(tr("Mode"));
    ui->spinBoxCount->setToolTip(tr("Count"));
    ui->lineEditPartSize->setToolTip(tr("Size"));
    ui->lineEditTotalEntropy->setToolTip(tr("Total"));
    ui->lineEditOffset->setToolTip(tr("Offset"));
    ui->lineEditSize->setToolTip(tr("Size"));
    ui->progressBarTotalEntropy->setToolTip(tr("Total"));
    ui->toolButtonReload->setToolTip(tr("Reload"));
    ui->toolButtonSaveEntropyTable->setToolTip(tr("Save"));
    ui->toolButtonSaveEntropyDiagram->setToolTip(tr("Save"));
    ui->checkBoxGridRegions->setToolTip(tr("Grid"));
    ui->tableWidgetBytes->setToolTip(tr("Bytes"));
    ui->tableViewRegions->setToolTip(tr("Regions"));
    ui->widgetEntropy->setToolTip(tr("Entropy"));
    ui->widgetBytes->setToolTip(tr("Bytes"));

    m_entropyData = {};

    m_inData = {};
    m_nOffset = 0;
    m_nSize = 0;

    ui->widgetEntropy->setCurveColor(Qt::red);
    ui->widgetEntropy->setAxisScaleY(0, 8);  // Fix
    ui->widgetEntropy->setTrackerVisible(true);

    ui->widgetBytes->setHistogramColor(Qt::blue);
    ui->widgetBytes->setAxisScaleX(0, 256, 32);

    ui->tabWidget->setCurrentIndex(0);

    ui->spinBoxCount->setValue(100);
}

XEntropyWidget::~XEntropyWidget()
{
    XFormats::removeDevice(m_inData.pDevice, m_inData);
    delete ui;
}

void XEntropyWidget::setData(const XBinary::INDATA &inData, qint64 nOffset, qint64 nSize, bool bAuto)
{
    XFormats::removeDevice(m_inData.pDevice, m_inData);
    m_inData = inData;
    m_inData.pDevice = XFormats::createDevice(inData);
    m_nOffset = nOffset;
    m_nSize = nSize;

    if ((m_nSize == -1) && m_inData.pDevice) {
        m_nSize = (m_inData.pDevice->size()) - (this->m_nOffset);
    }

    //    m_entropyData.nOffset=0; // We are using subdevice. Offset is always 0.
    //    m_entropyData.nSize=this->m_nSize;
    m_entropyData.nOffset = nOffset;
    m_entropyData.nSize = m_nSize;

    if ((m_inData.fileType != XBinary::FT_REGION) && m_inData.pDevice) {
        SubDevice subDevice(m_inData.pDevice, m_nOffset, m_nSize);

        if (subDevice.open(QIODevice::ReadOnly)) {
            m_entropyData.fileType = XFormats::setFileTypeComboBox(m_inData.fileType, &subDevice, ui->comboBoxType);
            m_entropyData.mapMode = XFormats::getMapModesList(m_inData.fileType, ui->comboBoxMapMode);

            subDevice.close();
        }
    } else if (m_inData.fileType == XBinary::FT_REGION) {
        ui->comboBoxType->addItem(XBinary::fileTypeIdToString(m_inData.fileType), m_inData.fileType);
    }

    qint64 nCount = m_nSize / 0x200;  // TODO const

    nCount = qMin(nCount, (qint64)100);

    if (nCount) {
        ui->spinBoxCount->setValue(nCount);
    } else {
        ui->spinBoxCount->setValue(1);
    }

    adjust();

    if (bAuto) {
        reload(true, true);
    }
}

void XEntropyWidget::setData(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::FT fileType, bool bAuto)
{
    setData(XFormats::createINDATA(fileType, pDevice), nOffset, nSize, bAuto);
}

void XEntropyWidget::setSaveDirectory(const QString &sSaveDirectory)
{
    this->m_sSaveDirectory = sSaveDirectory;
}

void XEntropyWidget::reload(bool bGraph, bool bRegions)
{
    // TODO TableWidget -> TableView
    if (m_inData.pDevice) {
        m_entropyData.fileType = (XBinary::FT)(ui->comboBoxType->currentData().toInt());
        m_entropyData.mapMode = (XBinary::MAPMODE)(ui->comboBoxMapMode->currentData().toInt());

        EntropyProcess entropyProcess;

        XDialogProcess dep(XOptions::getMainWidget(this), &entropyProcess);
        dep.setGlobal(getShortcuts(), getGlobalOptions());
        entropyProcess.setData(m_inData.pDevice, &m_entropyData, bGraph, bRegions, ui->spinBoxCount->value(), dep.getPdStruct());
        dep.start();
        dep.showDialogDelay();

        if (dep.isSuccess()) {
            if (bGraph) {
                ui->lineEditTotalEntropy->setText(XBinary::doubleToString(m_entropyData.dTotalEntropy, 5));

                ui->progressBarTotalEntropy->setMaximum(8 * 100);
                ui->progressBarTotalEntropy->setValue(m_entropyData.dTotalEntropy * 100);

                ui->lineEditOffset->setValue32_64(m_nOffset);
                ui->lineEditSize->setValue32_64(m_nSize);
                ui->progressBarTotalEntropy->setFormat(m_entropyData.sStatus + "(%p%)");

                qint32 nNumberOfEntropies = m_entropyData.listEntropies.count();

                QVector<double> vecOffsets(nNumberOfEntropies);
                QVector<double> vecEntropies(nNumberOfEntropies);

                for (qint32 i = 0; i < nNumberOfEntropies; i++) {
                    vecOffsets[i] = m_entropyData.listEntropies.at(i).dOffset;
                    vecEntropies[i] = m_entropyData.listEntropies.at(i).dEntropy;
                }

                ui->widgetEntropy->setCurveData(vecOffsets, vecEntropies);

                ui->tableWidgetBytes->clear();

                ui->tableWidgetBytes->setRowCount(256);
                ui->tableWidgetBytes->setColumnCount(3);

                QStringList listHeaders;
                listHeaders.append(tr("Byte"));
                listHeaders.append(tr("Count"));
                listHeaders.append(QString("%"));

                ui->tableWidgetBytes->setHorizontalHeaderLabels(listHeaders);
                ui->tableWidgetBytes->horizontalHeader()->setVisible(true);

                for (qint32 i = 0; i < 256; i++) {
                    QTableWidgetItem *pItemByte = new QTableWidgetItem;

                    pItemByte->setText(QString("0x%1").arg(i, 2, 16, QChar('0')));
                    pItemByte->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    ui->tableWidgetBytes->setItem(i, 0, pItemByte);

                    QTableWidgetItem *pItemCount = new QTableWidgetItem;

                    pItemCount->setData(Qt::DisplayRole, m_entropyData.byteCounts.nCount[i]);
                    pItemCount->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    ui->tableWidgetBytes->setItem(i, 1, pItemCount);

                    XPercentageWidgetItem *pItemPercentage = new XPercentageWidgetItem;

                    // nSize is 0 for an empty region/file; guard against 0.0/0 -> NaN ("nan" in every row).
                    double dPercentage = m_entropyData.byteCounts.nSize ? ((double)m_entropyData.byteCounts.nCount[i] * 100) / m_entropyData.byteCounts.nSize : 0.0;
                    pItemPercentage->setText(XBinary::doubleToString(dPercentage, 4));

                    pItemPercentage->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    ui->tableWidgetBytes->setItem(i, 2, pItemPercentage);
                }

                ui->tableWidgetBytes->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
                ui->tableWidgetBytes->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
                ui->tableWidgetBytes->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

                // TODO Size 0,2 columns !!!

                QVector<double> vecCounts(256);

                for (qint32 i = 0; i < 256; i++) {
                    vecCounts[i] = m_entropyData.byteCounts.nCount[i];
                }

                ui->widgetBytes->setHistogramData(vecCounts, 0, 1.0);
            }

            if (bRegions) {
                QList<XPlotWidget::ZONE> listZones;

                qint32 nNumberOfMemoryRecords = m_entropyData.listMemoryRecords.count();

                QStandardItemModel *pModel = new QStandardItemModel(nNumberOfMemoryRecords, 5);

                pModel->setHeaderData(0, Qt::Horizontal, tr("Offset"));
                pModel->setHeaderData(1, Qt::Horizontal, tr("Size"));
                pModel->setHeaderData(2, Qt::Horizontal, tr("Entropy"));
                pModel->setHeaderData(3, Qt::Horizontal, tr("Status"));
                pModel->setHeaderData(4, Qt::Horizontal, tr("Name"));

                for (qint32 i = 0; i < nNumberOfMemoryRecords; i++) {
                    QStandardItem *pItemOffset = new QStandardItem;

                    pItemOffset->setData(m_entropyData.listMemoryRecords.at(i).nOffset, Qt::UserRole + 0);
                    pItemOffset->setData(m_entropyData.listMemoryRecords.at(i).nSize, Qt::UserRole + 1);

                    pItemOffset->setText(XLineEditHEX::getFormatString(m_entropyData.mode, m_entropyData.listMemoryRecords.at(i).nOffset + m_nOffset));
                    pModel->setItem(i, 0, pItemOffset);

                    QStandardItem *pItemSize = new QStandardItem;

                    pItemSize->setText(XLineEditHEX::getFormatString(m_entropyData.mode, m_entropyData.listMemoryRecords.at(i).nSize));
                    pModel->setItem(i, 1, pItemSize);

                    QStandardItem *pItemEntropy = new QStandardItem;

                    pItemEntropy->setText(XBinary::doubleToString(m_entropyData.listMemoryRecords.at(i).dEntropy, 5));
                    pModel->setItem(i, 2, pItemEntropy);

                    QStandardItem *pItemStatus = new QStandardItem;

                    pItemStatus->setText(m_entropyData.listMemoryRecords.at(i).sStatus);
                    pModel->setItem(i, 3, pItemStatus);

                    QStandardItem *pItemName = new QStandardItem;

                    pItemName->setText(m_entropyData.listMemoryRecords.at(i).sName);

                    pModel->setItem(i, 4, pItemName);

                    XPlotWidget::ZONE zone = {};
                    zone.dBegin = m_entropyData.listMemoryRecords.at(i).nOffset;
                    zone.dEnd = m_entropyData.listMemoryRecords.at(i).nOffset + m_entropyData.listMemoryRecords.at(i).nSize;
                    zone.bVisible = false;
                    QColor color = Qt::darkBlue;
                    color.setAlpha(100);
                    zone.colorPen = color;
                    color.setAlpha(20);
                    zone.colorBrush = color;
                    listZones.append(zone);
                }

                ui->widgetEntropy->setZones(listZones);

                XOptions::setModelTextAlignment(pModel, 0, Qt::AlignRight | Qt::AlignVCenter);
                XOptions::setModelTextAlignment(pModel, 1, Qt::AlignRight | Qt::AlignVCenter);
                XOptions::setModelTextAlignment(pModel, 2, Qt::AlignRight | Qt::AlignVCenter);
                XOptions::setModelTextAlignment(pModel, 3, Qt::AlignLeft | Qt::AlignVCenter);
                XOptions::setModelTextAlignment(pModel, 4, Qt::AlignLeft | Qt::AlignVCenter);

                ui->tableViewRegions->setCustomModel(pModel, true);

                ui->tableViewRegions->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
                ui->tableViewRegions->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
                ui->tableViewRegions->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
                ui->tableViewRegions->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
                ui->tableViewRegions->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

                qint32 nColumnSize = XLineEditHEX::getWidthFromMode(this, m_entropyData.mode);

                ui->tableViewRegions->setColumnWidth(0, nColumnSize);
                ui->tableViewRegions->setColumnWidth(1, nColumnSize);

                connect(ui->tableViewRegions->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
                        SLOT(on_tableViewSelection(QItemSelection, QItemSelection)));
            }
        }
    }
}

void XEntropyWidget::adjustView()
{
    getGlobalOptions()->adjustWidget(this, XOptions::ID_VIEW_FONT_CONTROLS);
    getGlobalOptions()->adjustTableView(ui->tableViewRegions, XOptions::ID_VIEW_FONT_TABLEVIEWS);
    getGlobalOptions()->adjustTableView(ui->tableWidgetBytes, XOptions::ID_VIEW_FONT_TABLEVIEWS);
}

void XEntropyWidget::reloadData(bool bSaveSelection)
{
    Q_UNUSED(bSaveSelection)
    reload(true, true);
}

void XEntropyWidget::on_toolButtonReload_clicked()
{
    reload(true, true);
}

void XEntropyWidget::on_comboBoxType_currentIndexChanged(int nIndex)
{
    Q_UNUSED(nIndex)

    XBinary::FT fileType = (XBinary::FT)(ui->comboBoxType->currentData().toInt());
    XFormats::getMapModesList(fileType, ui->comboBoxMapMode);

    reload(false, true);
}

void XEntropyWidget::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
    // TODO !!!
    // XShortcutsWidget::registerShortcuts(bState);
}

void XEntropyWidget::on_toolButtonSaveEntropyTable_clicked()
{
    QString sResultFileName = XBinary::getResultFileName(m_inData.pDevice, QString("%1.txt").arg(tr("Entropy")));

    QAbstractItemModel *pModel = nullptr;

    if (ui->tabWidget->currentIndex() == 0) {
        pModel = ui->tableViewRegions->getProxyModel();
    } else {
        pModel = ui->tableWidgetBytes->model();
    }

    XShortcutsWidget::saveTableModel(pModel, sResultFileName);
}

void XEntropyWidget::on_toolButtonSaveEntropyDiagram_clicked()
{
    QString sFilter = XOptions::getImageFilter();
    QString sFileName = XBinary::getResultFileName(m_inData.pDevice, QString("%1.png").arg(tr("Entropy")));

    sFileName = QFileDialog::getSaveFileName(this, tr("Save diagram"), sFileName, sFilter);

    if (!sFileName.isEmpty()) {
        XPlotWidget *pWidget = nullptr;

        if (ui->tabWidget->currentIndex() == 0) {
            pWidget = ui->widgetEntropy;
        } else {
            pWidget = ui->widgetBytes;
        }

        pWidget->saveToFile(sFileName);
    }
}

void XEntropyWidget::on_spinBoxCount_valueChanged(int nValue)
{
    Q_UNUSED(nValue)

    adjust();
}

void XEntropyWidget::adjust()
{
    qint32 nValue = ui->spinBoxCount->value();

    if (nValue) {
        ui->lineEditPartSize->setValue32_64(m_nSize / nValue);
    } else {
        ui->lineEditPartSize->setValue_uint32((quint32)0);
    }
}

void XEntropyWidget::on_checkBoxGridRegions_toggled(bool bChecked)
{
    ui->widgetEntropy->setGridVisible(bChecked);
}

void XEntropyWidget::on_tableViewSelection(const QItemSelection &itemSelected, const QItemSelection &itemDeselected)
{
    Q_UNUSED(itemSelected)
    Q_UNUSED(itemDeselected)

    ui->widgetEntropy->clearZonesVisible();

    QItemSelectionModel *pSelectionModel = ui->tableViewRegions->selectionModel();

    if (pSelectionModel) {
        QModelIndexList listIndexes = pSelectionModel->selectedIndexes();

        qint32 nNumberOfRecords = listIndexes.count();

        for (qint32 i = 0; i < nNumberOfRecords; i++) {
            if (listIndexes.at(i).column() == 0) {
                // The view selects proxy indexes; zones are in source-model order
                QModelIndex indexSource = ui->tableViewRegions->getProxyModel()->mapToSource(listIndexes.at(i));
                ui->widgetEntropy->setZoneVisible(indexSource.row(), true);
            }
        }
    }
}

void XEntropyWidget::on_tableViewRegions_customContextMenuRequested(const QPoint &pos)
{
    qint32 nRow = ui->tableViewRegions->currentIndex().row();

    if (nRow != -1) {
        QMenu contextMenu(this);

        QList<XShortcuts::MENUITEM> listMenuItems;

        getShortcuts()->_addMenuItem_CopyRow(&listMenuItems, ui->tableViewRegions);

        getShortcuts()->adjustContextMenu(&contextMenu, &listMenuItems);

        contextMenu.exec(ui->tableViewRegions->viewport()->mapToGlobal(pos));
    }
}

void XEntropyWidget::on_tableWidgetBytes_customContextMenuRequested(const QPoint &pos)
{
    qint32 nRow = ui->tableWidgetBytes->currentIndex().row();

    if (nRow != -1) {
        QMenu contextMenu(this);

        QList<XShortcuts::MENUITEM> listMenuItems;

        getShortcuts()->_addMenuItem_CopyRow(&listMenuItems, ui->tableWidgetBytes);

        getShortcuts()->adjustContextMenu(&contextMenu, &listMenuItems);

        contextMenu.exec(ui->tableWidgetBytes->viewport()->mapToGlobal(pos));
    }
}

void XEntropyWidget::on_comboBoxMapMode_currentIndexChanged(int nIndex)
{
    Q_UNUSED(nIndex)

    reload(false, true);
}
