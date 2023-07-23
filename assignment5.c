#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define numPhilo 5

/* Gaussian function from Mccamish                                     */
/* successive calls to randomGaussian produce integer return values */
/* having a gaussian distribution with the given mean and standard  */
/* deviation.  Return values may be negative.                       */
int randomGaussian(int mean, int stddev) {
    double mu = 0.5 + (double) mean;
    double sigma = fabs((double) stddev);
    double f1 = sqrt(-2.0 * log((double) rand() / (double) RAND_MAX));
    double f2 = 2.0 * 3.14159265359 * (double) rand() / (double) RAND_MAX;
    if (rand() & (1 << 5)) 
        return (int) floor(mu + sigma * cos(f2) * f1);
    else            
        return (int) floor(mu + sigma * sin(f2) * f1);
}

int actions() {
    //Create semaphores
    int chopsticks = semget(IPC_PRIVATE, 5 , IPC_CREAT | IPC_EXCL | 0600);
    if(chopsticks < 0) {
        printf("%s\n", strerror(errno));
    }

    //Set choptsicks
    for(int i = 0; i < numPhilo; i++) {
        int set = semctl(chopsticks, i, SETVAL, 1);
        if(set < 0) {
            printf("%s\n", strerror(errno));
        }
    }

    //Create 5 child processes(philosophers)
    int pid;
    for(int philosopher = 0; philosopher<numPhilo; philosopher++) {
        if((pid = fork()) == 0) {
            int i = philosopher;
            int left; 
            int right;

            //Deadlock solution
            if(i < numPhilo-1) { 
                left = i;
                right = i+1; 
            }
            else { 
                left = 0;
                right = numPhilo-1;
            }

            //Operations to perform on semaphores (using them like quadrants)
            struct sembuf pickRight[1] = {{right, -1, 0}};
            struct sembuf pickLeft[1] = {{left, -1, 0}};
            struct sembuf dropRight[1] = {{right, 1, 0}};
            struct sembuf dropLeft[1] = {{left, 1, 0}};

            //Seeds rng for rand
            srand(getpid());

            int total_eating = 0;
            int total_thinking = 0;
            int eat_time;
            int think_time;
            int cycles = 0;

            //Stop when philosopher eats for 100 seconds
            while(total_eating < 100) { 
                cycles = cycles + 1;

                //Philosopher thinking
                think_time = randomGaussian(11, 7);
                   if(think_time < 0) { think_time = 0; }

                //Simulate time spent thinking
                printf("Philosopher %d is thinking for %d seconds (total = %d)\n", philosopher, think_time, total_thinking);
                sleep(think_time);
                total_thinking += think_time;

                //Pick up chopsticks
                int leftStick = semop(chopsticks, pickLeft, 1);
                int rightStick = semop(chopsticks, pickRight, 1);

                // semop error check
                if(leftStick < 0 || rightStick < 0) {
                    printf("%s\n", strerror(errno));
                }

                //Philosopher eating
                eat_time = randomGaussian(9, 3); 
                if(eat_time < 0) { eat_time = 0; }

                //Simulate time spent eating
                printf("Philosopher %d is eating for %d seconds (total = %d)\n", philosopher, eat_time, total_eating);
                sleep(eat_time); 
                total_eating += eat_time;

                //Put down chopsticks
                leftStick = semop(chopsticks, dropLeft, 1);
                rightStick = semop(chopsticks, dropRight, 1);

                //semop error check
                if(leftStick < 0 || rightStick < 0) {
                    printf("%s\n", strerror(errno));
                } 
            }
            printf("Philosopher %d done with meal (process %d)\n", philosopher, getpid());

            //Print results for each philosopher
            printf("Philosopher %d thought for %d seconds, ate for %d seconds over %d cycles.\n", 
                philosopher, total_thinking, total_eating, cycles);     
            exit(0);      
        }
        else if (pid < 0) { //fork error check
            printf("%s\n", strerror(errno));
        }
    }
    for(int i = 0; i < numPhilo; i++) {
         wait(NULL);
    }
   
    //Clean up IPC artifacts (remove semaphores)
    int rm = semctl(chopsticks, 0, IPC_RMID, 0);
    if(rm < 0) {
        printf("%s\n", strerror(errno));
    } 
} 

int main(int argc, char *argv[]){
    actions();
    return 0;
}
