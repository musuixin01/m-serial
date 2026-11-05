#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QSerialPort;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void refreshSerialPorts();
    void openCloseSerial();
    void sendData();
    void onSerialReadyRead();
    void updateUiForSerial(bool open);
    void onAutoSendToggled(bool checked);
    void onClearReceive();
    void onClearSend();
    void onSaveLog();
    void onAutoRefreshPorts();

private:
    QByteArray parseSendData() const;
    QString formatReceivedData(const QByteArray &data) const;
    QString lineEnding() const;

    Ui::MainWindow *ui;
    QSerialPort *serial;
    QTimer *autoSendTimer;
    QTimer *portRefreshTimer;
};

#endif // MAINWINDOW_H
