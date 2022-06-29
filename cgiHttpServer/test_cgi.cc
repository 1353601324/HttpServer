#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "log.hpp"
using namespace std;

string getArgs()
{
   string method = getenv("METHOD");
   string content_length = getenv("CONTENT_LENGTH");

   string args;
   if(method == "GET")
   {
       args = getenv("ARGS"); 
   }
   else if(method == "POST")
   {
       int sz = stoi(content_length);
       char ch;
       while(sz--)
       {
           read(0, &ch, 1);
           args += ch;
       }
   }
   else
   {
     //...不管
   }

   return args;
}

void cutArgs(string& s, string& t, string& args, string sep)
{
    size_t pos = args.find(sep);
    if(pos == string::npos) s = args;
    else s = args.substr(0, pos), t = args.substr(pos + 1);
}

int main()
{
    string args = getArgs();
    string sub1, sub2;
    string name1, val1, name2, val2;
    cutArgs(sub1, sub2, args, "&");
    cutArgs(name1, val1, sub1, "=");
    cutArgs(name2, val2, sub2, "=");

   cerr << name1 << " : " << val1 << endl;
   cerr << name2 << " : " << val2 << endl;
   
   int x = stoi(val1), y = stoi(val2);

   cout << "<html>" << endl;
   cout << "<body>" << endl;
   cout << "<h2>val1 + val2 = " + to_string(x + y) << endl;
   cout << "<h2>val1 - val2 = " + to_string(x - y) << endl;
   cout << "<h2>val1 * val2 = " + to_string(x * y) << endl;
   cout << "<h2>val1 / val2 = " + to_string(x / y) << endl;
   cout << "</body>" << endl;
   cout << "</html>";
}
