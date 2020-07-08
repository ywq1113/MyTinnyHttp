#include <cstdio>
#include <iostream>
#include <functional>
#include "../thread.h"
#include "../threadData.h"
#include "../../base/CountDownLatch.h"

using namespace std;

class Thread;
struct threadData;

void printNum(int x)
{
    if(x < 0) printf("%d\n", x);
    for(int i = 0; i < x; ++i)
    {
        printf("%d\n", i);
    }
}

int main()
{
    //function<void(int)> func_ = printNum;
    Thread thread_(bind(&printNum, 5), "calThread");
    thread_.start();
    cout << thread_.name() << endl;
    //thread_.join();
}