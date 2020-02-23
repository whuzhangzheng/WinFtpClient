#include "client.h"

Client::Client()
{
    infoThread = new InfoThread();
}
Client::~Client(){
    delete infoThread;
}

void Client::login(QString ipAddr, QString username, QString password){
    this->ipAddr = ipAddr.toStdString();
    this->username = username.toStdString();
    this->password = password.toStdString();
}

int Client::connectServer() {
    WSADATA dat;
    int ret;
    cout<<"connect server"<<endl;
    //1.1 初始化
    if (WSAStartup(MAKEWORD(2,2),&dat)!=0)  //Windows Sockets Asynchronous启动
    {
        cout<<"Init Failed: "<<GetLastError()<<endl;
        infoThread->sendInfo("Init Failed!\n");
        return -1;
    }

    //flag = -1;

    //1.2 创建controlSocket
    controlSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(controlSocket==INVALID_SOCKET)
    {
        cout<<"Creating Control Socket Failed: "<<GetLastError()<<endl;
        infoThread->sendInfo("Creating Control Socket Failed.\n");
        return -1;
    }

    //1.3 构建服务器访问参数结构体
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.S_un.S_addr=inet_addr(ipAddr.c_str()); //地址
    serverAddr.sin_port=htons(PORT);            //端口
    memset(serverAddr.sin_zero,0,sizeof(serverAddr.sin_zero));

    //2. 连接
    ret=connect(controlSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
    if(ret==SOCKET_ERROR)
    {
        cout<<"Control Socket Connecting Failed: "<<GetLastError()<<endl;
        infoThread->sendInfo("Control Socket Connecting Failed\n");
        return -1;
    }
    cout<<"Control Socket connecting is success."<<endl;

    //接收返回状态信息
    Sleep(300);
    recvControl(220);

    //输入用户名
    executeCmd("USER " + username);
    recvControl(331);

    //输入密码
    executeCmd("PASS " + password);
    recvControl(230);

    listPwd();

    return 0;
}

int Client::disconnect() {
    executeCmd("QUIT");
    recvControl(221);       // 退出网络

    filelist.clear();
    memset(buf, 0, BUFLEN);
    memset(databuf, 0, DATABUFLEN);

    closesocket(controlSocket);
    closesocket(dataSocket);

    int flag = WSACleanup();
    if(flag==0){
        infoThread->sendInfo("close socket successfully");
    }else{
        int errorCode = WSAGetLastError();
        infoThread->sendInfo("colse socket failed, error code is"+errorCode);
    }
    return flag;
}

// 显示远程文件列表
int Client::listPwd(){
    updateRemotePath();
    // 打开被动默认：与服务器发送的数据端口连接
    intoPasv();
    // 获取当前目录下所有文件信息
    executeCmd("LIST -al");
    recvControl(150);       // 打开连接
    //list -al 的结果将通过数据端口发送
    memset(databuf,0, DATABUFLEN);
    string allfile;
    int ret = recv(dataSocket, databuf, DATABUFLEN-1, 0);
    while(ret>0){
        databuf[ret]='\0';
        allfile += databuf;
        ret = recv(dataSocket, databuf, DATABUFLEN-1, 0);
    }
    removeSpace(allfile);
    //cout<<"fileList:\n"<<fileList<<endl;
    vector<string> row;
    string rawrow;
    int p = allfile.find("\r\n"),lastp=0;
    // 清空filelist里面的过期数据
    filelist.clear();
    while(p>0){
        rawrow = allfile.substr(lastp,p-lastp);
        //infoThread->sendInfo(rawrow+'\n');

        row.clear();
        splitRow(row, rawrow);
        filelist.push_back(row);
        lastp = p+2;
        p = allfile.find("\r\n", lastp);
    }
    closesocket(dataSocket);
    recvControl(226);

}
// 显示远程路径
void Client::updateRemotePath(){
    executeCmd("PWD");
    recvControl(257);       // 路径名建立
    // 解析buf中的数据，并作为路径显示
    string result(buf);
    int from = result.find_first_of('/') -1;
    int len = result.size()-from;
    string remotePath = result.substr(from, len);
    infoThread->updateRemotePath(remotePath);
}

int Client::changeDir(string tardir) {
    memset(buf, 0, BUFLEN);
    executeCmd("CWD "+tardir);
    recvControl(250);
    listPwd();
    return 0;
}
int Client::intoPasv() {
    int dataPort, ret;
    //切换到被动模式
    executeCmd("PASV");
    recvControl(227);

    //返回的信息格式为---h1,h2,h3,h4,p1,p2
    //其中h1,h2,h3,h4为服务器的地址，p1*256+p2为数据端口
    dataPort=getPortNum();
    //客户端数据传输socket
    dataSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    serverAddr.sin_port=htons(dataPort);    //更改连接参数中的port值
    ret=connect(dataSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
    if(ret==SOCKET_ERROR)
    {
        cout<<"Data Socket connecting Failed: "<<GetLastError()<<endl;
        return -1;
    }
    cout<<"Data Socket connecting is success."<<endl;
    return 0;
}
// 下载
int Client::downFile(std::string remoteName, std::string localDir){
    curTask = 1;
    string localFile = localDir+"/"+remoteName;
    ofstream ofile;
    getRemoteFileSize(remoteName);
    getLocalFileSize(localFile);
    if(localFileLength == remoteFileLength){
        infoThread->sendInfo("file had been downloaded!\n");
        cout << "file had been downloaded!\n";
        infoThread->hideProcessBar();
        return 0;
    }else if(localFileLength<remoteFileLength){
        // 断点续传
        ofile.open(localFile, ios::binary|ios::app);
        intoPasv();
        executeCmd("TYPE I");
        recvControl(200);
        // 设置偏移量
        executeCmd("REST "+to_string(localFileLength));
        recvControl(350);
        infoThread->updateDownloadProcess(localFileLength*100/remoteFileLength);
        infoThread->showProcessBar();
        // 下载文件
        executeCmd("RETR "+remoteName);
        recvControl(150);
        memset(databuf, 0, DATABUFLEN);
        int ret=recv(dataSocket,databuf, DATABUFLEN, 0), count=0;
        while(ret>0){
            if(!isRunningTask()){  //任务被终止
                closesocket(dataSocket);
                recvControl(426);
                ofile.close();
                return -1;
            }
            ofile.write(databuf,ret);
            localFileLength += ret;
            ret = recv(dataSocket, databuf, DATABUFLEN, 0);
            count ++;
            if(count==10){
                infoThread->updateDownloadProcess(localFileLength*100/remoteFileLength);
                count =0;
            }
        }
        ofile.close();
        infoThread->sendInfo(remoteName+" has been downloaded.\n");
        cout << remoteName+" has been downloaded.\n";
        closesocket(dataSocket);
        recvControl(226);
        infoThread->hideProcessBar();//下载完成后关闭下载进度条s
        curTask = 0; // 下载完成，也就没有了被暂停的任务
        return 0;

    }
}

// 上传
int Client::upFile(string localName){
    curTask = 2;
    ifstream ifile;
    string remoteName = localName.substr(localName.find_last_of('/')+1);
    getLocalFileSize(localName); // 得到本地文件大小
    getRemoteFileSize(remoteName);

    if(remoteFileLength == localFileLength){
        infoThread->sendInfo(remoteName + " has yet been uploaded \n");
        infoThread->hideProcessBar();
        return 0;
    }else if(remoteFileLength<localFileLength){
        intoPasv();
        ifile.open(localName, ios::binary);
        if(ifile) { cout << "open file successfully\n" ; } else {cout << "open file fail\n";}
        executeCmd("TYPE I");
        recvControl(200);
        // 追加模式
        executeCmd("APPE "+remoteName);
        recvControl(150);
        ifile.seekg(remoteFileLength);
        // 显示进度条
        infoThread->showProcessBar();
        infoThread->updateDownloadProcess(remoteFileLength * 100 / localFileLength);
        int count = 0;
        while(!ifile.eof()){
            if(!isRunningTask()){
                closesocket(dataSocket); //主动关闭数据socket
                infoThread->sendInfo("pause upload process\n");
                recvControl(426);
                ifile.close();
                return -1;
            }
            ifile.read(databuf, DATABUFLEN);
            int readLength = ifile.gcount(); //成功读出的数据
            send(dataSocket, databuf, readLength, 0);
            remoteFileLength += readLength;
            if(count==10){
                count = 0;
                infoThread->updateDownloadProcess(remoteFileLength*100/localFileLength);
            }
            count ++;
        }
        curTask = 0;
        ifile.close();
        infoThread->hideProcessBar();
        closesocket(dataSocket);

        recvControl(226);
        listPwd();
        infoThread->sendInfo("upload has been done!\n");
        return 0;
    }
}

int Client::deleteFile(std::string fname){
    executeCmd("DELE "+fname);
    recvControl(250);
    listPwd();
    return 0;
}

//private function---------------------------------------------------------

// 执行ftp命令，并cout
int Client::executeCmd(string cmd) {
    cmd += "\r\n";
    int cmdlen = cmd.size();
    infoThread->sendInfo(cmd);
    cout << cmd;
    send(controlSocket, cmd.c_str(), cmdlen, 0);
    return 0;
}

// 检查从服务器返回的状态码
int Client::recvControl(int stateCode, string errorInfo) {
    memset(buf, 0, BUFLEN);
    recv(controlSocket, buf, BUFLEN, 0);
    if(getStateCode()==stateCode){
        infoThread->sendInfo(buf);
        return 0;
    }else{
        string s = getStateCode()+" The StateCode is Error!\n";
        infoThread->sendInfo(s);
        cout<<s<<endl;
        return -1;
    }
}

//从返回信息中获取状态码
int Client::getStateCode()
{
    int num=0;
    char* p = buf;
    while(p != nullptr)
    {
        num=10*num+(*p)-'0';
        p++;
        if(*p==' ' || *p=='-')
        {
            break;
        }
    }
    return num;
}

//从返回信息“227 Entering Passive Mode (182,18,8,37,10,25).”中获取数据端口
int Client::getPortNum()
{
    int num1=0,num2=0;

    char* p=buf;
    int cnt=0;
    while( 1 )
    {
        if(cnt == 4 && (*p) != ',')
        {
            if(*p<='9' && *p>='0')
                num1 = 10*num1+(*p)-'0';
        }
        if(cnt == 5)
        {
            if(*p<='9' && *p>='0')
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
    std::cout<<"The data port number is "<<num1*256+num2<<std::endl;
    return num1*256+num2;
}

void Client::removeSpace(string & src) {
    //空白符只保留一个
    int p, q;
    p = src.find(' ');
    while(p>=0) {
        for(q=p+1; src[q]==' '; q++);
        src.erase(p+1, q-p-1);
        p = src.find(' ', p+1);
    }
}

void Client::splitRow(vector<string> &row, string rawrow){
    int p, lastp=0;
    string name, tmp;
    for(int i=0; i<8;i++){
        p = rawrow.find(' ',lastp);
        tmp = rawrow.substr(lastp,p-lastp);
        lastp = p+1;
        row.push_back(tmp);
    }
    name = rawrow.substr(p+1,rawrow.size()-p);
    row.push_back(name);

}

int Client::getRemoteFileSize(string fname) {
    executeCmd("SIZE "+fname);
    if(recvControl(213)==-1){  // 231-文件状态回复
        // 远程没有此文件
        infoThread->sendInfo("upload new file\n");
        remoteFileLength = 0;
        return 0;
    }
    infoThread->sendInfo(string("remote filesize(raw): ")+buf);
    char* p = buf;
    while(p != nullptr && *p != ' ') {
        p++;
    }
    p++;
    int num = 0;
    while(p != nullptr && *p != '\r') {
        num *= 10;
        num += (*p - '0');
        p++;
    }
    memset(buf, 0, BUFLEN);
    remoteFileLength = num;
    return num;

}

void Client::getLocalFileSize(std::string fileName){//获取本地已下载的文件长度 用于断点续传
    localFileLength = 0;
    ifstream ifile;
    ifile.open(fileName,ios::binary);
    if(ifile){
        ifile.seekg(0,ifile.end);
        int length = ifile.tellg();
        localFileLength = length>0?length:0;
        ifile.close();
    }else{
        infoThread->sendInfo("error! local file length fail.\n");
    }

}

