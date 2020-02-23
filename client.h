#ifndef CLIENT_H
#define CLIENT_H
#include<QString>
#include<string>
#include<winsock2.h>
#include<iostream>
#include<fstream>
#include"infothread.h"

using namespace std;

const int PORT = 21;
const int BUFLEN = 1000;
const int DATABUFLEN = 1000;
const char* const DELIMITER = "\r\n"; //FTP命令结束符

class Client
{
private:
    string ipAddr, username, password;
    char* buf = new char[BUFLEN];
    char* databuf = new char[DATABUFLEN];
    SOCKADDR_IN serverAddr;
    SOCKET controlSocket;
    SOCKET dataSocket;
    long long int localFileLength;
    long long int remoteFileLength;

    int curTask=0;          // 1-down  2-up   0-没有
    bool execute=true;                       // 是否可以运行，false表示被暂停
    // 辅助函数
    int getStateCode();
    int getPortNum();
    int recvControl(int stateCode, string errorInfo="0");
    int executeCmd(string cmd);
    void removeSpace(string&);
    void splitRow(vector<string> &row, string rawrow);
    void getLocalFileSize(string fileName); //获取本地已下载的文件长度 用于断点续传
    int getRemoteFileSize(string fname);
    // 核心功能
    int listPwd();
    void updateRemotePath();
    int intoPasv();


public:
    Client();
    ~Client();

    void login(QString ipAddr, QString username, QString password);
    int connectServer();
    int disconnect();
    int changeDir(string tardir);
    int upFile(std::string localName);
    int downFile(std::string remoteName, std::string localDir);
    int deleteFile(std::string fname);

    inline void startTask(){execute=true; }
    inline void stopCurrentTask(){execute=false;}
    inline bool isRunningTask(){return execute;}
    inline int getcurTask(){return curTask;}

    InfoThread *infoThread;
    std::vector<std::vector<std::string>> filelist;
};

#endif // CLIENT_H
