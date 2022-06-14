//
// Created by Iddo Petrank on 13/06/2022.
//

#ifndef OS_WET_3_QUEUE_H
#define OS_WET_3_QUEUE_H

#include <stdbool.h>
#include <sys/time.h>

typedef struct Queue *Queue;
typedef struct Node *Node;

Node nodeCreate(int fd);
Queue queueCreate(int max_size);
int dequeue(Queue queue);
void enqueue(Queue queue, int fd);
void queueDestroy(Queue queue);
void removeIndex(Queue queue, int index);
bool queueFull(Queue queue);
bool queueEmpty(Queue queue);
void queueDropRandom(Queue queue);
struct timeval queueHeadArrivalTime(Queue queue);
void queuePrint(Queue queue);
void dropTail(Queue queue);
void removeValue(Queue queue, int value);
int getIndexOfValue(Queue queue, int value);
int queueSize(Queue queue);





#endif //OS_WET_3_QUEUE_H
