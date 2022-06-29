#pragma once
#include <iostream>
#include <cstdio>
#include <string>
#include <sys/wait.h>
#include <cstdlib>
#include <vector>
#include "log.hpp"
#include "Util.hpp"
#include <sstream>
#include <unistd.h>
#include <map>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define Home_Page "index.html"
#define Web_Root "wwwroot"

#define OK 200
#define NOT_FOUND 404
#define BAD_REQUEST 400

using namespace std;

string code2Desc(int code)
{
    switch(code)
    {
      case 200:
        return "OK";
      case 404:
        return "NOT_FOUND";
      default:
        return "";
        break;
    }
}

map<string, string> suffixDesc = {{".html", "text/html"}, {".css", "text/css"}, {".js", "application/javascript"}, {".jpg", "application/x-jpg"}, {".xml", "application/xml"}};

string suffix2Desc(string suffix)
{
    if(suffixDesc.find(suffix) != suffixDesc.end()) return suffixDesc[suffix];
    else return "text/html";
}

class Respond
{
  public:
    string status_line;
    vector<string> respond_header;
    string blank;
    string respond_body;

    string version;
    int status_code;//状态码
    string codeDesc;
    int size;

    string suffix;
    Respond() :blank("\r\n"){}
};

class Request
{
  public:
    string request_line;
    vector<string> request_header;
    string request_body;

    //请求行
    string method;
    string uri;
    string version;
    
    //请求报头
    map<string, string> header;
    string Content_Length;
    string Content_Type;

    //uri
    string path;
    string args;

    bool cgi;
    Request() :Content_Length("0"), cgi(false){}
};

class EndPoint
{
  public:
    pthread_t tid;
    int sock;
    Request* request;
    Respond* respond;
    bool stop;
  public:
    EndPoint(int _sock);
    ~EndPoint();

    //
    void recvRequest();
    bool hasBody();
    void debug_recvRequest();
    bool recvRequestLine();
    bool recvRequestHeader();
    bool recvRequestBody();

    //
    void buildRespond();
    void buildRespondHelper(int code);
    void buildOKRespond();
    void build404Respond();

    //cgi
    int processCgi();
    void processNonCgi(int code);

    //
    void sendRespond();

    //
    void parsePathAndArgs();
    void parseRequestLine();
    void parseRequestHeader();
    void parseRequest();
};

EndPoint::EndPoint(int _sock) :sock(_sock), stop(false)
{                     
    request = new Request();
    respond = new Respond();
}

EndPoint::~EndPoint()
{
    delete request;
    close(sock);
}

void EndPoint::recvRequest()
{
    //printf("%p\n", (int*)tid);
    if(recvRequestLine() || recvRequestHeader())
    {
        //recv error
        cout << "recv error" << endl;
    }
    else
    {
        cout << "stop debug: " << stop << endl;
        parseRequest();
        recvRequestBody();
    }
}

void EndPoint::debug_recvRequest()
{
    char buf[10000];
    ssize_t s = recv(sock, buf, sizeof buf, 0);
    if(s > 0)
    {
        cout << buf << endl;
    }
    else if(s == 0)
    {
        cout << "client quit" << endl;
    }
    else
    {
        cout << "recv error" << endl;
    }
}

void EndPoint::buildRespond()
{
    //如果是GET方法，就要分割路径和参数
    //并且判断路径的合法性和是否要使用cgi
    //cout << request->method << endl;
    if(request->method == "GET")
    {
        parsePathAndArgs();
        string path = Web_Root;

        if((request->path[request->path.size() - 1]) == '/')
        {
            //目录，返回主页
            request->path += Home_Page;
            path += request->path;
            request->path = path;
            struct stat buf;
            stat(request->path.c_str(), &buf);
            //该目录下index.html的大小,是respond的正文大小
            respond->size = buf.st_size;
            respond->status_code = OK;
            //cout << request->path << endl;
        }
        else
        {
            path += request->path;
            request->path = path;
            struct stat buf;
            if(stat(request->path.c_str(), &buf) == 0)//如果路径存在
            {
                if(S_ISDIR(buf.st_mode))//如果这个路径还是目录
                {
                    request->path += "/";
                    request->path += Home_Page;
                    //cout << request->path << endl;
                    respond->status_code = OK;
                    //该目录下index.html的大小,是respond的正文大小
                    respond->size = buf.st_size;
                }
                else 
                    //不是目录, 但仍然是静态网页，因为没有？
                {
                    size_t pos = request->uri.find('?');
                    //cout << request->path << endl;
                    //cout << "pos is: " << pos << endl;
                    if(pos == string::npos)
                    {
                        //cout << request->path << endl;
                        respond->size = buf.st_size;//资源的大小，也就是respond正文的大小
                        //cout << __FILE__ << __LINE__ << " " << respond->size << endl;
                        respond->status_code = OK;
                    }
                    else//cgi
                    {
                        request->cgi = true;
                    }
                }
            }
            else//路径不存在
            {
                LOG("WARNING", "path is not existed");
                respond->status_code = NOT_FOUND;
            }
        }
    }
    else if(request->method == "POST")
    {
        //post,先不管
        //bug,如果post请求的路径不存在还没有处理
        //post一定是cgi，路径要给对,不要加上cgi，浏览器自己会加
        request->cgi = true;
        string root = Web_Root;
        string path = root + request->uri;
        request->path = path;
        respond->status_code = OK;
    }
    else
    {
        //除了post和get之外的暂时不处理了
        LOG("WARNING", "method is not right");
        respond->status_code = NOT_FOUND;
    }

    //如果访问的资源路径不存在,构建ErrorRespond
    if(respond->status_code == NOT_FOUND) buildRespondHelper(respond->status_code);
    else
    {
        //有这些资源，先处理着，看一下code，如果正常就构建正常的respond，不正常就构建errorRepsond
            if(request->cgi)
            {
                respond->status_code = processCgi();
                //cout << respond->status_code << __LINE__ << endl;
            }
            else
            {
                //其实noncgi并不用处理，可以直接构建respond了
                //processNonCgi();
            }     
            buildRespondHelper(respond->status_code);
    }
}

void EndPoint::buildRespondHelper(int code)
{
    switch(code)
    {
        case 200:
            buildOKRespond();
            break;
        case 404:
            build404Respond();
            break;
    }
}

void EndPoint::buildOKRespond()
{
    //bug, cgi传回来的respond_body的类型不知道,因为目前cgi返回的都是html，因此暂时也不需要这个功能。
    if(request->cgi)
    {
        respond->version = request->version;
        respond->status_line = respond->version + ' ' + to_string(respond->status_code) + ' ' + respond->codeDesc + "\r\n";
        respond->respond_header.push_back("Content-Length: " + to_string(respond->respond_body.size()) + "\r\n");
        respond->respond_header.push_back("Content-Type: text/html\r\n");
    }
    else//非cgi
    {
        processNonCgi(200); 
    }
}

void EndPoint::build404Respond()
{
    respond->version = request->version;
    respond->status_line = respond->version + ' ' + to_string(respond->status_code) + ' ' + code2Desc(respond->status_code) + "\r\n";
    struct stat st;
    request->path = Web_Root;
    request->path += "/404_NOT_FOUND.html";
    stat(request->path.c_str(), &st);
    respond->size = st.st_size;
    respond->respond_header.push_back("Content-Length: " + to_string(respond->size) + "\r\n");
    respond->respond_header.push_back("Content-Type: text.html\r\n");
    request->cgi = false;
}


int EndPoint::processCgi()
{
    int code = 200;
    int output[2];
    int input[2];

    if(pipe(output) == -1)
    {
        LOG("ERROR", "output pipe create error");
        code = 404;
        exit(1);
    }
    if(pipe(input) == -1)
    {
        LOG("ERROR", "input pipe create error");
        code = 404;
        exit(1);
    }
    //千万不要一创建好管道就开始关闭fd，fork之后才能关
    string env_method = "METHOD=" + request->method;
    string env_content_length = "CONTENT_LENGTH=" + request->Content_Length;
    putenv((char*)env_method.c_str());//为了cgi程序可以知道如何处理，传入方法环境变量
    putenv((char*)env_content_length.c_str());//加入正文长度的环境变量给cgi程序，方便它读取正文
    if(fork() == 0)
    {
        //child
        close(output[1]), close(input[0]);//关闭子进程的无用fd
        dup2(output[0], 0), dup2(input[1], 1);//重定向，原因替换之后原来的fd数据就消失了
        if(request->method == "GET")
        {
            string env_args = "ARGS=" + request->args;
            putenv((char*)env_args.c_str());
        }
        cerr << request->path << __LINE__ << endl;
        if(execl(request->path.c_str(), nullptr) < 0)
        {
            cerr << "execl error" << endl;
        }//程序替换成cgi程序
    }
    else
    {
        close(output[0]), close(input[1]);//关闭httpServer（父进程）的两个无用fd
        if(request->method == "POST")
        {
            write(output[1], request->request_body.c_str(), request->request_body.size());
        }

        char ch;
        while(read(input[0], &ch, 1) > 0) respond->respond_body.push_back(ch); 
        int st;
        waitpid(-1, &st, 0);//等价于wait(&st);
        if(WIFEXITED(st)) code = 200;
        else code = 404;
    }
    return code;
}

void EndPoint::processNonCgi(int code)
{
    respond->version = request->version;
    respond->status_code = code;
    respond->codeDesc = code2Desc(200);

    //cout << respond->version << endl << respond->status_code << endl << respond->codeDesc << endl;

    respond->status_line = respond->version + ' ' + to_string(respond->status_code) + ' ' + respond->codeDesc + "\r\n";
    //cout << respond->status_line << endl;
    //respond header 还没有处理
    size_t pos = request->path.rfind('.');
    if(pos != string::npos) respond->suffix = request->path.substr(pos);
    else respond->suffix = ".html";
    //cout << respond->suffix << __LINE__ << endl;
    respond->respond_header.push_back("Content-Length: " + to_string(respond->size) + "\r\n");
    respond->respond_header.push_back("Content-Type: " + suffix2Desc(respond->suffix) + "\r\n");
    //cout << __LINE__ << ' ' << "enter" << endl;
    //respond_body在sendRespond那里直接发送出去,不在这处理了
}

void EndPoint::sendRespond()
{
    if(request->cgi == false)//非cgi的把首页文件打开并发出去
    {
        int fd = open(request->path.c_str(), O_RDONLY);
        //LOG("INFO", request->path.c_str());
        send(sock, respond->status_line.c_str(), respond->status_line.size(), 0);
        for(size_t i = 0; i < respond->respond_header.size(); i++)
        {
            string s = respond->respond_header[i];
            send(sock, s.c_str(), s.size(), 0);
        }
        send(sock, respond->blank.c_str(), respond->blank.size(), 0);
        sendfile(sock, fd, 0, respond->size);
        close(fd);
    }
    else//cgi的直接把respond的body发出去
    { 
        send(sock, respond->status_line.c_str(), respond->status_line.size(), 0);
        for(size_t i = 0; i < respond->respond_header.size(); i++)
        {
            string s = respond->respond_header[i];
            //cout << s << endl;
            send(sock, s.c_str(), s.size(), 0);
        }
        send(sock, respond->blank.c_str(), respond->blank.size(), 0);
        send(sock, respond->respond_body.c_str(), respond->respond_body.size(), 0);
    }
}

bool EndPoint::recvRequestLine()
{
    string s;
    if(Util::ReadLine(sock, s) > 0)
    {
        s.pop_back();//把\n删掉，因为我要打印这行出来，带着endl打印太混乱了。
        LOG("INFO", s.c_str());
        request->request_line = s;
    }
    else
    {
        stop = true;
    }
    //cout << __LINE__ << "STOP DEBUG : " << stop << endl; 
    return stop;
}

void EndPoint::parsePathAndArgs()
{
    size_t pos = request->uri.find('?');
    if(pos == string::npos) request->path = request->uri;
    else
    {
        request->path = request->uri.substr(0, pos);
        request->args = request->uri.substr(pos + 1);
    }
}

bool EndPoint::recvRequestHeader()
{
    string s;
    //int i = 0;
    while(true)
    {
        s.clear();
        if(Util::ReadLine(sock, s) < 0)
        {
            stop = true;
            //cout << __LINE__ << "STOP DEBUG : " << stop << endl; 
            break;
        }
        //cout << "i : " << i << ' ' << s;
        //i++;
        if(s == "\n") break;
        s.pop_back();//方便打印，带着endl不方便打印
        request->request_header.push_back(s);
        LOG("INFO", s.c_str());
    }
    return stop;
}

bool EndPoint::hasBody()
{
    if(request->method == "POST")
    {
        request->Content_Length = request->header["Content-Length"];
        return true;
    }
    return false;
}

void EndPoint::parseRequestLine()
{
    string &method = request->method, &uri = request->uri, &version = request->version;
    stringstream ss;
    ss << request->request_line;
    ss >> method >> uri >> version;
    for(size_t i = 0; i < method.size(); i++) method[i] = toupper(method[i]); //处理method输入大小写问题
}

void EndPoint::parseRequestHeader()
{
    for(size_t i = 0; i < request->request_header.size(); i++)
    {
        string s = request->request_header[i];
        int pos = s.find(": ");
        request->header.insert({s.substr(0, pos), s.substr(pos + 2)});            
    }
}

void EndPoint::parseRequest()
{
    parseRequestLine();
    parseRequestHeader();
}

bool EndPoint::recvRequestBody()
{
    if(hasBody())
    {
        //如果没有正文，直接就退出，因为构造的时候已经初始化为0了
        int tmp = stoi(request->Content_Length);
        while(tmp--)
        {
            char ch;
            ssize_t s = recv(sock, &ch, 1, 0);
            if(s < 0)
            {
                LOG("ERROR", "recv request body error");
                stop = true;
                break;
            }
            else request->request_body.push_back(ch);
        }
        LOG("INFO", request->request_body.c_str());
    }
    return stop;
}

class Handler
{
  public:
    void handler(int sock)
    {
        EndPoint* ep = new EndPoint(sock);
        //LOG("INFO", "recv request");
        cout << "-------begin----------------------" << endl;
        ep->recvRequest();
        //ep->debug_recvRequest();
        cout << "-----------end-------------------" << endl;
        if(ep->stop == true)
        {
            LOG("WARNING", "Recv Request error, stop build and send Respond");
            delete ep;
            return;
        }
        ep->buildRespond();
        ep->sendRespond();
        delete ep;
        return;
    }
};
