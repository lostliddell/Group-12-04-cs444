#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define THINKING  0
#define HUNGRY  1
#define EATING  2

int limit = 0;

pthread_mutex_t bowl;

struct philo_Struct {
	pthread_cond_t condp;
	int state;
	int blocktime;
} people[5];



// Algorithm to figure out which forks are being used or not
int abletoEat(int id) {
	int t;
	
	if(people[(id+4) % 5].state == EATING || people[(id+1) % 5].state == EATING) {
		return 0;
	}
	
	t = time(0);
	
	if(people[(id+4) % 5].state == HUNGRY && people[(id+4) % 5].blocktime < people[id].blocktime && (t - people[(id+4) % 5].blocktime) >= 5) {
		return 0;
	}
	
	if(people[(id+1) % 5].state == HUNGRY && people[(id+1) % 5].blocktime < people[id].blocktime && (t - people[(id+1) % 5].blocktime) >= 5) {
		return 0;
	}
	
	else {
		return 1;
	}
}

void getforks(int id) {
	pthread_mutex_lock(&bowl);
	people[id].state = HUNGRY;
	people[id].blocktime = time(0);
	while(!abletoEat(id)) {
		pthread_cond_wait(&people[id].condp, &bowl);
	}
	people[id].state = EATING;
	printf("Philosopher %d PICKS UP forks: %d and %d\n", id, id, ((id+4) % 5));
	pthread_mutex_unlock(&bowl);
}

void put_forks(int id) {
	pthread_mutex_lock(&bowl);
	printf("Philosopher %d PUTS DOWN forks: %d and %d\n", id, id, ((id+4) % 5));
	people[id].state = THINKING;
	if(people[(id+4) % 5].state == HUNGRY) {
		pthread_cond_signal(&people[(id+4) % 5].condp);
	}
	if(people[(id+1) % 5].state == HUNGRY) {
		pthread_cond_signal(&people[(id+1) % 5].condp);
	}
	pthread_mutex_unlock(&bowl);
}

void think(int id) {
	int thinkingtime = (rand() % 19) + 1;
	printf("Philosopher %d is THINKING\n", id);
	sleep(thinkingtime);
}

void eat(int id) {
	int eatingtime = (rand() % 7) + 2;
	printf("Philosopher %d is EATING\n", id );
	sleep(eatingtime);
}

void* philosopher(void *num){
	int id = *((int *) num);
	while( 1 ){
		
		think(id);
		getforks(id);
		eat(id);
		put_forks(id);

	}
} 

int main( int argc, char *argv[] ) {

	pthread_t philo[5];

	pthread_mutex_init(&bowl, NULL);

	int i = 0;
	int id[5] = {0,1,2,3,4};
	
	// Initializing some of struct values
	for(i = 0; i < 5; i++) {
		pthread_cond_init(&people[i].condp, NULL);
		people[i].state = THINKING;
		people[i].blocktime = 0;
	}
	
	for( i = 0; i < 5; i++ ){
		pthread_create( &philo[i], NULL, philosopher, (void *)(id + i));
	}

	for( i = 0; i < 5; i++ ){
		pthread_join( philo[i], NULL );
	}

	pthread_mutex_destroy( &bowl );
	for( i = 0; i < 5; i++ ){
		pthread_cond_destroy( &people[i].condp );
	}	
}