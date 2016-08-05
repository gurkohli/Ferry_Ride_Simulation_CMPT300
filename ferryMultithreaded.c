#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/resource.h>
#include <errno.h>
#include <wait.h>
#include <unistd.h>

/* Constants */
#define TOTAL_SPOTS_ON_FERRY 6
#define MAX_LOADS 11
#define CROSSING_TIME 1000000

/* Threads to create and control vehiclepthreads */
pthread_t vehicleThread;
pthread_t vehicleThreadProcesses[300];
pthread_t captainThread;
int threadCounter = 0;

/* Counters and their corresponding Mutexes */
pthread_mutex_t protectCarsQueued;
int carsQueuedCounter = 0;
pthread_mutex_t protectTrucksQueued;
int trucksQueuedCounter = 0;
pthread_mutex_t protectCarsUnloaded;
int carsUnloadedCounter = 0;
pthread_mutex_t protectTrucksUnloaded;
int trucksUnloadedCounter = 0;

/* Counting semaphores to manage cars and trucks */
sem_t carsQueued;
sem_t trucksQueued;
sem_t carsLoaded;
sem_t trucksLoaded;
sem_t carsUnloaded;
sem_t trucksUnloaded;
sem_t vehiclesSailing;
sem_t vehiclesArrived;
sem_t waitUnload;
sem_t readyUnload;
sem_t waitToExit;

//Constants declaration
struct timeval startTime;

void* captain_process();
void* vehicle_process();
void* truck();
void* car();
void init();
void clean();
int timeChange( const struct timeval startTime );

int maxTimeToNextArrival;
int truckArrivalProb; 

int terminateSimulation = 0;

//Threads functions declaration
int sem_waitChecked(sem_t *semaphoreID);
int sem_postChecked(sem_t *semaphoreID);
int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value);
int sem_destroyChecked(sem_t *semaphoreID);
int pthread_mutex_lockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID);
int pthread_mutex_initChecked(pthread_mutex_t *mutexID,
                              const pthread_mutexattr_t *attrib);
int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID);
int timeChange( const struct timeval startTime );

void* captain_process() {
    int localThreadId;
    localThreadId = (int )pthread_self();
    //{
        /* Counting variables for cars and trucks to determine when ferry is full */
        int currentLoad = 0;
        int numberOfCarsQueued = 0;
        int numberOfTrucksQueued = 0;
        int numberOfTrucksLoaded = 0;
        int numberOfSpacesFilled = 0;
        int numberOfVehicles = 0;
        int counter = 0;
        printf("Captain process created - Thread ID = %d\n", localThreadId);
        while (currentLoad < MAX_LOADS) {
            numberOfTrucksLoaded = 0;
            numberOfSpacesFilled = 0;
            numberOfVehicles = 0;
	    printf("                                            ");
            printf("Captain has begin LOADING\n");
            while(numberOfSpacesFilled < TOTAL_SPOTS_ON_FERRY) {
                pthread_mutex_lockChecked(&protectTrucksQueued);
                pthread_mutex_lockChecked(&protectCarsQueued);

                numberOfTrucksQueued = trucksQueuedCounter;
                numberOfCarsQueued = carsQueuedCounter;
                
                pthread_mutex_unlockChecked(&protectCarsQueued);
                pthread_mutex_unlockChecked(&protectTrucksQueued);
                
                while(numberOfTrucksQueued > 0 && numberOfSpacesFilled < TOTAL_SPOTS_ON_FERRY && numberOfTrucksLoaded < 2) {
                    pthread_mutex_lockChecked(&protectTrucksQueued);
                    trucksQueuedCounter--;
		    printf("                                            ");
                    printf("Captain signals a TRUCK to load\n");
                    sem_postChecked(&trucksQueued);
                    pthread_mutex_unlockChecked(&protectTrucksQueued);

                    numberOfTrucksQueued--;
                    numberOfTrucksLoaded++;
                    numberOfSpacesFilled+=2;
                    numberOfVehicles++;
                }
                while(numberOfCarsQueued > 0 && numberOfSpacesFilled < TOTAL_SPOTS_ON_FERRY) {
                    pthread_mutex_lockChecked(&protectCarsQueued);
                    carsQueuedCounter--;
                    printf("                                            ");
                    printf("Captain signals a CAR to load\n");
                    sem_postChecked(&carsQueued);
                    pthread_mutex_unlockChecked(&protectCarsQueued);
                    numberOfCarsQueued--;
                    numberOfSpacesFilled++;
                    numberOfVehicles++;
                }
            }
            for(counter = 0; counter < numberOfTrucksLoaded; counter++) {
                sem_waitChecked(&trucksLoaded);
		printf("                                            ");
                printf("Captain knows TRUCK is loaded\n");
            }
            for(counter = 0; counter < numberOfVehicles - numberOfTrucksLoaded; counter++) {
                sem_waitChecked(&carsLoaded);
                printf("                                            ");
                printf("Captain knows CAR is loaded\n");
            }
	    printf("                                            ");
            printf("Captain - Ferry is FULL\n");
	    printf("                                            ");
            printf("Captain - Ferry is about to SAIL\n");
            for(counter = 0; counter < numberOfVehicles; counter++) {
		printf("                                            ");
                printf("Captain requests confirmation for SAILING from Vehicle %d\n",counter);
                sem_postChecked(&vehiclesSailing);
            }
	    printf("                                            ");
            printf("Captain - All confirmations recieved\n");
	    printf("                                            ");
	    printf("Captain - Ferry is SAILING\n");

	    usleep(CROSSING_TIME);

            for(counter = 0; counter < numberOfVehicles; counter++) {
	    	printf("                                            ");
                printf("Captain notifies Vehicle %d about ARRIVAL\n", counter);
                sem_postChecked(&vehiclesArrived);
            }
            for(counter = 0; counter < numberOfVehicles; counter++) {
                sem_waitChecked(&readyUnload);
            }
	    printf("                                            ");
	    printf("Captain - All vehicles are ready to UNLOAD\n");
	    printf("                                            ");
	    printf("Captain - Ferry is UNLOADING\n");
            for(counter = 0; counter < numberOfVehicles; counter++) {
                sem_postChecked(&waitUnload);
            }
            while(numberOfSpacesFilled > 0) {
                pthread_mutex_lockChecked(&protectCarsUnloaded);
                if( carsUnloadedCounter > 0 ) {
                    sem_waitChecked(&carsUnloaded);
                    carsUnloadedCounter--;
                    numberOfSpacesFilled--;
		    printf("                                            ");
                    printf("Captain knows CAR has UNLOADED\n");
                }
                pthread_mutex_unlockChecked(&protectCarsUnloaded);
                pthread_mutex_lockChecked(&protectTrucksUnloaded);
                if( trucksUnloadedCounter > 0 ) {
                    sem_waitChecked(&trucksUnloaded);
                    trucksUnloadedCounter--;
                    numberOfSpacesFilled-=2;
		    printf("                                            ");
                    printf("Captain knows TRUCK has UNLOADED\n");
                }
                pthread_mutex_unlockChecked(&protectTrucksUnloaded);
            }
	    printf("                                            ");
            printf("Captain - All vehicles have been UNLOADED\n");
            for(counter = 0; counter < numberOfVehicles; counter++) {
                sem_post(&waitToExit);
            }
	    printf("                                            ");
            printf("Captain - Ferry is EMPTY\n");
	    printf("                                            ");
            printf("Captain - Ferry is SAILING BACK\n");
            usleep(CROSSING_TIME);
	    printf("                                            ");
            printf("Captain - Ferry has returned and DOCKED\n");
            currentLoad++;
            if(currentLoad >= MAX_LOADS) {
		printf("\n-----------------------------------------------------\n");
		printf("More than %d loads have occured. Terminating Simulation\n",MAX_LOADS);
		printf("\n-----------------------------------------------------\n");
		terminateSimulation = 1;
		return 0;
	    } else {
                printf("                                            ");
            	printf("Captain is ready for Load %d\n", currentLoad+1);
	    }
        }
        exit(0);
   // }
}

void* truck() {
    int localThreadId;
    localThreadId = (int )pthread_self();
    
    pthread_mutex_lockChecked(&protectTrucksQueued);
    trucksQueuedCounter++;
    pthread_mutex_unlockChecked(&protectTrucksQueued);
    
    printf("TRUCK process created : Thread ID = %d \n", localThreadId);
    sem_waitChecked(&trucksQueued);
    printf("TRUCK with Thread ID: %d is LOADING\n", localThreadId);
    printf("TRUCK with Thread ID: %d has LOADED\n", localThreadId);
    sem_postChecked(&trucksLoaded);
    sem_waitChecked(&vehiclesSailing);
    printf("TRUCK with Thread ID: %d confirms it's about to SAIL\n", localThreadId);
    sem_waitChecked(&vehiclesArrived);
    printf("TRUCK with Thread ID: %d knows it has ARRIVED at the destination\n", localThreadId);
    sem_postChecked(&readyUnload);
    sem_waitChecked(&waitUnload);
    printf("TRUCK with Thread ID: %d is UNLOADING\n", localThreadId);
    pthread_mutex_lockChecked(&protectTrucksUnloaded);
    trucksUnloadedCounter++;
    pthread_mutex_unlockChecked(&protectTrucksUnloaded);
    printf("TRUCK with Thread ID: %d has UNLOADED\n", localThreadId);
    sem_postChecked(&trucksUnloaded);
    sem_waitChecked(&waitToExit);
    printf("TRUCK with Thread ID: %d exits\n", localThreadId);
    pthread_exit(0);
}


void* car() {
    int localThreadId;
    localThreadId = (int )pthread_self();
    
    pthread_mutex_lockChecked(&protectCarsQueued);
    carsQueuedCounter++;
    pthread_mutex_unlockChecked(&protectCarsQueued);
    
    printf("CAR process created : Thread ID = %d \n", localThreadId);
    sem_waitChecked(&carsQueued);
    printf("CAR with Thread ID: %d is LOADING\n", localThreadId);
    printf("CAR with Thread ID: %d has LOADED\n", localThreadId);
    sem_postChecked(&carsLoaded);
    sem_waitChecked(&vehiclesSailing);
    printf("CAR with Thread ID: %d confirms it's about to SAIL\n", localThreadId);
    sem_waitChecked(&vehiclesArrived);
    printf("CAR with Thread ID: %d knows it has ARRIVED at the destination\n", localThreadId);
    sem_postChecked(&readyUnload);
    sem_waitChecked(&waitUnload);
    printf("CAR with Thread ID: %d is UNLOADING\n", localThreadId);
    pthread_mutex_lockChecked(&protectCarsUnloaded);
    carsUnloadedCounter++;
    pthread_mutex_unlockChecked(&protectCarsUnloaded);
    printf("CAR with Thread ID: %d has UNLOADED\n", localThreadId);
    sem_postChecked(&carsUnloaded);
    sem_waitChecked(&waitToExit);
    printf("CAR with Thread ID: %d exits\n", localThreadId);
    pthread_exit(0);
}

void* vehicle_process() {
    int localThreadId;
    localThreadId = (int)pthread_self();

    struct timeval startTime;
    int elapsed = 0;
    int lastArrivalTime = 0;
    
    printf("Vehicle process created - Thread ID = %d\n", localThreadId);
    gettimeofday(&startTime, NULL);
    srand(time(0));
    while(terminateSimulation == 0) {
        if(elapsed > lastArrivalTime) {
	    lastArrivalTime += rand() % maxTimeToNextArrival;

            if(rand() % 100 < truckArrivalProb ) {
                // It's a truck process
                pthread_create(&(vehicleThreadProcesses[threadCounter]), NULL, truck, NULL);
            } else {
                // It's a car process
                pthread_create(&(vehicleThreadProcesses[threadCounter]), NULL, car, NULL);
            }
            printf("Present time %d, next arrival time %d\n", elapsed, lastArrivalTime);
        }
        elapsed = timeChange(startTime);
    }
    
    printf("Vehicle Process Ended\n");
    return 0;
}

void init() {
    
    sem_initChecked(&carsQueued, 0, 0);
    sem_initChecked(&trucksQueued, 0, 0);
    sem_initChecked(&carsLoaded, 0, 0);
    sem_initChecked(&trucksLoaded, 0, 0);
    sem_initChecked(&vehiclesSailing, 0, 0);
    sem_initChecked(&vehiclesArrived, 0, 0);
    sem_initChecked(&waitUnload, 0, 0);
    sem_initChecked(&readyUnload, 0, 0);
    sem_initChecked(&carsUnloaded, 0, 0);
    sem_initChecked(&trucksUnloaded, 0, 0);
    sem_initChecked(&waitToExit, 0, 0);
    printf("Initialized Semaphores\n\n");
}

int main() {
    init();

    printf("Please enter integer values for the following variables\n");

    printf("\nEnter the %% probability that the next vehicle is a truck:");
    scanf("%d", &truckArrivalProb );
    printf("\nEnter the maximum length of the interval between vehicles:");
    scanf("%d", &maxTimeToNextArrival );
    
    pthread_create(&vehicleThread, NULL, vehicle_process, NULL);
    pthread_create(&captainThread, NULL, captain_process, NULL);

    pthread_mutex_initChecked(&protectCarsQueued, NULL);
    pthread_mutex_initChecked(&protectTrucksQueued, NULL);
    pthread_mutex_initChecked(&protectCarsUnloaded, NULL);
    pthread_mutex_initChecked(&protectTrucksUnloaded, NULL);

    pthread_join(vehicleThread, NULL);
    pthread_join(captainThread, NULL);

    clean();
    return 0;
}

void clean() {
    
    pthread_mutex_destroyChecked(&protectTrucksQueued);
    pthread_mutex_destroyChecked(&protectCarsQueued);
    pthread_mutex_destroyChecked(&protectCarsUnloaded);
    pthread_mutex_destroyChecked(&protectTrucksUnloaded);
    sem_destroyChecked(&carsQueued);
    sem_destroyChecked(&trucksQueued);
    sem_destroyChecked(&carsLoaded);
    sem_destroyChecked(&trucksLoaded);
    sem_destroyChecked(&vehiclesSailing);
    sem_destroyChecked(&vehiclesArrived);
    sem_destroyChecked(&waitUnload);
    sem_destroyChecked(&readyUnload);
    sem_destroyChecked(&carsUnloaded);
    sem_destroyChecked(&trucksUnloaded);
    sem_destroyChecked(&waitToExit);
    printf("Cleaning Complete\n");
}

int timeChange( const struct timeval startTime ) {
    struct timeval nowTime;
    long int elapsed;
    int elapsedTime;
    gettimeofday(&nowTime, NULL);
    elapsed = (nowTime.tv_sec - startTime.tv_sec) * 1000000
    + (nowTime.tv_usec - startTime.tv_usec);
    elapsedTime = elapsed / 1000;
    return elapsedTime;
}

int sem_waitChecked(sem_t *semaphoreID) {
    int returnValue;
    returnValue = sem_wait(semaphoreID);
    if (returnValue == -1 ) {
        printf("Semaphore wait failed: simulation terminating\n");
        exit(0);
    }
    return returnValue;
}

int sem_postChecked(sem_t *semaphoreID) {
    int returnValue;
    returnValue = sem_post(semaphoreID);
    if (returnValue < 0 ) {
        printf("Semaphore post operation failed: simulation terminating\n");
        exit(0);
    }
    return returnValue;
}

int sem_initChecked(sem_t *semaphoreID, int pshared, unsigned int value) {
    int returnValue;
    returnValue = sem_init(semaphoreID, pshared, value);
    if (returnValue < 0 ) {
        printf("Semaphore init operation failed: simulation terminating\n");
        exit(0);
    }
    return returnValue;
}

int sem_destroyChecked(sem_t *semaphoreID) {
    int returnValue;
    returnValue = sem_destroy(semaphoreID);
    if (returnValue < 0 ) {
        printf("Semaphore destroy operation failed: simulation terminating\n");
        exit(0);
    }
    return returnValue;
}

int pthread_mutex_lockChecked(pthread_mutex_t *mutexID) {
    int returnValue;
    returnValue = pthread_mutex_lock(mutexID);
    if (returnValue < 0 ) {
        printf("pthread mutex lock operation failed: terminating\n");
        exit(0);
    }
    return returnValue;
}

int pthread_mutex_unlockChecked(pthread_mutex_t *mutexID) {
    int returnValue;
    returnValue = pthread_mutex_unlock(mutexID);
    if (returnValue < 0 ) {
        printf("pthread mutex unlock operation failed: terminating\n");
        exit(0);
    }
    return returnValue;
}

int pthread_mutex_initChecked(pthread_mutex_t *mutexID,
                              const pthread_mutexattr_t *attrib) {
    int returnValue;
    returnValue = pthread_mutex_init(mutexID, attrib);
    if (returnValue < 0 ) {
        printf("pthread init operation failed: simulation terminating\n");
        exit(0);
    }
    return returnValue;
}

int pthread_mutex_destroyChecked(pthread_mutex_t *mutexID) {
    int returnValue;
    returnValue = pthread_mutex_destroy(mutexID);
    if (returnValue < 0 ) {
        printf("pthread destroy failed: simulation terminating\n");
        exit(0);
    }
    return returnValue;
}
