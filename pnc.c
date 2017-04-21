#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t buffer;
pthread_cond_t condc,  condp;
int limit = 0;

struct item {
	int value;
	int ctimer;
} citem[32];

void* producer(void *ptr) {
	while(1) {
		int ptimer = (rand() % 4) + 3;
		wait(ptimer);
		
		pthread_mutex_lock(&buffer);
		if(limit < 32)
			pthread_cond_wait(&condp, &buffer);
		citem[limit].value = rand();
		citem[limit].ctimer = (rand() % 7) + 2;
		limit++;
		pthread_cond_signal(&condc);
		pthread_mutex_unlock(&buffer);
	}
}

void* consumer(void *ptr) {
	while(1) {
		if(limit == 0)
			pthread_cond_wait(&condc, &buffer);
		else {
			pthread_mutex_lock(&buffer);
			printf("Consumer consumed: %d", citem[limit].value);
			wait(citem[limit].ctimer);
			pthread_cond_signal(&condp);
			pthread_mutex_unlock(&buffer);
		}
	}
}

int main () {

	pthread_t prod, cons;
	
	pthread_mutex_init(&buffer, NULL);
	pthread_cond_init(&condc, NULL);
	pthread_cond_init(&condp, NULL);
	
	int i = 0;
	for(i = 0; i < 10; i++) {
		pthread_create(&cons, NULL, consumer, NULL);
	}
	
	for(i = 0; i < 10; i++) {
		pthread_create(&prod, NULL, producer, NULL);
	}
	
	pthread_join(prod, NULL);
	pthread_join(cons, NULL);
	
	pthread_mutex_destroy(&buffer);
	pthread_cond_destroy(&condc);
	pthread_cond_destroy(&condp);
}