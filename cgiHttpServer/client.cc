#include <iostream>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <mutex>
#include <vector>

using namespace std;

class Client
{
    public:
        int sock;
        int port;
        string ip;
        string method;
        vector<string> httpRequest;
        vector<string> httpRespond;

        Client(string, int, string);
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        static Client* getInstance();
        void buildHttpRequest(int a, int b);
        void sendHttpRequest();
        void recvHttpRequest();
        static void run(Client*);

        static Client* ptr;
        static mutex mtx;
};
Client* Client::ptr = nullptr;
mutex Client::mtx;

static Client* getInstance(string _ip, int _port, string _method)
{
    if(Client::ptr == nullptr)
    {
        Client::mtx.lock();
        if(Client::ptr == nullptr)
        {
            Client::ptr = new Client(_ip, _port, _method);
        }
        Client::mtx.unlock();
    }
    return Client::ptr;
}

Client:: Client(string _ip, int _port, string _method)
{
    port = _port;
    ip = _ip;
    method = _method;
    sock = socket(AF_INET, SOCK_STREAM, 0); 
    struct sockaddr_in peer;
    peer.sin_family = AF_INET;
    peer.sin_port = htons(port);
    peer.sin_addr.s_addr = inet_addr(ip.c_str());
    if(connect(sock, (sockaddr*)&peer, sizeof(peer)) < 0)
    {
        cout << "connect error" << endl;
        exit(-1);
    }
}

void Client:: buildHttpRequest(int a, int b)
{
    string requestLine;
    if(method == "GET")
    {
        requestLine = method + " /test_cgi?data1=" + to_string(a) + "&data2=" + to_string(b) + " HTTP/1.1\r\n";
        //请求报头和空行和正文都不要了
        httpRequest.push_back(requestLine);
        httpRequest.push_back("\r\n");//blank line
        //cout << "build done" << endl;
    }
    else//POST
    {
        requestLine = method + " /test_cgi HTTP/1.1\r\n";
        string body = "data1=" + to_string(a) + "&" + "data2=" + to_string(b) + "\r\n";
        string contentLength = "Content-Length: " + to_string(body.size()) + "\r\n";
        httpRequest.push_back(requestLine);
        httpRequest.push_back(contentLength);
        httpRequest.push_back("\r\n");//blank line
        httpRequest.push_back(body);
    }
}

void Client::sendHttpRequest()
{
    for(int i = 0; i < httpRequest.size(); i++)
    {
        //cout << "sending ..." << endl;
        //cout << httpRequest[i] << endl;
        send(sock, httpRequest[i].c_str(), httpRequest[i].size(), 0);
        //cout << "send done" << endl;
    }
}

void Client::recvHttpRequest()
{
    char buf[1024] = {0};
    while(recv(sock, buf, sizeof buf, 0) > 0)
        httpRespond.push_back(buf);
    for(int i = 0; i < httpRespond.size(); i++)
        cout << httpRespond[i] << endl;
}

static void run(Client* client)
{
    client->buildHttpRequest(10, 20);
    client->sendHttpRequest();
    client->recvHttpRequest();
}


int main(int argc, char* argv[])
{
    string ip = argv[1];
    int port = atoi(argv[2]);
    string method = "POST";
    Client* client = getInstance(ip, port, method);
    run(client);
}


