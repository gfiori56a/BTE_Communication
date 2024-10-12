#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStatusBar>
/*
arduino server data:
BLEService            ledService("19B10010-E8F2-537E-4F6C-D104768A1214");
BLEByteCharacteristic ledCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
BLEByteCharacteristic buttonCharacteristic("19B10012-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);
*/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_bleInterface = new BLEInterface(this);
    m_bleInterface->BLE_start("arBLE Peripheral", "DC:54:75:C6:D8:31");

    connChanged(m_bleInterface->isConnected());

    connect(ui->writeButton, SIGNAL(clicked()), this, SLOT(tx_data()));
    connect(ui->readButton, SIGNAL(clicked()), this, SLOT(rx_data()));
    connect(ui->chk_uuid, SIGNAL(clicked()), this, SLOT(uuid_enable()));
    connect(m_bleInterface, &BLEInterface::dataReceived, this, &MainWindow::dataChanged);
    connect(m_bleInterface, &BLEInterface::connectedChanged, this, &MainWindow::connChanged);
    connect(m_bleInterface, &BLEInterface::statusInfoChanged, [this](QString info, bool)
        {
            this->statusBar()->showMessage(info);
        });
    uuid_enable();
}

MainWindow::~MainWindow()
{
    m_bleInterface->BLE_stop();
    delete m_bleInterface;
    delete ui;
}

// -------------- slots ----------------------
void MainWindow::tx_data()
{
    QByteArray data;
    data = QByteArray::fromHex(ui->sendTextEdit->toPlainText().toLatin1());
    qDebug() << "write:" << data;
    if(ui->chk_uuid->checkState() != Qt::Checked)
      m_bleInterface->BLE_write(data);
    else
      m_bleInterface->BLE_writeByUuid(data, ui->txt_uuid->text());
}
void MainWindow::rx_data()
{
    QByteArray data;
    ui->receivedTextEdit->clear();
    if(ui->chk_uuid->checkState() != Qt::Checked)
        m_bleInterface->BLE_read(data);
    else
       m_bleInterface->BLE_readByUuid(data, ui->txt_uuid->text());

    ui->receivedTextEdit->setText(data.toHex() + " READ");
}
void MainWindow::uuid_enable() {
    if(ui->chk_uuid->checkState()== Qt::Checked) {
        ui->lbl_uuid->setEnabled(true);
        ui->txt_uuid->setEnabled(true);
        ui->readButton->setText("Read Hex (by UUID)");
        ui->writeButton->setText("Write Hex (by UUID)");
    } else {
        ui->lbl_uuid->setEnabled(false);
        ui->txt_uuid->setEnabled(false);
        ui->readButton->setText("Read Hex (default)");
        ui->writeButton->setText("Write Hex (default)");
    }
}

void MainWindow::dataChanged(QByteArray data, QString uuid)
{
    ui->receivedTextEdit->append(data.toHex() + " EVENT " + uuid);
}
void MainWindow::connChanged(bool value)
{
    ui->writeButton->setEnabled(value);
    ui->readButton->setEnabled(value);
}
