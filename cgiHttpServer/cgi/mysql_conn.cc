#include <iostream>
#include "mysql.h"
#include <string>
#include <unistd.h>
#include <sys/types.h>
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

void InsertSql(string& sql)
{
    MYSQL* conn = mysql_init(nullptr);
    mysql_set_character_set(conn, "utf8");
    if(nullptr == mysql_real_connect(conn, "81.70.152.200", "http_test", "1353601324ERIC", "http_test", 3306, nullptr, 0))
    {
        cerr << "db connect error" << endl;
        return;
    }
    mysql_query(conn, sql.c_str());
    mysql_close(conn);
}

int main()
{
    string query_sql = getArgs();
    string sub1, sub2;
    cutArgs(sub1, sub2, query_sql, "&");
    string namekey, nameval, passwdkey, passwdval;
    cutArgs(namekey, nameval, sub1, "=");
    cutArgs(passwdkey, passwdval, sub2, "=");

    string sql = "insert into user (name, password) values";
    string values = "('" + nameval + "'" ",'" + passwdval + "');";
    sql += values;
    cerr << sql << endl;
    InsertSql(sql);
    cout << "<html>" << endl;
   cout << "<body>" << endl;
   cout << "<h2>insert success"  << endl;
   cout << "</body>" << endl;
   cout << "</html>";
}

