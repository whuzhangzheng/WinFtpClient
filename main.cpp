#include <QApplication>
#include<iostream>
#include<fstream>
#include<winsock2.h>

#define IP_ADDR "49.235.200.69"	//主机地址

using namespace std;
#include"ftpmainwindow.h"

int getStateCode(char* buf);
int getPortNum(char* buf);
bool executeFTPCmd(SOCKET controlSocket, char* buf, int len, int stateCode);
int test();

/*带完善的点：不能处理中文路径
 *
 **/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    FTPMainWindow w;
    w.show();
    return a.exec();

}

int test(){

    WSADATA dat;
    SOCKET controlSocket, dataSocket;
    SOCKADDR_IN serverAddr;
    int dataPort, ret;
    char buf[100]={0};


    //1.1 初始化
    if (WSAStartup(MAKEWORD(2,2),&dat)!=0)  //Windows Sockets Asynchronous启动
    {
        cout<<"Init Failed: "<<GetLastError()<<endl;
        return -1;
    }

    //1.2 创建controlSocket
    controlSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(controlSocket==INVALID_SOCKET)
    {
        cout<<"Creating Control Socket Failed: "<<GetLastError()<<endl;
        return -1;
    }

    //1.3 构建服务器访问参数结构体
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.S_un.S_addr=inet_addr(IP_ADDR); //地址
    serverAddr.sin_port=htons(21);            //端口
    memset(serverAddr.sin_zero,0,sizeof(serverAddr.sin_zero));

    //2. 连接
    ret=connect(controlSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
    if(ret==SOCKET_ERROR)
    {
        cout<<"Control Socket Connecting Failed: "<<GetLastError()<<endl;
//        infoThread->sendInfo("Control Socket Connecting Failed\n");
        return -1;
    }
    cout<<"Control Socket connecting is success."<<endl;

    //接收返回状态信息
    Sleep(300);

    recv(controlSocket,buf,100,0);
    cout<<buf;													//220
    //根据返回信息提取状态码，进行判断
    if(getStateCode(buf) != 220)
    {
        cout<<"Error: Control Socket connecting Failed"<<endl;
        system("pause");
        exit(-1);
    }

    // 用户名
    memset(buf,0,100);
    sprintf(buf,"USER %s\r\n","zz");
    executeFTPCmd(controlSocket, buf, 9, 331);				//331

    //密码
    memset(buf,0,100);
    sprintf(buf,"PASS %s\r\n","987654321");
    executeFTPCmd(controlSocket, buf, 16, 230);             // 这个数字是16， 表示指令的总长度，不能错

    //切换到被动模式
    memset(buf,0,sizeof(buf));
    sprintf(buf,"PASV\r\n");
    executeFTPCmd(controlSocket, buf, 6, 227);
    //返回的信息格式为---h1,h2,h3,h4,p1,p2
    //其中h1,h2,h3,h4为服务器的地址，p1*256+p2为数据端口
    dataPort=getPortNum(buf);

    // 客户端数据传输socket
    dataSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    serverAddr.sin_port = htons(dataPort);
    ret = connect(dataSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr));

    if(ret==SOCKET_ERROR)
    {
        cout<<"Data Socket connecting Failed: "<<GetLastError()<<endl;
        system("pause");
        return -1;
    }
    cout<<"Data Socket connecting is success."<<endl;

    // 更改当前目录
    memset(buf,0,100);
    sprintf(buf,"CWD %s\r\n","folder");	//250   只能一级一级地改目录
    executeFTPCmd(controlSocket, buf, 12, 250);

//    //上传文件
//    memset(buf,0,100);
//    sprintf(buf, "TYPE I\r\n");
//    executeFTPCmd(controlSocket, buf, 8, 200);

//    memset(buf,0,100);
//    sprintf(buf,"STOR %s\r\n","1.jpg");
//    cout<<"before stor"<<endl;
//    executeFTPCmd(controlSocket, buf, 11, 125);

//    fstream ifile;
//    ifile.open("1.jpg",ios::binary);
//    if(ifile) { cout << "open file successfully\n" ; } else {cout << "open file fail\n";}
//    cout<<"before open file"<<endl;
//    FILE *f = fopen("1.jpg","rb");
//    cout<<"after open file"<<endl;
//    if(f==NULL)
//        {
//            cout<<"The file pointer is NULL!"<<endl;
//            cout<<"Error: "<<__FILE__<<" "<<__LINE__<<endl;
//            exit(-1);
//        }

//    while(!feof(f)){
//        fread(sendBuf, 1, 1014, f);
//        send(dataSocket, sendBuf, 1024, 0);
//    }
//    fclose(f);

    //释放资源
    closesocket(dataSocket);
    closesocket(controlSocket);


}

bool executeFTPCmd(SOCKET controlSocket, char* buf, int len, int stateCode){
    send(controlSocket, buf, len, 0);

    memset(buf,0,100);
    recv(controlSocket,buf, 100, 0);

    if(getStateCode(buf) == stateCode){
        return true;
    }else{
        cout<<getStateCode(buf)<<"The StateCode is Error!"<<endl;
        return false;
    }

}
//从返回信息中获取状态码
int getStateCode(char* buf)
{
    int num=0;
    char* p=buf;
    while(p != NULL)
    {
        num=10*num+(*p)-'0';
        p++;
        if(*p==' ')
        {
            break;
        }
    }

    return num;
}

int getPortNum(char* buf){
    int num1=0,num2=0;

    char* p=buf;
    int cnt=0;
    while( 1 )
    {
        if(cnt == 4 && (*p) != ',')
        {
            num1 = 10*num1+(*p)-'0';
        }
        if(cnt == 5)
        {
            num2 = 10*num2+(*p)-'0';
        }
        if((*p) == ',')
        {
            cnt++;
        }
        p++;
        if((*p) == ')')
        {
            break;
        }
    }
    cout<<"The data port number is "<<num1*256+num2<<endl;
    return num1*256+num2;
}
