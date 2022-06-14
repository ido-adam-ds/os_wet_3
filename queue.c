//
// Created by Iddo Petrank on 13/06/2022.
//
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

struct Node{
    int fd;
    struct timeval arrival;
    Node next;
};

struct Queue{
    int max_size;
    int curr_size;
    Node dummy_head;
    Node tail;
};

struct timeval queueHeadArrivalTime(Queue queue)
{
    return queue->dummy_head->next->arrival;
}
Queue queueCreate(int max_size)
{

    Queue new_queue = (Queue)malloc(sizeof(struct Queue));
    new_queue->curr_size = 0;
    new_queue->max_size = max_size;
    new_queue->dummy_head = nodeCreate(-1);
    new_queue->tail =  new_queue->dummy_head;
    return new_queue;
}

Node nodeCreate(int fd)
{
    Node new_node = (Node) malloc(sizeof(struct Node));
    gettimeofday(&(new_node->arrival), NULL);
    new_node->fd  = fd;
    new_node->next = NULL;
    return new_node;
}

bool queueFull(Queue queue)
{
    return queue->max_size == queue->curr_size;
}

bool queueEmpty(Queue queue)
{
    return  queue->curr_size == 0;
}


int dequeue(Queue queue)
{
    if (queueEmpty(queue))
        return -1;

    Node to_remove = queue->dummy_head->next;
    int fd = to_remove->fd;
    queue->dummy_head->next = to_remove->next;
    free(to_remove);
    queue->curr_size--;
    return fd;
}

void enqueue(Queue queue, int fd)
{
    if (queueFull(queue))
        return;

    Node new_node = nodeCreate(fd);
    if (queueEmpty(queue))
    {
        queue->dummy_head->next = new_node;
        queue->tail = new_node;
    }
    else
    {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }
    queue->curr_size++;
}

void queueDestroy(Queue queue)
{
    while(!queueEmpty(queue))
        dequeue(queue);
    free(queue->dummy_head);
    free(queue);
}

void removeIndex(Queue queue, int index)
{
    if(index >= queue->curr_size || index < 0)
        return;
    Node temp = queue->dummy_head;
    for(int i = 0; i < index; i++)
        temp = temp->next;

    Node to_delete = temp->next;
    temp->next = to_delete->next;
    free(to_delete);
    queue->curr_size--;
}


void queueDropRandom(Queue queue)
{
    int num_to_drop = ceil(queue->curr_size*0.3);
    for(int i = 0; i < num_to_drop; i++)
        removeIndex(queue, (int)rand() % (queue->curr_size));

}

void queuePrint(Queue queue)
{
    if (queueEmpty(queue)){
        printf("Queue is empty\n");
        return;
    }

    Node temp = queue->dummy_head->next;
    while (temp){
        printf("%d ", temp->fd);
        temp = temp->next;
    }
    printf("\n");
}

void dropTail(Queue queue)
{
    if(queueEmpty(queue))
        return;
    removeIndex(queue, queue->curr_size-1);
}

int getIndexOfValue(Queue queue, int value)
{
    if (queueEmpty(queue))
        return -1;
    Node iter = queue->dummy_head->next;
    int i = 0;
    for(;i<queue->curr_size;i++)
    {
        if (iter->fd == value)
            break;
        iter = iter->next;
    }
    return (i == queue->curr_size)?-1:i;
}


void removeValue(Queue queue, int value)
{
    if (queueEmpty(queue))
        return;
    removeIndex(queue, getIndexOfValue(queue, value));

}


int queueSize(Queue queue)
{
    return queue->curr_size;
}
