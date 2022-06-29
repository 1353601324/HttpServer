#pragma once
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <algorithm>
#include <ctype.h>
#include "log.hpp"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
using namespace std;

class Util
{
  public:
  static int ReadLine(int sock, string& str)
  {
      char ch = '0';
      while(ch != '\n')
      {
          ssize_t s = recv(sock, &ch, 1, 0);
          if(s > 0)
          {
              if(ch == '\r')
              {
                  recv(sock, &ch, 1, MSG_PEEK);
                  if(ch == '\n') recv(sock, &ch, 1, 0);
                  else ch = '\n';
              }
              str += ch;
          }
          else if(s == 0) {
            LOG("INFO", "client quit");
            //不break，浏览器发一些垃圾信息给我们就有bug
            return -1;
          }
          else {
            LOG("WARNING", "ReadLine recv error"); 
            return -1;
          }
      }
      return str.size();
  }
};


 







  
