#include "threadpool.h"

//线程池创建
threadpool_t *threadpool_create(int min_thr_num, int max_thr_num,
                                int queue_max_size) {
  threadpool_t *pool = NULL;
  do {
    if ((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
      printf("malloc threadpool fail\n");
      break;
    }

    pool->min_thr_num = min_thr_num;
    pool->max_thr_num = max_thr_num;
    pool->live_thr_num = min_thr_num;
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->queue_max_size = queue_max_size;
    pool->queue_size = 0;
    pool->busy_thr_num = 0;
    pool->wait_exit_thr_num = 0;
    pool->shutdown = false;

    //线程tid数组开辟空间
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * max_thr_num);
    if (pool->threads == NULL) {
      printf("malloc thread fail\n");
      break;
    }

    memset(pool->threads, 0, sizeof(pthread_t) * max_thr_num);

    //任务池开辟空间
    pool->task_queue =
        (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_max_size);
    if (pool->task_queue == NULL) {
      printf("malloc task_queue fail\n");
      break;
    }

    //锁初始化
    if (pthread_mutex_init(&pool->lock, NULL) != 0 ||
        pthread_mutex_init(&pool->thread_counter, NULL) != 0 ||
        pthread_cond_init(&pool->queue_not_empty, NULL) != 0 ||
        pthread_cond_init(&pool->queue_not_full, NULL) != 0) {
      printf("lock init fail\n");
      break;
    }

    //设置线程属性为分离
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (int i = 0; i < min_thr_num; i++) {
      pthread_create(&pool->threads[i], &attr, threadpool_thread, (void *)pool);
      printf("start thread 0x%x..\n", (unsigned int)pool->threads[i]);
    }

    //创建管理者线程
    pthread_create(&(pool->adjust_tid), &attr, adjust_thread, (void *)pool);

    return pool;

  } while (0);
  printf("%s fail\n", __func__);
  threadpool_free(pool);
  return NULL;
}
//管理者线程
void *adjust_thread(void *threadpool) {
  int i;
  threadpool_t *pool = (threadpool_t *)threadpool;

  while (!pool->shutdown) {
    sleep(DEFAULT_TIME);

    pthread_mutex_lock(&pool->lock);
    int queue_size = pool->queue_size;
    int live_thr_num = pool->live_thr_num;
    pthread_mutex_unlock(&pool->lock);

    pthread_mutex_lock(&pool->thread_counter);
    int busy_thr_num = pool->busy_thr_num;
    pthread_mutex_unlock(&pool->thread_counter);

    //创建线程 算法:任务数大于最小线程数,且存活线程少于最大线程个数时
    if (queue_size >= pool->min_thr_num && live_thr_num < pool->max_thr_num) {
      pthread_mutex_lock(&pool->lock);
      int add = 0;

      for (i = 0; i < pool->max_thr_num && add < DEFAULT_THREAD_VARY &&
                  pool->live_thr_num < pool->max_thr_num;
           i++) {
        if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i])) {
          pthread_create(&(pool->threads[i]), NULL, threadpool_thread,
                         (void *)pool);
          add++;
          pool->live_thr_num++;
        }
      }

      pthread_mutex_unlock(&pool->lock);
    }

    //销毁多余的空闲线程 算法:忙线程*2小于 存活的线程数且存活
    if ((busy_thr_num * 2) < live_thr_num && live_thr_num > pool->min_thr_num) {
      pthread_mutex_lock(&pool->lock);
      pool->wait_exit_thr_num = DEFAULT_THREAD_VARY;
      pthread_mutex_unlock(&pool->lock);

      for (i = 0; i < DEFAULT_THREAD_VARY; i++) {
        pthread_cond_signal(&pool->queue_not_empty);
      }
    }
  }

  return NULL;
}
//判断线程是否存在
int is_thread_alive(pthread_t tid) {
  int kill_rc = pthread_kill(tid, 0);
  if (kill_rc == ESRCH) {
    return false;
  }
  return true;
}
//线程池添加任务
int threadpool_add(threadpool_t *pool, void *(*function)(void *arg),
                   void *arg) {
  pthread_mutex_lock(&pool->lock);

  //任务队列已满,阻塞等待线程取
  while ((pool->queue_size == pool->queue_max_size) && (!pool->shutdown)) {
    pthread_cond_wait(&pool->queue_not_full, &pool->lock);
  }

  if (pool->shutdown) {
    pthread_cond_broadcast(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->lock);
    return 0;
  }

  //清空工作线程
  if (pool->task_queue[pool->queue_rear].arg != NULL) {
    pool->task_queue[pool->queue_rear].arg = NULL;
  }

  //添加任务
  pool->task_queue[pool->queue_rear].function = function;
  pool->task_queue[pool->queue_rear].arg = arg;
  pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;
  pool->queue_size++;

  //添加完任务 队列不为空,唤醒一个线程处理任务
  pthread_cond_signal(&pool->queue_not_empty);
  pthread_mutex_unlock(&pool->lock);

  return 0;
}
//子线程工作线程
void *threadpool_thread(void *threadpool) {
  threadpool_t *pool = (threadpool_t *)threadpool;
  threadpool_task_t task;

  while (true) {
    pthread_mutex_lock(&pool->lock);

    //无工作线程
    while ((pool->queue_size == 0) && (!pool->shutdown)) {
      printf("thread 0x%lx waitting\n", pthread_self());
      pthread_cond_wait(&pool->queue_not_empty, &pool->lock);

      if (pool->wait_exit_thr_num > 0) {
        pool->wait_exit_thr_num--;

        if (pool->live_thr_num > pool->min_thr_num) {
          pool->live_thr_num--;
          printf("thread 0x%lx exit\n", pthread_self());
          pthread_mutex_unlock(&pool->lock);
          pthread_exit(NULL);
        }
      }
    }
    if (pool->shutdown) {
      pthread_mutex_unlock(&pool->lock);
      printf("thread 0x%lx exit\n", pthread_self());
      pthread_exit(NULL);
    }

    //从任务队列队头取任务
    task.function = pool->task_queue[pool->queue_front].function;
    task.arg = pool->task_queue[pool->queue_front].arg;

    pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size;
    pool->queue_size--;

    //通知可以有新任务来
    pthread_cond_broadcast(&pool->queue_not_full);

    //任务取出立刻释放线程池
    pthread_mutex_unlock(&pool->lock);

    //执行任务
    printf("thread 0x%lx start working\n", pthread_self());
    pthread_mutex_lock(&pool->thread_counter);
    pool->busy_thr_num++;
    pthread_mutex_unlock(&pool->thread_counter);

    (*(task.function))(task.arg);

    //执行结束
    printf("thread 0x%lx end working\n", pthread_self());
    pthread_mutex_lock(&pool->thread_counter);
    pool->busy_thr_num--;
    pthread_mutex_unlock(&pool->thread_counter);
  }
}
//线程池摧毁
int threadpool_destroy(threadpool_t *pool) {
  if (pool == NULL) {
    return -1;
  }
  pool->shutdown = true;

  //唤醒所有阻塞线程,使其自动释放
  pthread_cond_broadcast(&pool->queue_not_empty);

  threadpool_free(pool);

  return 0;
}
//线程池释放
int threadpool_free(threadpool_t *pool) {
  if (pool == NULL) {
    return -1;
  }

  if (pool->task_queue) {
    free(pool->task_queue);
  }

  if (pool->threads) {
    free(pool->threads);
    pthread_mutex_lock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);
    pthread_mutex_lock(&(pool->thread_counter));
    pthread_mutex_destroy(&(pool->thread_counter));
    pthread_cond_destroy(&(pool->queue_not_empty));
    pthread_cond_destroy(&(pool->queue_not_full));
  }
  free(pool);
  pool = NULL;

  return 0;
}
