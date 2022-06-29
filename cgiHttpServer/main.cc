#include "httpServer.hpp"

int main(int argc, char* argv[])
{
   if(argc != 2)
   {
      cout << "参数数量不对" << endl;
      exit(5);
   }
   HttpServer* server = new HttpServer(atoi(argv[1]));
   server->HttpServerInit();
   server->Loop();
}
