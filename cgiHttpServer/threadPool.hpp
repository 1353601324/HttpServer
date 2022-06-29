#pragma once

#include "protocol.hpp"
#include <iostream>
#include <pthread.h>
#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>

using namespace std;

class Task
{
    int sock;
    Handler h;
  public:
      Task(int _sock) :sock(_sock) {}
      Task(){}
      void handler()
      {
          h.handler(sock);
      }
};

class threadPool
{
    public:
        // pthread_mutex_t lock;
        // pthread_cond_t cond;
        mutex mtx;
        condition_variable cv;
        size_t cap;
        queue<Task> q;

    threadPool()
    {
        cap = 5;
        // pthread_mutex_init(&lock, nullptr);
        // pthread_cond_init(&cond, nullptr);
        // for(size_t i = 0; i < cap; i++)
        // {
        //     pthread_t tid;
        //     pthread_create(&tid, nullptr, routine, this);
        // }
        vector<thread> v(cap);
        for(int i = 0; i < 5; i++)
        {
            v[i] = thread([&]{
                while(true)
                {
                    Task t;
                    get(t);
                    t.handler();
                }
            });
        }
        for(int i = 0; i < 5; i++) v[i].detach();
    }

    ~threadPool()
    {
        // pthread_mutex_destroy(&lock);
        // pthread_cond_destroy(&cond);
    }

    // static void* routine(void* arg)
    // {
    //     threadPool* threadpool = (threadPool*)arg;
    //     while(true)
    //     {
    //        Task t;
    //        threadpool->get(t);
    //        t.handler();
    //     }
    // }

    void put(Task& task)
    {
        //pthread_mutex_lock(&lock);
        unique_lock<mutex> lck(mtx);
        cv.wait(lck, [&]{return q.size() < cap;});
        q.push(task);
        //pthread_mutex_unlock(&lock);
        //pthread_cond_signal(&cond);
        cv.notify_one();
    }

    void get(Task& task)
    {
        //pthread_mutex_lock(&lock);
        unique_lock<mutex> lck(mtx);
        //while(q.size() == 0) pthread_cond_wait(&cond, &lock);
        cv.wait(lck, [&]{return q.size() > 0;});
        //cout << __LINE__ << "ENTER" << endl;
        task = q.front();
        q.pop();
        //pthread_mutex_unlock(&lock);
    }
};
