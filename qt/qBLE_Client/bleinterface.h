#ifndef BLEINTERFACE_H
#define BLEINTERFACE_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QTimer>
#include "qqmlhelpers.h"

#define READ_INTERVAL_MS  3000
#define MAIN_INTERVAL_MS  1000
#define MAX_RETRY_COUNTER 5
#define CHUNK_SIZE        20

enum E_STATUS {
  E_IDLE,
  E_WAIT_DEVICE_AVAILABLE,
  E_WAIT_DEVICE_CONNECTION,
  E_TEST_DEVICE_SERVICE,
  E_READY,
  E_PREPARE_RETRY,
  E_WAIT_FOR_RETRY,
  E_STATUS_LAST_ITEM
};

class DeviceInfo: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString deviceName READ getName NOTIFY deviceChanged)
    Q_PROPERTY(QString deviceAddress READ getAddress NOTIFY deviceChanged)
public:
    DeviceInfo(const QBluetoothDeviceInfo &device);
    QString getName() const { return m_device.name(); }
    QBluetoothDeviceInfo getDevice() const;
    QString getAddress() const;
    void setDevice(const QBluetoothDeviceInfo &device);
signals:
    void deviceChanged();

private:
    QBluetoothDeviceInfo m_device;
};

class BLEInterface : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(int currentService READ currentService WRITE setCurrentService NOTIFY currentServiceChanged)
    QML_WRITABLE_PROPERTY(int, currentDevice)
    QML_READONLY_PROPERTY(QStringList, devicesNames)
    QML_READONLY_PROPERTY(QStringList, services)

public:
    explicit BLEInterface(QObject *parent = 0);
    ~BLEInterface();

    bool BLE_start(QString name, QString address);
    void BLE_stop();

    void BLE_write(const QByteArray& data);
    void BLE_read(QByteArray &data);

    bool BLE_writeByUuid(const QByteArray &data, QString uuid);
    bool BLE_readByUuid(QByteArray &data, QString uuid);


    bool isConnected() const
    {
        return m_connected;
    }
    int currentService() const
    {
        return m_currentService;
    }

signals:
    // raise events!
    void statusInfoChanged(QString info, bool isGood);
    void dataReceived(const QByteArray &data, QString uuid);
    void connectedChanged(bool connected);
    void currentServiceChanged(int currentService);

private slots:
    void mainProgram();

    //QBluetothDeviceDiscoveryAgent
    void onAddDevice(const QBluetoothDeviceInfo&);
    void onScanFinished();
    void onDeviceScanError(QBluetoothDeviceDiscoveryAgent::Error);

    //QLowEnergyController
    void onServiceDiscovered(const QBluetoothUuid &);
    void onServiceScanDone();
    void onControllerError(QLowEnergyController::Error);
    void onDeviceConnected();
    void onDeviceDisconnected();

    //QLowEnergyService
    void onServiceStateChanged(QLowEnergyService::ServiceState s);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void onServiceError(QLowEnergyService::ServiceError e);
    void onCharacteristicRead(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void onCharacteristicWrite(const QLowEnergyCharacteristic &c, const QByteArray &value);

private:
    E_STATUS m_status;
    E_STATUS m_status_cpy;
    int m_result[E_STATUS_LAST_ITEM];
    QString m_statusName[E_STATUS_LAST_ITEM];
    QString m_currentDeviceName;
    QString m_currentDeviceAddress;
    QTimer *m_statusTimer;

    QByteArray                     m_LastReadValue;
    int                            m_currentService;
    QBluetoothDeviceDiscoveryAgent *m_deviceDiscoveryAgent;
    QLowEnergyDescriptor           m_notificationDesc;
    QLowEnergyController           *m_control;
    QList<QBluetoothUuid>          m_servicesUuid;
    QLowEnergyService              *m_service;
    QLowEnergyCharacteristic       m_readCharacteristic;
    QLowEnergyCharacteristic       m_writeCharacteristic;
    QList<DeviceInfo*>             m_devices;
    bool                           m_connected;
    QLowEnergyService::WriteMode   m_writeMode;

    void raiseMessage(QString s, bool v);
    QString getStatusName(int index);
    bool setStatus(E_STATUS index);
    inline void waitForWrite();
    inline void waitForRead();
    void update_connected(bool connected);
    void searchCharacteristic();
    void update_currentService(int currentSerice);
    void setCurrentService(int currentService);
    void connectDevice();
    void disconnectDevice();
    void scanDevices();
};

#endif // BLEINTERFACE_H
