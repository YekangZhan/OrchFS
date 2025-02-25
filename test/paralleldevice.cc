#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include <syspes.h>
#include <assert.h>
#include <atomic>
using namespace std;
//#include "tools.h"

#define IOCOUNT (1024 * 1024LL)
#define IOSIZE (4 * 1024LL)
#define THREADS (1)

double time_array[THREADS] = {0};
atomic_uint64_t GLO_OFFSET(0);

double run_worker(int thread_id)
{
    void *readbuf = (void *)valloc(IOSIZE);
    int fddev = open("/dev/nvme0n1", O_RDWR | O_DIRECT);
    assert(fddev > 0);
    size_t count = IOCOUNT / THREADS;

    tic(thread_id);
    for (size_t i = 0; i < count; i++)
    {
        size_t ret = pread64(fddev, readbuf, IOSIZE, GLO_OFFSET.fetch_add(IOSIZE));
        // int a = 0;
        // perror("aa");
        assert(ret == IOSIZE);
    }
    double time = toc(thread_id);
    time_array[thread_id] = time;

    free(readbuf);
    return time;
}

int main()
{
    tic(100);
    thread worker_tid[THREADS];
    for (size_t th = 0; th < THREADS; th++)
    {
        worker_tid[th] = thread(run_worker, th);
    }
    for (size_t th = 0; th < THREADS; th++)
    {
        worker_tid[th].join();
    }
    double time_total = toc(100);

    // double time_total = 0;
    // for (size_t i = 0; i < THREADS; i++)
    // {
    //     time_total += time_array[i];
    // }

    double T = (double)time_total / (1000 * 1000 * 1000LL);
    double C = (double)(IOCOUNT * IOSIZE) / (1024 * 1024);
    cout << "TOTAL(MB): " << C << " TIME(s): " << T << endl;
    cout << "BAND(MB/s): " << C / T << endl;

    return 0;
}
