#pragma once
#include "tcpServer.hpp"
#include "protocol.hpp"
#include <pthread.h>
#include <signal.h>
#include "threadPool.hpp"
class HttpServer
{
  private:
    int port;
    threadPool* threadpool;
    tcpServer* server;
  public:
    HttpServer(int _port = 8080) :port(_port) {}

    ~HttpServer(){}

    void HttpServerInit()
    {
        //信号SIGPIPE需要进行忽略，如果不忽略，服务器发送respond（写入），读端关闭了fd，服务器会被sigpipe信号杀掉
        signal(SIGPIPE, SIG_IGN);
        threadpool = new threadPool;
        server = new tcpServer(port);
        server->tcpServerInit();
    }

    void Loop()
    {
        for(;;)
        {
            int lsock = server->getLsock();
            int sock = Sock::Accept(lsock);
            LOG("INFO", "get a new link");
            Task task(sock);
            //httpServer永远放任务就好了，这个任务就是收和发数据。具体怎么完成的它不用管
            threadpool->put(task);
        }
    }
};

