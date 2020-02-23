#ifndef FTPMAINWINDOW_H
#define FTPMAINWINDOW_H
#include<QTreeWidgetItem>
#include <QMainWindow>
#include<QFileDialog>
#include "clientthread.h"
namespace Ui {
class FTPMainWindow;
}

class FTPMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FTPMainWindow(QWidget *parent = 0);
    ~FTPMainWindow();

private slots:
    // 接收从消息线程传来的信号
    void recvSuccess();
    void recvInfo(QString info);
    void updateRemotePath(QString);
    void clientThreadRunning();
    void updateDownloadProcess(int process);
    void setProcessBarVIsibility(bool);

    // 接收从socket线程传来的信号
    void recvClearList();
    void recvListItem(QString, QString, QString, QString);
    void recvClientThreadStop();

    // 接收从界面传来的信号
    void on_connectButton_clicked();
    void on_downButton_clicked();
    void on_upButton_clicked();
    void on_deleteButton_clicked();
    void on_remoteFileTree_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_pauseButton_clicked();

private:
    Ui::FTPMainWindow *ui;
    ClientThread* clientThread;
    bool connected = false; //标识当前的状态
};

#endif // FTPMAINWINDOW_H
