#include <time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

struct memory{
    int32_t size;
    sem_t sem_1;
    sem_t sem_2;
    sem_t query_sem;
    u_int8_t max_users_connected;
    u_int8_t users_connected;
    u_int8_t client_disconnect;
    sem_t client_connecting;
    int32_t serching_val;
    int32_t count_of_serching_val;
    int32_t first_index;
    int32_t count_of_query;
};
struct config{
    struct memory * data;
    u_int8_t *server_running;
    sem_t server_shutdown;
    sem_t sem_server_running;
};
void *communication(void *);

int main() {

    int fd= shm_open("shared_memory",O_CREAT|O_EXCL|O_RDWR,0666);
    if(fd==-1){
        if(errno==EEXIST){
            shm_unlink("shared_memory");
            printf("restart\n");
        }else{
            printf("Nie mozna utworzyc pamieci wspoldzielonej\n");
        }
        return 1;
    }
    ftruncate(fd,sizeof(struct memory));
    struct memory *data=(struct memory *) mmap(0,sizeof(struct memory),PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);


    sem_init(&data->sem_1,1,0);
    sem_init(&data->sem_2,1,0);
    sem_init(&data->query_sem,1,1);
    sem_init(&data->client_connecting,1,1);
    data->max_users_connected=1;
    data->client_disconnect=1;
    data->count_of_query=0;
    u_int8_t server_running = 1;
    struct config c;
    c.data=data;
    c.server_running=&server_running;

    sem_init(&c.server_shutdown,0,0);
    sem_init(&c.sem_server_running,0,1);

    pthread_t key_read;
    pthread_create(&key_read,NULL,communication,(void *)&c);

    char command[11]={0};
    while (1){
        scanf("%10[^\n]",command);
        while (!getchar());
        if(strcmp(command,"quit")==0){
            sem_wait(&c.sem_server_running);
            server_running=0;
            sem_post(&c.sem_server_running);
            sem_post(&data->sem_1);
            break;
        }
        if(strcmp(command,"stat")==0){
            sem_wait(&data->query_sem);
            printf("liczba zapytan wynosi: %i\n",data->count_of_query);
            sem_post(&data->query_sem);
        }
    }
    sem_wait(&c.server_shutdown);

    munmap(data,sizeof(struct memory));
    close(fd);
    shm_unlink("shared_memory");
    return 0;
}
void *communication(void *val){

    struct config *data = val;
    while (1){

        sem_wait(&data->data->sem_1);
        sem_wait(&data->sem_server_running);
        sem_wait(&data->data->client_connecting);
        if(*(data->server_running)==0 && data->data->client_disconnect==1){
            sem_post(&data->sem_server_running);
            sem_post(&data->data->client_connecting);
            break;
        }
        sem_post(&data->data->client_connecting);
        sem_post(&data->sem_server_running);

        int fd_2= shm_open("data_shared",O_CREAT|O_EXCL|O_RDWR,0666);
        if(fd_2==-1){
            if(errno==EEXIST){
                printf("restart\n");
                shm_unlink("data_shared");
            }
            break;
        }
        ftruncate(fd_2, sizeof(int32_t)*data->data->size);
        int32_t *number_tab= mmap(0,sizeof(int32_t)*data->data->size,PROT_READ|PROT_WRITE,MAP_SHARED,fd_2,0);

        sem_post(&data->data->sem_2);
        sem_wait(&data->data->sem_1);

        data->data->first_index=-1;
        data->data->count_of_serching_val=0;
        for(int i=0;i<data->data->size;i++){
            if(*(number_tab+i)==data->data->serching_val){
                data->data->count_of_serching_val++;
                if(data->data->first_index==-1){
                    data->data->first_index=i;
                }
            }
        }

        sem_post(&data->data->sem_2);
        sem_wait(&data->data->sem_1);

        munmap(number_tab,sizeof(int32_t)*data->data->size);
        close(fd_2);
        shm_unlink("data_shared");

        sem_post(&data->data->sem_2);
    }
    sem_post(&data->server_shutdown);
    return NULL;
}
