#ifndef INFOTHREAD_H
#define INFOTHREAD_H

#include<QThread>
#include<string>
#include<QString>

using namespace std;

class InfoThread: public QThread
{
    Q_OBJECT
public:
    explicit InfoThread();

    void sendInfo(string);
    void updateDownloadProcess(int process);
    void hideProcessBar();
    void showProcessBar();
    void updateRemotePath(string);
signals:
    void emitInfo(QString);
    void emitDownloadProcess(int);
    void emitSetDownloadProcessVisibility(bool);
    void emitUpdateRemotePath(QString);
};

#endif // INFOTHREAD_H
