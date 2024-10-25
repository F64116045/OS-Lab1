#include "receiver.h"
#include <time.h>  
#define SEM_SEND "/sem_send"
#define SEM_RECV "/sem_recv"

void receive(message_t* message_ptr, mailbox_t* mailbox_ptr) {
    if (mailbox_ptr->flag == 1) {
        int k = msgrcv(mailbox_ptr->storage.msqid, message_ptr, sizeof(message_ptr->content), 0, 0);
        printf("size : %ld\n",strlen(message_ptr->content));
        if (k == -1) {
            perror("msgrcv failed");
            exit(1);
        }
    } else if (mailbox_ptr->flag == 2) {
        strcpy(message_ptr->content, mailbox_ptr->storage.shm_addr);
    }

    message_ptr->content[sizeof(message_ptr->content) - 1] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <method>\n", argv[0]);
        return 1;
    }

    int method = atoi(argv[1]);
    mailbox_t mailbox;
    mailbox.flag = method;
    message_t message;

    if (method == 1) {
        printf("\033[1m\033[33mMessage Passing\033[0m\n");
        key_t key = ftok("receiver.c", 65);
        if (key == -1) {
            perror("ftok failed");
            exit(1);
        }
        mailbox.storage.msqid = msgget(key, 0666);
    } else if (method == 2) {
        printf("\033[1m\033[33mShare Memory\033[0m\n");
        key_t key = ftok("receiver.c", 65);
        if (key == -1) {
            perror("ftok failed");
            exit(1);
        }

        int shmid = shmget(key, sizeof(message_t), 0666);
        mailbox.storage.shm_addr = (char*) shmat(shmid, NULL, 0);
    }


    struct timespec start, end;

    sem_t *sem_send = sem_open(SEM_SEND, O_CREAT, 0666, 0);
    sem_t *sem_recv = sem_open(SEM_RECV, O_CREAT, 0666, 1);

    sem_post(sem_send);
    double total_time = 0;
    while (1) {      
        sem_wait(sem_send); 
        clock_gettime(CLOCK_MONOTONIC, &start);
        receive(&message, &mailbox); 
        clock_gettime(CLOCK_MONOTONIC, &end);
        sem_post(sem_recv);

        if (strcmp(message.content, "q") == 0) {
            printf("\n");
            printf("\033[1m\033[31mEnd of input file! exit!\033[0m:\n");
            break;
        }
        printf("\033[1m\033[33mReceive message\033[0m: %s", message.content); 
        double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
        total_time += elapsed_time;
    }


    sem_close(sem_send);
    sem_close(sem_recv);
    sem_unlink(SEM_SEND);
    sem_unlink(SEM_RECV);
    

    printf("\nTotal receiving time: %f seconds\n", total_time);

    if (method == 2) {
        shmdt(mailbox.storage.shm_addr);  
        shmctl(shmget(ftok("receiver.c", 65), sizeof(message_t), 0666), IPC_RMID, NULL); 
    }


    return 0;
}
