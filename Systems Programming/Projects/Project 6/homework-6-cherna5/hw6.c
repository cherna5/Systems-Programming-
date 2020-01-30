#include "hw6.h"
#include <stdio.h>
#include<pthread.h>

//------------------------------------

//putting everything as a struct 
static struct Elavator{
    pthread_barrier_t barrier;
	pthread_barrier_t pBarrier;
	pthread_mutex_t lock;
    pthread_mutex_t plock;
    int current_floor;
    int direction;
    int occupancy;
    int start;
    int end;
    enum {ELEVATOR_ARRIVED=1, ELEVATOR_OPEN=2, ELEVATOR_CLOSED=3} state;
} e[ELEVATORS];

//=================================================================================
//initializing 
void scheduler_init() {	

    for (int i = 0; i < ELEVATORS; i++)
    {
        e[i].current_floor=0;	
        e[i].direction = -1;
        e[i].occupancy=0;
        e[i].start = -1;
        e[i].end = -1;	
        e[i].state=ELEVATOR_ARRIVED;
        pthread_mutex_init(&(e[i].lock), 0);    
        pthread_barrier_init(&(e[i].barrier), NULL, 2); 
        pthread_mutex_init(&(e[i].plock), 0); 
        pthread_barrier_init(&(e[i].pBarrier), NULL, 2); 
    }
}

//=================================================================================
void passenger_request(int passenger, int from_floor, int to_floor, 
        void (*enter)(int, int), 
        void(*exit)(int, int))
{	//to keep match of pass and ele 
    int s = passenger % ELEVATORS;

	pthread_mutex_lock(&(e[s].plock));

	e[s].start = from_floor;
	e[s].end = to_floor;
    //waiting for pass to enter ele & update count 
	pthread_barrier_wait(&(e[s].pBarrier));
    enter(passenger, s);
	e[s].occupancy++;

    //ele checking and waiting for pass before closing 
	pthread_barrier_wait(&(e[s].barrier));
	pthread_barrier_wait(&(e[s].pBarrier));

    //closing and decrement 
	exit(passenger, s);
	e[s].occupancy--;
    pthread_barrier_wait(&(e[s].barrier));

    //reset for next floor 
	e[s].start = -1;
	e[s].end = -1;
	pthread_mutex_unlock(&(e[s].plock));

}

//=================================================================================
void elevator_ready(int elevator, int at_floor, 
        void(*move_direction)(int, int), 
        void(*door_open)(int), void(*door_close)(int)) {                   

    pthread_mutex_lock(&e[elevator].lock);              

    //redifine because we need in form of int for comparisons 
    int start  = e[elevator].start;                      
    int end  = e[elevator].end;                        
    int direction = e[elevator].direction;                       
    int status = e[elevator].occupancy;                       

    //
    if (e[elevator].state == ELEVATOR_ARRIVED) 
    {

        if ((status == 0 && at_floor == start) || (status == 1 && at_floor == end)) 
        {//open at floor else close 
            door_open(elevator);                                 
            e[elevator].state = ELEVATOR_OPEN;             
            pthread_barrier_wait(&e[elevator].pBarrier);  
            pthread_barrier_wait(&e[elevator].barrier);  
        }
        else
        {
            e[elevator].state = ELEVATOR_CLOSED;         
        }
    }//if door still open after operation close 
    else if (e[elevator].state == ELEVATOR_OPEN) {
        door_close(elevator);                                     
        e[elevator].state = ELEVATOR_CLOSED;              
    }
    else 
    {
        //this is where at each floor waits to be directed to go to which floor to and from 
        if((at_floor  == 0) || (at_floor == FLOORS - 1)            ||           
            (status == 0 && direction == 1  && start < at_floor)     ||           
            (status == 1 && direction == 1  && end < at_floor)      ||           
            (status == 0 && direction == -1 && start > at_floor)    ||           
            (status == 1 && direction == -1 && end > at_floor))
            {            
                e[elevator].direction *= -1;                 
            }


        move_direction(elevator, e[elevator].direction);         
        e[elevator].current_floor = at_floor + e[elevator].direction;
        e[elevator].state = ELEVATOR_ARRIVED;     
    }

    pthread_mutex_unlock(&e[elevator].lock);            
}
