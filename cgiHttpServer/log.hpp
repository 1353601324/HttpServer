#pragma once

#include <iostream>
#include <ctime>

using namespace std;

#define LOG(info, msg) log(info, msg, __FILE__, __LINE__)

void log(const char* info, const char* msg, const char* file, int line)
{
    int t = time(nullptr);
    printf("[%s][%d][%s][%s][%d]\n", info, t, msg, file, line);
}
