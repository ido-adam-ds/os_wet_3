#include "segel.h"
#include "request.h"
#include "queue.h"
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//
/*
sprintf(buf, "%sStat-Req-Arrival:: %lu.%06lu\r\n", buf,stats.arrival_time.tv_sec,stats.arrival_time.tv_usec);

sprintf(buf, "%sStat-Req-Dispatch:: %lu.%06lu\r\n", buf,,stats.dispatch_interval.tv_sec,stats.dispatch_interval.tv_usec);

sprintf(buf, "%sStat-Thread-Id:: %d\r\n", buf, stats.handler_thread_stats.handler_thread_id);

sprintf(buf, "%sStat-Thread-Count:: %d\r\n", buf,stats.handler_thread_stats.handler_thread_req_count);

sprintf(buf, "%sStat-Thread-Static:: %d\r\n", buf,stats.handler_thread_stats.handler_thread_static_req_count);

sprintf(buf, "%sStat-Thread-Dynamic:: %d\r\n\r\n", buf,stats.handler_thread_stats.handler_thread_dynamic_req_count
*/

typedef enum {BLOCK, DT, DH, RANDOM} sched_algo;

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to .
       tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

pthread_mutex_t m;
pthread_cond_t master_cond;//bad names but one is for when we try to add another job to full queue
pthread_cond_t worker_cond;//and the other is for when we read from empty queue

int* static_thread_count;
int* dynamic_thread_count;
int* total_thread_count;



Queue wait_queue = NULL;
Queue worker_queue = NULL;

// HW3: Parse the new arguments too
void getargs(int *port, int argc, char *argv[], int* threads_num, int* queue_size, sched_algo * schedalg)
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threads_num = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    if(strcmp(argv[4], "block") == 0)
        *schedalg = BLOCK;
    else if(strcmp(argv[4], "dt") == 0)
        *schedalg = DT;
    else if(strcmp(argv[4], "dh") == 0)
        *schedalg = DH;
    else if(strcmp(argv[4], "random") == 0)
        *schedalg = RANDOM;

}



void* thread_func(void *args)
{
	int* temp = (int*) args;
	int index = *temp;
//	printf("index: %d\n", index);

    while(1){
        pthread_mutex_lock(&m);
        while (queueEmpty(wait_queue))
            pthread_cond_wait(&worker_cond, &m);
	//printf("about to handle\n");
        struct timeval arrive = queueHeadArrivalTime(wait_queue);
        int fd = dequeue(wait_queue);
        enqueue(worker_queue, fd);
        pthread_mutex_unlock(&m);

        struct timeval end, dispatch;
        gettimeofday(&end, NULL);
        timeval_subtract(&dispatch, &end, &arrive);
        requestHandle(fd, index, dynamic_thread_count, static_thread_count, total_thread_count, arrive, dispatch);
        close(fd);


        pthread_mutex_lock(&m);
        removeValue(worker_queue, fd);
        pthread_cond_signal(&master_cond);
        pthread_mutex_unlock(&m);
    }
    return NULL;
}



int main(int argc, char *argv[])
{

    srand(time(0));
    int listenfd, connfd, port, clientlen, threads_num, queue_size;
    struct sockaddr_in clientaddr;
    sched_algo schealg;


    getargs(&port, argc, argv, &threads_num, &queue_size, &schealg);
    wait_queue = queueCreate(queue_size);
    worker_queue = queueCreate(queue_size);


    // 
    // HW3: Create some threads...
    //
	int indexes[threads_num];
    for(int i = 0; i < threads_num; i++){
        indexes[i] = i;
    }
 
    pthread_t* threads = (pthread_t*) malloc(sizeof(pthread_t)*threads_num);
    for(int i = 0; i < threads_num; i++){

       
        pthread_create(&threads[i], NULL, thread_func, (void*)(&indexes[i]));//TODO create thread func that handles requests
    }
    dynamic_thread_count = (int*) malloc(sizeof(int)*threads_num);
    static_thread_count = (int*) malloc(sizeof(int)*threads_num);
    total_thread_count = (int*) malloc(sizeof(int)*threads_num);
    


    for(int i = 0; i < threads_num; i++){
        dynamic_thread_count[i] = 0;
        static_thread_count[i] = 0;
        total_thread_count[i] = 0;
        indexes[i] = i;
    }

    //TODO intiate locks
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&worker_cond, NULL);
    pthread_cond_init(&master_cond, NULL);


    listenfd = Open_listenfd(port);
   // int i = 0;
    while (1) {
      //  printf("infinte loop: %d\n", i);
	//i++;
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
      //  printf("cought\n");

        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.
        //

        pthread_mutex_lock(&m);
	    if(queueSize(wait_queue)+queueSize(worker_queue) >= queue_size)
		{
			if(schealg == BLOCK)
			{
				while (queueSize(wait_queue)+queueSize(worker_queue) >= queue_size)
                	pthread_cond_wait(&master_cond, &m);
			}
			else if(schealg == RANDOM)
			{
				if(queueEmpty(wait_queue))
				{
                	close(connfd);
                	pthread_mutex_unlock(&m);
                	continue;
				}
				else {
					queueDropRandom(wait_queue);
				}
			}
			else if(schealg == DT)
			{
               	close(connfd);
               	pthread_mutex_unlock(&m);
                continue;
			}
			else if(schealg == DH)
			{
				if(queueEmpty(wait_queue))
				{
                	close(connfd);
                	pthread_mutex_unlock(&m);
                	continue;
				}
				else{
					close(dequeue(wait_queue));		
				}
			}	
		}
							
			
			
       
/* if(schealg == BLOCK)
        {
            while (queueSize(wait_queue)+queueSize(worker_queue) == queue_size)
                pthread_cond_wait(&master_cond, &m);
            enqueue(wait_queue, connfd);
            pthread_cond_signal(&worker_cond);
        }
        else if(schealg == RANDOM)
        {
            if(queueSize(wait_queue)+queueSize(worker_queue) == queue_size){
				if(queueEmpty(wait_queue))
				{
                	close(connfd);
                	pthread_mutex_unlock(&m);
                	continue;
				}
                queueDropRandom(wait_queue);
			}
            enqueue(wait_queue, connfd);
            pthread_cond_signal(&worker_cond);
        }
        else if(schealg == DT)
        {
            if(queueSize(wait_queue)+queueSize(worker_queue) == queue_size)
				{
                	close(connfd);
                	pthread_mutex_unlock(&m);
                	continue;
				}
            enqueue(wait_queue, connfd);
            pthread_cond_signal(&worker_cond);
        }
        else if(schealg == DH)
        {
            if(queueSize(wait_queue)+queueSize(worker_queue) == queue_size)
            {
				if(queueSize(wait_queue) == 0)
				{
                	close(connfd);
                	pthread_mutex_unlock(&m);
                	continue;
				}
				else{
					close(dequeue(wait_queue));
				}
            }

        }*/
      //  pthread_cond_signal(&worker_cond);
        enqueue(wait_queue, connfd);
        pthread_cond_signal(&worker_cond);
        pthread_mutex_unlock(&m);
  //  	printf("end\n");


    }
    

}


    


 
