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

int main(int size,char **tab) {
    if((size-1)%2!=0){
        printf("nieporawna liczba argumentow main!!!");
        return 2;
    }

    int fd= shm_open("shared_memory",O_RDWR,0666);
    if(fd==-1){
        printf("SM nie istnieje\n");
        return 1;
    }

    ftruncate(fd,sizeof(struct memory));
    struct memory *data=(struct memory*) mmap(0,sizeof(struct memory),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

    sem_wait(&data->client_connecting);
    if(data->users_connected==data->max_users_connected){
        printf("SERVER JEST PELEN!!");
        return 3;
    }
    data->users_connected++;
    data->client_disconnect=0;
    sem_post(&data->client_connecting);
    int32_t file_size=0;

    for(int i=1;i<size;i+=2){

        FILE *f= fopen(*(tab+i),"r");
        if(f==0){
            printf("TAKI PLIK NIE ISTNIEJE");
            continue;
        }
        file_size=0;
        int32_t tmp;
        while (!feof(f)){
            fscanf(f,"%u\n",&tmp);
            file_size++;
        }

        data->size=file_size;
        sem_post(&data->sem_1);
        sem_wait(&data->sem_2);

        int fd_2 = shm_open("data_shared",O_RDWR,0666);
        if(fd_2==-1){
            munmap(data,sizeof(struct memory));
            close(fd);
            return 1;
        }
        ftruncate(fd_2,sizeof(int32_t)*file_size);
        int32_t *number_tab= mmap(0,sizeof(int32_t)*file_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd_2,0);


        fseek(f,0,SEEK_SET);
        file_size=0;
        while (!feof(f)){
            fscanf(f,"%u\n",&tmp);
            *(number_tab+file_size)=tmp;
            file_size++;
        }

        data->serching_val=atoi(*(tab+i+1));

        sem_post(&data->sem_1);
        sem_wait(&data->sem_2);

        printf("szukana liczba: %i ,liczba wystapien: %i ,index wytapienia: %i\n",atoi(*(tab+i+1)),data->count_of_serching_val,data->first_index);

        sem_wait(&data->query_sem);
        data->count_of_query++;
        sem_post(&data->query_sem);

        sem_post(&data->sem_1);
        sem_wait(&data->sem_2);

        munmap(number_tab,sizeof(int32_t)*data->size);
        close(fd);
    }

    sem_wait(&data->client_connecting);
    data->client_disconnect=1;
    data->users_connected=0;
    sem_post(&data->client_connecting);

    munmap(data,sizeof(struct memory));
    close(fd);
    return 0;
}
