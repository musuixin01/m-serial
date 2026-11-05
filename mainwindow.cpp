#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTimer>
#include <QTextCursor>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serial(new QSerialPort(this))
    , autoSendTimer(new QTimer(this))
    , portRefreshTimer(new QTimer(this))
{
    ui->setupUi(this);

    ui->comboBoxBaudRate->addItems({"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"});
    ui->comboBoxDataBits->addItems({"5", "6", "7", "8"});
    ui->comboBoxParity->addItems({"无", "奇校验", "偶校验", "标志位", "空校验"});
    ui->comboBoxStopBits->addItems({"1", "1.5", "2"});
    ui->comboBoxFlowControl->addItems({"无", "硬件", "软件"});
    ui->comboBoxLineEnd->addItems({"无", "\r", "\n", "\r\n"});

    ui->checkBoxHexReceive->setChecked(false);
    ui->checkBoxHexSend->setChecked(false);
    ui->checkBoxAutoSend->setChecked(false);
    ui->checkBoxShowTimestamp->setChecked(true);
    ui->spinBoxAutoSendInterval->setValue(1000);
    ui->spinBoxAutoSendInterval->setSuffix(" ms");
    ui->plainTextEditSend->setPlaceholderText("输入要发送的内容，支持文本或 HEX 格式");
    ui->textEditReceive->setReadOnly(true);

    refreshSerialPorts();
    updateUiForSerial(false);

    connect(ui->pushButtonOpenClose, &QPushButton::clicked, this, &MainWindow::openCloseSerial);
    connect(ui->pushButtonRefresh, &QPushButton::clicked, this, &MainWindow::refreshSerialPorts);
    connect(ui->pushButtonSend, &QPushButton::clicked, this, &MainWindow::sendData);
    connect(ui->pushButtonClearReceive, &QPushButton::clicked, this, &MainWindow::onClearReceive);
    connect(ui->pushButtonClearSend, &QPushButton::clicked, this, &MainWindow::onClearSend);
    connect(ui->pushButtonSaveLog, &QPushButton::clicked, this, &MainWindow::onSaveLog);
    connect(ui->checkBoxAutoSend, &QCheckBox::toggled, this, &MainWindow::onAutoSendToggled);
    connect(autoSendTimer, &QTimer::timeout, this, &MainWindow::sendData);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::onSerialReadyRead);
    connect(portRefreshTimer, &QTimer::timeout, this, &MainWindow::onAutoRefreshPorts);

    portRefreshTimer->start(3000);
}

MainWindow::~MainWindow()
{
    if (serial->isOpen()) {
        serial->close();
    }
    delete ui;
}

void MainWindow::refreshSerialPorts()
{
    QString currentPort = ui->comboBoxPortName->currentText();
    ui->comboBoxPortName->clear();

    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ui->comboBoxPortName->addItem(info.portName() + (info.description().isEmpty() ? "" : " (" + info.description() + ")"));
    }

    if (ui->comboBoxPortName->findText(currentPort) != -1) {
        ui->comboBoxPortName->setCurrentText(currentPort);
    }
}

void MainWindow::openCloseSerial()
{
    if (serial->isOpen()) {
        serial->close();
        updateUiForSerial(false);
        ui->statusbar->showMessage("串口已关闭");
        return;
    }

    if (ui->comboBoxPortName->currentText().isEmpty()) {
        QMessageBox::warning(this, "提示", "未选择串口，请先刷新并选择可用串口。");
        return;
    }

    QString portName = ui->comboBoxPortName->currentText();
    if (portName.contains('(')) {
        portName = portName.left(portName.indexOf('(')).trimmed();
    }
    serial->setPortName(portName);
    serial->setBaudRate(ui->comboBoxBaudRate->currentText().toInt());
    serial->setDataBits(static_cast<QSerialPort::DataBits>(ui->comboBoxDataBits->currentText().toInt()));
    serial->setParity([this]() {
        const QString value = ui->comboBoxParity->currentText();
        if (value == "奇校验") return QSerialPort::OddParity;
        if (value == "偶校验") return QSerialPort::EvenParity;
        if (value == "标志位") return QSerialPort::MarkParity;
        if (value == "空校验") return QSerialPort::SpaceParity;
        return QSerialPort::NoParity;
    }());
    serial->setStopBits([this]() {
        const QString value = ui->comboBoxStopBits->currentText();
        if (value == "1.5") return QSerialPort::OneAndHalfStop;
        if (value == "2") return QSerialPort::TwoStop;
        return QSerialPort::OneStop;
    }());
    serial->setFlowControl([this]() {
        const QString value = ui->comboBoxFlowControl->currentText();
        if (value == "硬件") return QSerialPort::HardwareControl;
        if (value == "软件") return QSerialPort::SoftwareControl;
        return QSerialPort::NoFlowControl;
    }());

    if (!serial->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, "错误", QString("串口打开失败：%1").arg(serial->errorString()));
        updateUiForSerial(false);
        return;
    }

    updateUiForSerial(true);
    ui->statusbar->showMessage(QString("已连接 %1，%2 波特").arg(portName).arg(serial->baudRate()));
}

void MainWindow::sendData()
{
    if (!serial->isOpen()) {
        QMessageBox::warning(this, "提示", "串口未打开，请先打开串口再发送数据。\n也可先在发送区编写内容后打开串口。");
        return;
    }

    QByteArray data = parseSendData();
    if (data.isEmpty()) {
        return;
    }

    qint64 written = serial->write(data);
    if (written == -1) {
        QMessageBox::warning(this, "发送失败", "数据发送失败，请检查串口。");
        return;
    }

    ui->statusbar->showMessage(QString("已发送 %1 字节").arg(written));
}

QByteArray MainWindow::parseSendData() const
{
    QString content = ui->plainTextEditSend->toPlainText();
    if (content.isEmpty()) {
        return {};
    }

    QByteArray data;
    if (ui->checkBoxHexSend->isChecked()) {
        QString hex = content;
        hex.remove(QRegularExpression("[^0-9A-Fa-f]"));
        if (hex.length() % 2 != 0) {
            QMessageBox::warning(const_cast<MainWindow*>(this), "错误", "HEX 数据长度必须是偶数。请检查输入。");
            return {};
        }
        bool ok = true;
        for (int i = 0; i < hex.length(); i += 2) {
            QByteArray byte = QByteArray::fromHex(hex.mid(i, 2).toLatin1());
            if (byte.isEmpty()) {
                ok = false;
                break;
            }
            data.append(byte);
        }
        if (!ok) {
            QMessageBox::warning(const_cast<MainWindow*>(this), "错误", "HEX 数据格式错误，请输入有效的 16 进制字符串。");
            return {};
        }
    } else {
        data = content.toUtf8();
        data.append(lineEnding().toUtf8());
    }

    return data;
}

QString MainWindow::lineEnding() const
{
    const QString value = ui->comboBoxLineEnd->currentText();
    if (value == "\r") return "\r";
    if (value == "\n") return "\n";
    if (value == "\r\n") return "\r\n";
    return "";
}

void MainWindow::onSerialReadyRead()
{
    const QByteArray data = serial->readAll();
    if (data.isEmpty()) {
        return;
    }

    QString display = formatReceivedData(data);
    ui->textEditReceive->moveCursor(QTextCursor::End);
    ui->textEditReceive->insertPlainText(display);
    ui->textEditReceive->moveCursor(QTextCursor::End);
}

QString MainWindow::formatReceivedData(const QByteArray &data) const
{
    if (ui->checkBoxHexReceive->isChecked()) {
        QString hexText;
        for (unsigned char byte : data) {
            hexText += QString::asprintf("%02X ", byte);
        }
        if (ui->checkBoxShowTimestamp->isChecked()) {
            return QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ") + hexText + "\n";
        }
        return hexText + "\n";
    }

    QString text = QString::fromUtf8(data);
    if (ui->checkBoxShowTimestamp->isChecked()) {
        return QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ") + text;
    }
    return text;
}

void MainWindow::updateUiForSerial(bool open)
{
    ui->comboBoxPortName->setEnabled(!open);
    ui->comboBoxBaudRate->setEnabled(!open);
    ui->comboBoxDataBits->setEnabled(!open);
    ui->comboBoxParity->setEnabled(!open);
    ui->comboBoxStopBits->setEnabled(!open);
    ui->comboBoxFlowControl->setEnabled(!open);
    ui->pushButtonRefresh->setEnabled(!open);
    ui->pushButtonOpenClose->setText(open ? "关闭串口" : "打开串口");
    ui->pushButtonSend->setEnabled(open);
    ui->checkBoxAutoSend->setEnabled(open);
    ui->spinBoxAutoSendInterval->setEnabled(open && ui->checkBoxAutoSend->isChecked());
}

void MainWindow::onAutoSendToggled(bool checked)
{
    if (!serial->isOpen()) {
        ui->checkBoxAutoSend->setChecked(false);
        return;
    }
    if (checked) {
        autoSendTimer->start(ui->spinBoxAutoSendInterval->value());
    } else {
        autoSendTimer->stop();
    }
    ui->spinBoxAutoSendInterval->setEnabled(checked);
}

void MainWindow::onClearReceive()
{
    ui->textEditReceive->clear();
}

void MainWindow::onClearSend()
{
    ui->plainTextEditSend->clear();
}

void MainWindow::onSaveLog()
{
    QString fileName = QFileDialog::getSaveFileName(this, "保存接收日志", QString(), "文本文件 (*.txt);;所有文件 (*.*)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "保存失败", "无法保存文件，请检查权限和路径。");
        return;
    }

    file.write(ui->textEditReceive->toPlainText().toUtf8());
    file.close();
    ui->statusbar->showMessage("接收日志已保存：" + fileName);
}

void MainWindow::onAutoRefreshPorts()
{
    if (!serial->isOpen()) {
        refreshSerialPorts();
    }
}
