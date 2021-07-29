#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_TIME 10 /*10s检测一次*/
#define MIN_WAIT_TASK_NUM \
  10 /*如果queue_size > MIN_WAIT_TASK_NUM 添加新的线程到线程池*/
#define DEFAULT_THREAD_VARY 10 /*每次创建和销毁线程的个数*/
#define true 1
#define false 0

typedef struct {
  void *(*function)(void *arg); /* 函数指针，回调函数 */
  void *arg;                    /* 上面函数的参数 */
} threadpool_task_t;            /* 各子线程任务结构体 */

/* 描述线程池相关信息 */
struct threadpool_t {
  pthread_mutex_t lock;           /* 用于锁住本结构体 */
  pthread_mutex_t thread_counter; /* 记录忙状态线程个数de琐 -- busy_thr_num */

  pthread_cond_t
      queue_not_full; /* 当任务队列满时，添加任务的线程阻塞，等待此条件变量 */
  pthread_cond_t queue_not_empty; /* 任务队列里不为空时，通知等待任务的线程 */

  pthread_t *threads;   /* 存放线程池中每个线程的tid。数组 */
  pthread_t adjust_tid; /* 存管理线程tid */
  threadpool_task_t *task_queue; /* 任务队列(数组首地址) */

  int min_thr_num;       /* 线程池最小线程数 */
  int max_thr_num;       /* 线程池最大线程数 */
  int live_thr_num;      /* 当前存活线程个数 */
  int busy_thr_num;      /* 忙状态线程个数 */
  int wait_exit_thr_num; /* 要销毁的线程个数 */

  int queue_front;    /* task_queue队头下标 */
  int queue_rear;     /* task_queue队尾下标 */
  int queue_size;     /* task_queue队中实际任务数 */
  int queue_max_size; /* task_queue队列可容纳任务数上限 */

  int shutdown; /* 标志位，线程池使用状态，true或false */
};

typedef struct threadpool_t threadpool_t;

/**
 * @function threadpool_create
 * @descCreates a threadpool_t object.
 * @param thr_num  thread num
 * @param max_thr_num  max thread size
 * @param queue_max_size   size of the queue.
 * @return a newly created thread pool or NULL
 */
threadpool_t *threadpool_create(int min_thr_num, int max_thr_num,
                                int queue_max_size);

/**
 * @function threadpool_add
 * @desc add a new task in the queue of a thread pool
 * @param pool     Thread pool to which add the task.
 * @param function Pointer to the function that will perform the task.
 * @param argument Argument to be passed to the function.
 * @return 0 if all goes well,else -1
 */
int threadpool_add(threadpool_t *pool, void *(*function)(void *arg), void *arg);

/**
 * @function threadpool_destroy
 * @desc Stops and destroys a thread pool.
 * @param pool  Thread pool to destroy.
 * @return 0 if destory success else -1
 */
int threadpool_destroy(threadpool_t *pool);

/**
 * @desc get the thread num
 * @pool pool threadpool
 * @return # of the thread
 */
int threadpool_all_threadnum(threadpool_t *pool);

/**
 * desc get the busy thread num
 * @param pool threadpool
 * return # of the busy thread
 */
int threadpool_busy_threadnum(threadpool_t *pool);

void *threadpool_thread(void *threadpool);

void *adjust_thread(void *threadpool);

int is_thread_alive(pthread_t tid);
int threadpool_free(threadpool_t *pool);
#endif