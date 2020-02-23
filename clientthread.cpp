#include "clientthread.h"

ClientThread::ClientThread()
{
    for(int i=0; i<5; i++)
        arglist.push_back("");
    curClient = new Client();
}
ClientThread::~ClientThread(){
    delete curClient;

}
void ClientThread::run(){
    curClient->startTask();
    emit emitRunning();
    switch (task) {
    case TConnect:
        if(!curClient->connectServer())
            emit emitSuccess();
        flushList();
        break;
    case TDisconnect:
        if(curClient->disconnect()==0){
            emit emitSuccess();
        }
        flushList();
        break;
    case TCd:
        curClient->changeDir(arglist[0]);
        flushList();
        break;
    case TUp:
        curClient->upFile(arglist[0]);
//        sleep(300);
        flushList();
        break;
    case TDown:
        curClient->downFile(arglist[0],arglist[1]);
        break;
    case TDele:
        curClient->deleteFile(arglist[0]);
        flushList();
        break;
    default:
        break;
    }
    curClient->infoThread->sendInfo("chlient thread has done\n");
    stop();
}
void ClientThread::flushList() {
    //刷新文件列表
    emit emitClearList();
    curClient->infoThread->sendInfo("flushList\n");
    int num = curClient->filelist.size();
    QString type, size, time, name;
    vector<string> eachrow;
    for(int i=0; i<num; i++) {
        eachrow = curClient->filelist[i];
        //if(type=="-")   continue;
        size = QString::fromStdString(eachrow[4]);
        time = QString::fromStdString(eachrow[5] + " " + eachrow[6] + " " + eachrow[7]);
        name = QString::fromStdString(eachrow[8]);
        switch (eachrow[0][0]) {
        case '-':
            type = "file";
            break;
        case 'd':
            type = "dictory";
            break;
        case 'I':
            type = "link";
            break;
        case 'b':
            type = "block device";
            break;
        case 'c':
            type = "character device";
            break;
        case 'p':
            type = "pipe";
            break;
        case 's':
            type = "socket";
            break;
        default:
            type = "-";
            break;
        }

        emit emitListItem(name, time, type, size);
    }

}

void ClientThread::stop(){
    emit emitStop();
    std::cout<<"subthread finished"<<std::endl;
    curClient->stopCurrentTask();
    //isInterruptionRequested();  //关闭线程
    //quit();
}
