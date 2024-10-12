#include "bleinterface.h"
#include <QDebug>
#include <QEventLoop>

// ************ CLASS DEVICEINFO *******************
DeviceInfo::DeviceInfo(const QBluetoothDeviceInfo &info):
    QObject(), m_device(info)
{
}
QBluetoothDeviceInfo DeviceInfo::getDevice() const
{
    return m_device;
}
QString DeviceInfo::getAddress() const
{
#ifdef Q_OS_MAC
    // workaround for Core Bluetooth:
    return m_device.deviceUuid().toString();
#else
    return m_device.address().toString();
#endif
}
void DeviceInfo::setDevice(const QBluetoothDeviceInfo &device)
{
    m_device = device;
    emit deviceChanged();
}
// ************ COSTRUCTOR *******************
BLEInterface::BLEInterface(QObject *parent) : QObject(parent)
{
    m_currentDevice = 0;
    m_control = 0;
    m_service = 0;
    m_connected = false;
    m_currentService = 0;
    m_currentDeviceName = "";
    m_currentDeviceAddress = "";
    m_status = E_IDLE;
    m_status_cpy = E_STATUS_LAST_ITEM;
    m_LastReadValue.clear();
    for(int i = 0; i < E_STATUS_LAST_ITEM; i++) {
       m_result[i] = 0;
       m_statusName[i] = "STATUS index=" + QString::number(i);
    }
    m_statusName[E_IDLE] = "E_IDLE";
    m_statusName[E_WAIT_DEVICE_AVAILABLE] = "E_WAIT_DEVICE_AVAILABLE";
    m_statusName[E_WAIT_DEVICE_CONNECTION] = "E_WAIT_DEVICE_CONNECTION";
    m_statusName[E_TEST_DEVICE_SERVICE] = "E_TEST_DEVICE_SERVICE";
    m_statusName[E_READY] = "E_READY";
    m_statusName[E_PREPARE_RETRY] = "E_PREPARE_RETRY";
    m_statusName[E_WAIT_FOR_RETRY] = "E_WAIT_FOR_RETRY";

    m_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    connect(m_deviceDiscoveryAgent, SIGNAL(deviceDiscovered(const QBluetoothDeviceInfo&)), this, SLOT(onAddDevice(const QBluetoothDeviceInfo&)));
    connect(m_deviceDiscoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)),  this, SLOT(onDeviceScanError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(m_deviceDiscoveryAgent, SIGNAL(finished()), this, SLOT(onScanFinished()));

    m_statusTimer= new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &BLEInterface::mainProgram);
    m_statusTimer->start(MAIN_INTERVAL_MS);
}
// ************ DESTRUCTOR *******************
BLEInterface::~BLEInterface()
{
    qDeleteAll(m_devices);
    m_devices.clear();
}

/************************************************************************************
 *
 *       GENERAL FUNCTIONS
 *
************************************************************************************/

// ---- device disconnect -------
void BLEInterface::disconnectDevice()
{
    if (m_devices.isEmpty()) {
        return;
    }

    //disable notifications
    if (m_notificationDesc.isValid() && m_service) {
        m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0000"));
    } else {
        m_control->disconnectFromDevice();
        delete m_service;
        m_service = 0;
    }
}
// ---- update connection -------
void BLEInterface::update_connected(bool connected){
    if(connected != m_connected){
        m_connected = connected;
        emit connectedChanged(connected);
    }
}

// ------ get STATUS NAME -----------
QString BLEInterface::getStatusName(int index)
{
    if(index < 0 || index >= E_STATUS_LAST_ITEM)
        return "INVALID STATUS, wrong index=" + QString::number(index);
    return m_statusName[index];
}
// ------ set STATUS -----------
bool BLEInterface::setStatus(E_STATUS index)
{
    if(index < 0 || index >= E_STATUS_LAST_ITEM)
        return false;
    m_result[index] = 0;
    m_status = index;
    return true;
}
// ------ raise message -----------
void BLEInterface::raiseMessage(QString s, bool v)
{
    emit statusInfoChanged(s, v);
    qDebug() << s;
}

/************************************************************************************
 *
 *  SCAN DEVICES AND MANAGE 3 EVENTS, result: E_WAIT_DEVICE_AVAILABLE
 *
************************************************************************************/
void BLEInterface::scanDevices()
{
    m_devicesNames.clear();
    qDeleteAll(m_devices);
    m_devices.clear();
    raiseMessage("Scanning for devices...", true);
    m_deviceDiscoveryAgent->start();
}
// ---- scan event n.1
void BLEInterface::onAddDevice(const QBluetoothDeviceInfo &device)
{
    qDebug() << "---SCAN: onAddDevice";
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        QString name = device.name();
        QString adr = device.address().toString();

        if (name != m_currentDeviceName && adr != m_currentDeviceAddress)
            return;

        qWarning() << "Discovered LE Device: Name=" << name << " Address=" << adr;
        m_devicesNames.append(device.name());
        DeviceInfo *dev = new DeviceInfo(device);
        m_devices.append(dev);
        m_deviceDiscoveryAgent->stop();
        raiseMessage("Low Energy device found.", true);
        m_result[E_WAIT_DEVICE_AVAILABLE] = 1;
    }
}
// ---- scan event n.2
void BLEInterface::onScanFinished()
{
    qDebug() << "---SCAN: onScanFinished()";
    if (m_devicesNames.size() == 0) {
        raiseMessage("No Low Energy devices found", false);
        m_result[E_WAIT_DEVICE_AVAILABLE] = -1;
    } else {
        m_result[E_WAIT_DEVICE_AVAILABLE] = 1;
    }
}
// ---- scan event n.3
void BLEInterface::onDeviceScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    qDebug() << "SCAN: onDeviceScanError()";
    if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError) {
        raiseMessage("The Bluetooth adaptor is powered off, power it on before doing discovery.", false);
        m_result[E_WAIT_DEVICE_AVAILABLE] = -2;
    } else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError) {
        raiseMessage("Writing or reading from the device resulted in an error.", false);
        m_result[E_WAIT_DEVICE_AVAILABLE] = -3;
    } else {
        raiseMessage("An unknown error has occurred.", false);
        m_result[E_WAIT_DEVICE_AVAILABLE] = -4;
    }
}
/************************************************************************************
 *
 *  CONNECT DEVICE AND MANAGE 5 EVENTS, result: E_WAIT_DEVICE_CONNECTION
 *
************************************************************************************/
void BLEInterface::connectDevice()
{
    if(m_devices.isEmpty())
        return;

    if (m_control) {
        m_control->disconnectFromDevice();
        delete m_control;
        m_control = 0;

    }
    m_control = new QLowEnergyController(m_devices[ m_currentDevice]->getDevice(), this);
    connect(m_control, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(onServiceDiscovered(QBluetoothUuid)));
    connect(m_control, SIGNAL(discoveryFinished()),  this, SLOT(onServiceScanDone()));
    connect(m_control, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(onControllerError(QLowEnergyController::Error)));
    connect(m_control, SIGNAL(connected()), this, SLOT(onDeviceConnected()));
    connect(m_control, SIGNAL(disconnected()), this, SLOT(onDeviceDisconnected()));
    m_control->connectToDevice();
}
// ---- connect event n.1
void BLEInterface::onDeviceConnected()
{
    qDebug() << "---CONNECT: onDeviceConnected";
    m_servicesUuid.clear();
    m_services.clear();

    qDebug() << "reset currentService, m_control->discoverServices()";
    setCurrentService(-1);
    m_control->discoverServices();
}

// ---- connect event n.2
void BLEInterface::onDeviceDisconnected()
{
    qDebug() << "---CONNECT: onDeviceDisconnected";
    update_connected(false);
    raiseMessage("Service disconnected", false);
    qWarning() << "Remote device disconnected";
    m_result[E_WAIT_DEVICE_CONNECTION] = -1;
}
// ---- connect event n.3
void BLEInterface::onControllerError(QLowEnergyController::Error error)
{
    qDebug() << "---CONNECT: onControllerError";
    raiseMessage("Cannot connect to remote device.", false);
    qWarning() << "Controller Error:" << error;
    m_result[E_WAIT_DEVICE_CONNECTION] = -2;
}
// ---- connect event n.4
void BLEInterface::onServiceDiscovered(const QBluetoothUuid &gatt)
{
    qDebug() << "---CONNECT: onServiceDiscovered";
    Q_UNUSED(gatt)
    raiseMessage("Service discovered. Waiting for service scan to be done...", true);
}
// ---- connect event n.5
void BLEInterface::onServiceScanDone()
{
    qDebug() << "---CONNECT: onServiceScanDone";
    m_servicesUuid = m_control->services();
    if(m_servicesUuid.isEmpty()) {
        raiseMessage("Can't find any services.", true);
        m_result[E_WAIT_DEVICE_CONNECTION] = -3;
    } else {
        m_services.clear();
        foreach (auto uuid, m_servicesUuid) {
            m_services.append(uuid.toString());
        }
        m_currentService = -1;
        raiseMessage("All services discovered.", true);
        m_result[E_WAIT_DEVICE_CONNECTION] = 1;
    }
}

/************************************************************************************
 *
 *  SERVICE/CHARACTERISTIC AND MANAGE 5 EVENTS, result: E_TEST_DEVICE_SERVICE
 *
************************************************************************************/

void BLEInterface::setCurrentService(int currentService)
{
    qDebug() << "setCurrentService, index=" << currentService;
    if (m_currentService == currentService)
        return;
    update_currentService(currentService);
    m_currentService = currentService;
    emit currentServiceChanged(currentService);
}
void BLEInterface::update_currentService(int indx)
{
    qDebug() << "update_currentService, index=" << indx;
    delete m_service;
    m_service = 0;

    if (indx >= 0 && m_servicesUuid.count() > indx) {
        m_service = m_control->createServiceObject( m_servicesUuid.at(indx), this);
    }

    if (!m_service) {
        raiseMessage("Service not found.", false);
        m_result[E_TEST_DEVICE_SERVICE] = -1;
        return;
    }

    qDebug() << "update_currentService: uuid=" << m_service->serviceUuid();

    connect(m_service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(onServiceStateChanged(QLowEnergyService::ServiceState)));
    connect(m_service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(onCharacteristicChanged(QLowEnergyCharacteristic,QByteArray)));
    connect(m_service, SIGNAL(characteristicRead(QLowEnergyCharacteristic,QByteArray)), this, SLOT(onCharacteristicRead(QLowEnergyCharacteristic,QByteArray)));
    connect(m_service, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(onCharacteristicWrite(QLowEnergyCharacteristic,QByteArray)));
    connect(m_service, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(onServiceError(QLowEnergyService::ServiceError)));

    if(m_service->state() == QLowEnergyService::DiscoveryRequired) {
        raiseMessage("Connecting to service...", true);
        qDebug() << "update_currentService: m_service->discoverDetails()";
        m_service->discoverDetails();  // this generates characteristic events
    }  else {
        qDebug() << "update_currentService: searchCharacteristic()";
        searchCharacteristic();
    }
}
void BLEInterface::searchCharacteristic(){
    bool ok = false;
    if(m_service) {
        foreach (QLowEnergyCharacteristic c, m_service->characteristics()) {
#ifdef SELECTION_DEBUG
            QString uuid = c.uuid().toString();
            if (uuid != "{19b10011-e8f2-537e-4f6c-d104768a1214}")
                continue;
#endif
            if(c.isValid()){

                if (c.properties() & QLowEnergyCharacteristic::WriteNoResponse ||
                    c.properties() & QLowEnergyCharacteristic::Write) {
                    m_writeCharacteristic = c;
                    qDebug() << "Write Characteristic: " << c.uuid();
                    if(c.properties() & QLowEnergyCharacteristic::WriteNoResponse)
                        m_writeMode = QLowEnergyService::WriteWithoutResponse;
                    else
                        m_writeMode = QLowEnergyService::WriteWithResponse;

                }
                if (c.properties() & QLowEnergyCharacteristic::Read) {
                    m_readCharacteristic = c;
                    qDebug() << "Read Characteristic: " << c.uuid();
                }
                ok = true;
                // ENABLE NOTIFICATION EVENTS FROM SERVER
                m_notificationDesc = c.descriptor( QBluetoothUuid::ClientCharacteristicConfiguration);
                if (m_notificationDesc.isValid()) {
                    m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
                }
            }
        }
    }
    if(ok) {
      m_result[E_TEST_DEVICE_SERVICE] = 1;
      raiseMessage("Search characteristic done.", true);
    } else {
      m_result[E_TEST_DEVICE_SERVICE] = -2;
      raiseMessage("Search characteristic error.", true);
    }
}

// service/characteristic event n.1
void BLEInterface::onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    qDebug() << "---SERVICE: onCharacteristicChanged";
    QString uuid = c.uuid().toString();
    Q_UNUSED(c)
    qDebug() << "Characteristic Changed: " << value << "size:" << value.size() << "uuid:" << uuid;
    emit dataReceived(value, uuid);
}
// service/characteristic event n.2
void BLEInterface::onCharacteristicWrite(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    qDebug() << "---SERVICE: onCharacteristicWrite";
    QString uuid = c.uuid().toString();
    Q_UNUSED(c)
    qDebug() << "Characteristic Written: " << value << "size:" << value.size() << "uuid:" << uuid;
}
// service/characteristic event n.3
void BLEInterface::onCharacteristicRead(const QLowEnergyCharacteristic &c, const QByteArray &value){
    qDebug() << "---SERVICE: onCharacteristicRead";
    QString uuid = c.uuid().toString();
    Q_UNUSED(c)
    qDebug() << "Characteristic Read: " << value << "size:" << value.size() << "uuid:" << uuid;
    m_LastReadValue = value;
}
// service/characteristic event n.4
void BLEInterface::onServiceStateChanged(QLowEnergyService::ServiceState s)
{
    qDebug() << "---SERVICE: serviceStateChanged, state: " << s;
    if (s == QLowEnergyService::ServiceDiscovered) {
        searchCharacteristic();
    }
}
// service/characteristic event n.5
void BLEInterface::onServiceError(QLowEnergyService::ServiceError e)
{
    qDebug() << "---SERVICE: serviceError";
    qWarning() << "Service error:" << e;
    raiseMessage("Service error", true);
    m_result[E_TEST_DEVICE_SERVICE] = -3;
}
/*-------------------------------------------------------------------------------------
 *   MAIN PROGRAM (status machine)
 * ----------------------------------------------------------------------------------*/

void BLEInterface::mainProgram()
{
    static int retry_counter = 0;

    if(m_status != m_status_cpy) {
       m_status_cpy = m_status;
       qDebug() <<  "-------->" << getStatusName(m_status);
    }
    switch(m_status) {
    case E_IDLE:
        if(m_result[E_IDLE] == 1) {
          setStatus(E_WAIT_DEVICE_AVAILABLE);
          scanDevices();
        }
        m_result[E_IDLE] = 0;
        break;

     case E_WAIT_DEVICE_AVAILABLE:
        if (m_result[m_status] == 0)
          break;
        if (m_result[m_status] < 0) {
          m_status = E_PREPARE_RETRY;
          break;
        }
        // device found, connect it!
        setStatus(E_WAIT_DEVICE_CONNECTION);
        connectDevice();
        break;

     case E_WAIT_DEVICE_CONNECTION:
        if (m_result[m_status] == 0)
          break;
        if (m_result[m_status] < 0) {
          m_status = E_PREPARE_RETRY;
          break;
        }
        // device connected!
        setStatus(E_TEST_DEVICE_SERVICE);
        setCurrentService(0);  // use the first service
        break;

    case E_TEST_DEVICE_SERVICE:
        if (m_result[m_status] == 0)
          break;
        if (m_result[m_status] < 0) {
          m_status = E_PREPARE_RETRY;
          break;
        }
        update_connected(true);
        m_status = E_READY;
        break;

    case E_READY:
        if(isConnected() == false) {
            m_status = E_PREPARE_RETRY;
        }
        break;

    case E_PREPARE_RETRY:
        update_connected(false);
        retry_counter = 0;
        m_status = E_WAIT_FOR_RETRY;
        break;

    case E_WAIT_FOR_RETRY:
        retry_counter++;
        if(retry_counter >= MAX_RETRY_COUNTER) {
            setStatus(E_WAIT_DEVICE_AVAILABLE);
            scanDevices();
        }
        break;
    default:
        break;
    }
}

/*-------------------------------------------------------------------------------------
 *   AID FUNCTIONS
 * ----------------------------------------------------------------------------------*/

void BLEInterface::waitForWrite() {
    QEventLoop loop;
    connect(m_service, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), &loop, SLOT(quit()));
    loop.exec();
}

void BLEInterface::waitForRead() {
    QEventLoop loop;
    connect(m_service, SIGNAL(characteristicRead(QLowEnergyCharacteristic,QByteArray)), &loop, SLOT(quit()));
    loop.exec();
}

/*-------------------------------------------------------------------------------------
 *   PUBLIC FUNCTIONS
 * ----------------------------------------------------------------------------------*/

bool BLEInterface::BLE_start(QString name, QString address)
{
    if(m_status != E_IDLE && m_status != E_WAIT_FOR_RETRY)
        return false;

    m_currentDeviceName = name;
    m_currentDeviceAddress = address;
    m_result[E_IDLE] = 1;
    return true;
}
void BLEInterface::BLE_stop()
{
    setStatus(E_IDLE);
    disconnectDevice();
}

void BLEInterface::BLE_read(QByteArray &data)
{
    qDebug() << "BLEInterface::BLE_read";
    data.clear();
    if(m_status != E_READY)
        return;

    m_LastReadValue.clear();
    if(m_service && m_readCharacteristic.isValid()) {
        m_service->readCharacteristic(m_readCharacteristic);
        qDebug() << m_readCharacteristic.uuid() << " read =>";
        waitForRead();
        qDebug() << m_readCharacteristic.uuid() << "read <=";
    }
    data = m_LastReadValue;
}

void BLEInterface::BLE_write(const QByteArray &data)
{
    qDebug() << "BLEInterface::BLE_write: " << data;
    if(m_status != E_READY)
        return;
    if(m_service && m_writeCharacteristic.isValid()){
        if(data.length() > CHUNK_SIZE){
            qDebug() << m_writeCharacteristic.uuid() << "write type 1";
            int sentBytes = 0;
            while (sentBytes < data.length()) {
                m_service->writeCharacteristic(m_writeCharacteristic, data.mid(sentBytes, CHUNK_SIZE), m_writeMode);
                sentBytes += CHUNK_SIZE;
                if(m_writeMode == QLowEnergyService::WriteWithResponse){
                    qDebug() << m_writeCharacteristic.uuid() << " write =>";
                    waitForWrite();
                    qDebug() << m_writeCharacteristic.uuid() << " write <=";
                    if(m_service->error() != QLowEnergyService::NoError)
                        return;
                }
            }
        } else {
            qDebug() << m_writeCharacteristic.uuid() << "write type 2";
            m_service->writeCharacteristic(m_writeCharacteristic, data, m_writeMode);
        }
    }
}

//-------------------- R/W by characteristic uuid -----------------------------

bool uuid_ok(QString uuid1, QString uuid2)
{
    QString u1 = uuid1.replace("{","");
    u1 = u1.replace("}","");
    u1 = u1.toUpper();
    QString u2 = uuid2.replace("{","");
    u2 = u2.replace("}","");
    u2 = u1.toUpper();
    return  u1 == u2;
}

bool BLEInterface::BLE_writeByUuid(const QByteArray &data, QString uuid)
{
    qDebug() << "BLEInterface::BLE_writeByUuid";
    if(m_status != E_READY)
       return false;

    bool ok = false;
    if(m_service) {
        foreach (QLowEnergyCharacteristic c, m_service->characteristics()) {
            if(c.isValid() == true && uuid_ok(c.uuid().toString(), uuid) == true){
                QLowEnergyService::WriteMode wMode;
                if (c.properties() & QLowEnergyCharacteristic::WriteNoResponse || c.properties() & QLowEnergyCharacteristic::Write) {
                    if(c.properties() & QLowEnergyCharacteristic::WriteNoResponse) {
                        wMode = QLowEnergyService::WriteWithoutResponse;
                    } else {
                        wMode = QLowEnergyService::WriteWithResponse;
                    }
                    if(data.length() > CHUNK_SIZE){
                        qDebug() << uuid << "write type 1";
                        int sentBytes = 0;
                        while (sentBytes < data.length()) {
                            m_service->writeCharacteristic(c, data.mid(sentBytes, CHUNK_SIZE), wMode);
                            sentBytes += CHUNK_SIZE;
                            if(wMode == QLowEnergyService::WriteWithResponse){
                                qDebug() << c.uuid() << " write =>";
                                waitForWrite();
                                qDebug() << c.uuid() << " write <=";
                                if(m_service->error() != QLowEnergyService::NoError)
                                    return ok;
                            }
                        }
                    } else {
                        qDebug() << uuid << "write type 2";
                        m_service->writeCharacteristic(c, data, wMode);
                    }
                    ok = true;
                    break;
                }
            }
        }
    }
    return ok;
}
bool BLEInterface::BLE_readByUuid(QByteArray &data, QString uuid)
{
    data.clear();
    qDebug() << "BLEInterface::BLE_readByUuid";
    m_LastReadValue.clear();
    if(m_status != E_READY)
        return false;

    bool ok = false;
    if(m_service) {
        foreach (QLowEnergyCharacteristic c, m_service->characteristics()) {
            if(c.isValid() == true && uuid_ok(c.uuid().toString(), uuid) == true){
                if (c.properties() & QLowEnergyCharacteristic::Read) {
                   qDebug() << "Read Characteristic: " << c.uuid();
                }
                m_service->readCharacteristic(c);
                qDebug() << uuid << " read =>";
                waitForRead();
                qDebug() << uuid << "read <=";
                data = m_LastReadValue;
                ok = true;
                break;
             }
        }
    }
    return ok;
}




