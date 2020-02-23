#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H
#include<QThread>
#include<vector>
#include<string>
#include"client.h"
enum subThreadTask{TConnect, TDisconnect, TCd, TDown, TUp, TDele};

using namespace std;
class ClientThread:public QThread
{
    Q_OBJECT
public:
    explicit ClientThread();  // 声明为explicit的构造函数不能在隐式转换中使用
    ~ClientThread();
    void bind(Client*c);
    subThreadTask task;
    vector<string> arglist;
    Client * curClient;

protected:
    void run();
private:
    void flushList();
public slots:
    void stop();

signals:
    void emitListItem(QString, QString, QString, QString);
    void emitInfo(QString);
    void emitSuccess();
    void emitClearList();
    void emitRunning();
    void emitStop();

};

#endif // CLIENTTHREAD_H
