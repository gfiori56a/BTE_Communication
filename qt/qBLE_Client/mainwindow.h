#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "bleinterface.h"
namespace Ui {
class MainWindow;
}
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
   void tx_data();
   void rx_data();
   void uuid_enable();
private:
    Ui::MainWindow *ui;
    BLEInterface *m_bleInterface;
    void dataChanged(QByteArray data, QString uuid);
    void connChanged(bool value);
};

#endif // MAINWINDOW_H
