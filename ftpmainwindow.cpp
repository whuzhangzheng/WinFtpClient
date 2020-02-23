#include "ftpmainwindow.h"
#include "ui_ftpmainwindow.h"

FTPMainWindow::FTPMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FTPMainWindow)
{
    clientThread = new ClientThread();
    connect(clientThread, SIGNAL(emitRunning()), this, SLOT(clientThreadRunning()));
    connect(clientThread, SIGNAL(emitSuccess()), this, SLOT(recvSuccess()));
    connect(clientThread, SIGNAL(emitClearList()), this, SLOT(recvClearList()));
    connect(clientThread, SIGNAL(emitListItem(QString,QString,QString,QString)),
            this,SLOT(recvListItem(QString,QString,QString,QString)));
    connect(clientThread, SIGNAL(emitStop()),this, SLOT(recvClientThreadStop()));
    connect(clientThread->curClient->infoThread, SIGNAL(emitInfo(QString)), this, SLOT(recvInfo(QString)));
    connect(clientThread->curClient->infoThread, SIGNAL(emitUpdateRemotePath(QString)),this,SLOT(updateRemotePath(QString)));
    connect(clientThread->curClient->infoThread, SIGNAL(emitDownloadProcess(int)), this, SLOT(updateDownloadProcess(int)));
    connect(clientThread->curClient->infoThread, SIGNAL(emitSetDownloadProcessVisibility(bool)),this, SLOT(setProcessBarVIsibility(bool)));

    ui->setupUi(this);
    ui->progressBar->setVisible(false);
}

FTPMainWindow::~FTPMainWindow()
{
    delete ui;
    delete clientThread;
}

void FTPMainWindow::on_connectButton_clicked()
{
    if(!clientThread->isRunning()){
        if(!connected){     // 没有连接到服务器, 点击连接
            QString ipAddr = ui->ipEdit->text();
            QString username = ui->userEdit->text();
            QString password = ui->passEdit->text();
            clientThread->curClient->login(ipAddr, username, password);
            clientThread->task = TConnect;
            clientThread->start();
        }else{
            clientThread->task = TDisconnect;
            clientThread->start();
            ui->connectButton->setText("connect");
        }
    }

}

void FTPMainWindow::on_downButton_clicked()
{
    if(connected){
        if(!clientThread->isRunning()){
            QTreeWidgetItem * curItem = ui->remoteFileTree->currentItem();
            QString downName;
            if(curItem)
                downName = curItem->text(0); //获取文件名，只有当被选中item的时候下载才会有效
            else{
                recvInfo("please clicked the file you want to downloaded first!\n");
                return;
            }
            if(curItem->text(2)=="dictory"){
                recvInfo("sorry, cannot download a dictory, select a file insteadly!\n");
                return;
            }

            QString saveDir = QFileDialog::getExistingDirectory(
                        this,"choose the save path","C:/Users/lenovo/Desktop");
            if(saveDir == NULL || saveDir.isEmpty()){
                recvInfo("you don't select the dictory which target file will be download in!\n");
                return;
            }else{
                clientThread->task = TDown;
                clientThread->arglist[0] = downName.toStdString();
                clientThread->arglist[1] = saveDir.toStdString();
                //ui->progressBar->setValue(0);
                clientThread->start();
                recvInfo(QString::fromStdString("download name: ")+downName+'\n');
                recvInfo(QString::fromStdString("save dir: ")+saveDir+'\n');
            }
        }else{
            recvInfo("there's work running, wait please!\n");
        }
    }else{
        recvInfo("please connect first\n");
    }
}

void FTPMainWindow::on_upButton_clicked()
{
    if(connected){
        if(!clientThread->isRunning()){
            string localFile;
            localFile = QFileDialog::getOpenFileName(this, "choose file to upload.").toStdString();

            if(localFile.empty()){
                recvInfo("you haven't selected any file!\n");
            }else{
                recvInfo(QString::fromStdString("locafile: "+localFile+"\n"));
                clientThread->task = TUp;
                clientThread->arglist[0] = localFile;
                clientThread->start();
                ui->progressBar->setValue(0);
            }
        }else{
            recvInfo("there's work running, wait please!\n");
        }
    }else{
        recvInfo("please connect first\n");
    }

}

void FTPMainWindow::on_deleteButton_clicked()
{
    if(connected){
        if(!clientThread->isRunning()){
            QTreeWidgetItem* curItem = ui->remoteFileTree->currentItem();
            QString fname;
            if(curItem)
                fname = curItem->text(0);
            else{
                recvInfo("please clicked the file or dictory you want to delete first!\n");
                return;
            }
            clientThread->arglist[0] = fname.toStdString();

            if(curItem->text(2)!="file"){
                recvInfo("can only delete file, not folder!\n");
                return;
            }
            clientThread->task = TDele;
            clientThread->start();
        }else{
            recvInfo("some work are running! wait please.\n");
        }
    }
    else {
        recvInfo("please connect first!\n");
    }
}

void FTPMainWindow::on_remoteFileTree_itemDoubleClicked(QTreeWidgetItem *item, int column){
    QString type = item->text(2);
    if(type!="dictory"){
        clientThread->curClient->infoThread->sendInfo("open folder fail\n");
        return;
    }
    QString file = item->text(0);
    if(!clientThread->isRunning()) {
        clientThread->arglist[0] = file.toStdString();
        clientThread->task = TCd;
        clientThread->start();
    }

}

void FTPMainWindow::recvSuccess() {
    if(!connected) {
        connected = true;
        ui->connectButton->setText("disconnect");

    }
    else {
        connected = false;
        ui->connectButton->setText("connect");
        ui->status->setText("ready");
        ui->remotePathLabel->setText("remote path:");
    }
}
void FTPMainWindow::clientThreadRunning(){
    ui->status->setText("running");
    ui->status->setVisible(true);
}

void FTPMainWindow::recvInfo(QString info) {
    //TODO:添加日志文件
    QTextCursor cursor = ui->infoEdit->textCursor();
    cursor.insertText(info);
    cursor.movePosition(QTextCursor::End);
    ui->infoEdit->setTextCursor(cursor);
}
void FTPMainWindow::updateRemotePath(QString remotePath){
    ui->remotePathLabel->setText("remote path:  "+remotePath);
}
void FTPMainWindow::recvClearList(){
    ui->remoteFileTree->clear();
}
void FTPMainWindow::recvListItem(QString name, QString time, QString type, QString size){

    QTreeWidgetItem* item = new QTreeWidgetItem(ui->remoteFileTree);
    item->setText(0, name);
    item->setText(1, time);
    item->setText(2, type);
    item->setText(3, size);
    ui->remoteFileTree->addTopLevelItem(item);
}

void FTPMainWindow::on_pauseButton_clicked()
{
    if(connected){
        if(clientThread->isRunning()){      // 暂停
            clientThread->curClient->stopCurrentTask();
            ui->pauseButton->setText("resume");
            ui->status->setText("paused");
        }else if(clientThread->curClient->getcurTask()!=0){          // 继续
            ui->status->setText("running");
            ui->pauseButton->setText("pause");
            ui->progressBar->setVisible(true);
            if(clientThread->curClient->getcurTask()==1){
                // 恢复下载
                clientThread->task = TDown;
                clientThread->start();
            }else if(clientThread->curClient->getcurTask()==2){
                // 恢复上传
                clientThread->task = TUp;
                clientThread->start();
            }
        }else{
            recvInfo("this is no work\n");
        }
    }else{
         recvInfo("please connect first\n");
    }
}

void FTPMainWindow::updateDownloadProcess(int process){
    ui->progressBar->setValue(process);
}

void FTPMainWindow::setProcessBarVIsibility(bool b){
    ui->progressBar->setVisible(b);
}

void FTPMainWindow::recvClientThreadStop(){
    ui->status->setText("ready");
}
