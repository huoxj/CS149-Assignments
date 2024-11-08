#include <stdio.h>
#include <thread>

#include "CycleTimer.h"

typedef struct {
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int* output;
    int threadId;
    int numThreads;
} WorkerArgs;


extern void mandelbrotSerial(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int numRows,
    int maxIterations,
    int output[]);

double pow(double x, int y) {
    double ret = 1;
    while(y > 0 && y--) {
        ret *= x;
    }
    return ret;
}


int loads[32];
int loads_start[32];

void getLoads(double eps, int threads, int total) {
    eps = 1.0 / eps;
    int loads_base = total * (1.0 - eps) / (1 - pow(eps, threads / 2)) / 2;
    int cur_loads = loads_base;
    for(int i = 0; i < (threads + 1) / 2 && total > 0; i++) {
        if(i == threads - 1 - i) {
            if(total < cur_loads) {
                loads[i] = total;
            }
            else {
                loads[i] = cur_loads;
                total -= cur_loads;
            }
        }
        else {
            if(total < 2 * cur_loads) {
                loads[i] = total / 2;
                loads[threads - 1 - i] = total - total / 2;
            }
            else {
                loads[i] = loads[threads - 1 - i] = cur_loads;
                total -= 2 * cur_loads;
            }
        }
        cur_loads *= eps;
    } 
    if(total > 0) {
        loads[0] += total / 2;
        loads[threads - 1] += total - total / 2;
    }
    for(int i = 1; i < threads; i++) {
        loads_start[i] = loads_start[i - 1] + loads[i - 1];
    }
    for(int i = 0; i < threads; i++) {
        printf("Thread %d: [%d, %d) -- %d\n", i, loads_start[i], loads_start[i] + loads[i], loads[i]);
    }
}

int lastLineNum = 0;
//
// workerThreadStart --
//
// Thread entrypoint.
void workerThreadStart(WorkerArgs * const args) {

    // TODO FOR CS149 STUDENTS: Implement the body of the worker
    // thread here. Each thread should make a call to mandelbrotSerial()
    // to compute a part of the output image.  For example, in a
    // program that uses two threads, thread 0 could compute the top
    // half of the image and thread 1 could compute the bottom half.

    // printf("Hello world from thread %d\n", args->threadId);
    
    // time clac
    double startTime = CycleTimer::currentSeconds();

    //int startRow = loads_start[args->threadId], 
    //numRows = loads[args->threadId];

    int startRow = args->height / args->numThreads * args->threadId,
    numRows = args->height / args->numThreads;
    mandelbrotSerial(
            args->x0, args->y0,
            args->x1, args->y1,
            args->width, args->height,
            startRow, numRows,
            args->maxIterations,
            args->output
            );   
    double endTime = CycleTimer::currentSeconds();
    printf("Thread %d total time: %.2fms\n", args->threadId, (endTime - startTime) * 1000);
}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations, int output[])
{
    static constexpr int MAX_THREADS = 32;

    if (numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    // Creates thread objects that do not yet represent a thread.
    std::thread workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    for (int i=0; i<numThreads; i++) {
      
        // TODO FOR CS149 STUDENTS: You may or may not wish to modify
        // the per-thread arguments here.  The code below copies the
        // same arguments for each thread
        args[i].x0 = x0;
        args[i].y0 = y0;
        args[i].x1 = x1;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].maxIterations = maxIterations;
        args[i].numThreads = numThreads;
        args[i].output = output;
      
        args[i].threadId = i;
    }

//    getLoads(2.5, numThreads, height);

    // Spawn the worker threads.  Note that only numThreads-1 std::threads
    // are created and the main application thread is used as a worker
    // as well.
    for (int i=1; i<numThreads; i++) {
        workers[i] = std::thread(workerThreadStart, &args[i]);
    }
    
    workerThreadStart(&args[0]);

    // join worker threads
    for (int i=1; i<numThreads; i++) {
        workers[i].join();
    }
}

