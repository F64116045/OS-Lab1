#include <time.h>
#include "sender.h"
#define SEM_SEND "/sem_send"
#define SEM_RECV "/sem_recv"


void send(message_t message, mailbox_t* mailbox_ptr){
    if (mailbox_ptr->flag == 1) {
        if (msgsnd(mailbox_ptr->storage.msqid, &message, sizeof(message.content), 0) == -1) {
            perror("msgsnd failed");
            exit(EXIT_FAILURE);
        }
    } else if (mailbox_ptr->flag == 2) {
        strcpy(mailbox_ptr->storage.shm_addr, message.content);
    }

}

int main(int argc, char *argv[]){
    /*  TODO: 
        1) Call send(message, &mailbox) according to the flow in slide 4
        2) Measure the total sending time
        3) Get the mechanism and the input file from command line arguments
            • e.g. ./sender 1 input.txt
                    (1 for Message Passing, 2 for Shared Memory)
        4) Get the messages to be sent from the input file
        5) Print information on the console according to the output format
        6) If the message form the input file is EOF, send an exit message to the receiver.c
        7) Print the total sending time and terminate the sender.c
    */
    int method = atoi(argv[1]);
    mailbox_t mailbox;
    mailbox.flag = method;
    message_t message;

    if (method == 1) {
        printf("\033[1m\033[33mMessage Passing\033[0m\n");
        key_t key = ftok("receiver.c", 65);
        mailbox.storage.msqid = msgget(key, IPC_CREAT | 0666);
        if (key == -1) {
            perror("ftok failed");
            exit(1);
        }
    } else if (method == 2) {
        printf("\033[1m\033[33mShare Memory\033[0m\n");
        key_t key = ftok("receiver.c", 65);
        if (key == -1) {
            perror("ftok failed");
            exit(1);
        }
        int shmid = shmget(key, sizeof(message_t), IPC_CREAT | 0666);
        mailbox.storage.shm_addr = (char*) shmat(shmid, NULL, 0); 
    }

    FILE *input = fopen(argv[2], "r");
    if (input == NULL) {
        perror("Error opening file");
        return 1;
    }


    struct timespec start, end;
    sem_t *sem_send = sem_open(SEM_SEND, O_CREAT, 0666, 0); 
    if (sem_send == SEM_FAILED) {
        perror("sem_open failed for sem_send");
        exit(EXIT_FAILURE);
    }
    sem_t *sem_recv = sem_open(SEM_RECV, O_CREAT, 0666, 1);
    if (sem_recv == SEM_FAILED) {
        perror("sem_open failed for sem_recv");
        exit(EXIT_FAILURE);
    }

    double total_time = 0;
    while (fgets(message.content, sizeof(message.content), input)) { 

        sem_wait(sem_recv); 
        clock_gettime(CLOCK_MONOTONIC, &start);
        send(message, &mailbox);
        clock_gettime(CLOCK_MONOTONIC, &end);
        sem_post(sem_send);
       
        printf("\033[1m\033[33mSending message\033[0m: %s", message.content);
        double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
        total_time += elapsed_time;

    }


    sem_wait(sem_recv);
    strncpy(message.content, "q", sizeof(message.content) - 1);
    message.content[sizeof(message.content) - 1] = '\0'; 

    printf("\n\033[1m\033[31mSender Exit\033[0m\n");
    send(message, &mailbox);
    sem_post(sem_send);



    printf("\nTotal sending time: %f seconds\n", total_time);


    fclose(input);

    sem_close(sem_send);
    sem_close(sem_recv);
    sem_unlink(SEM_SEND);
    sem_unlink(SEM_RECV);


    if (method == 2) {
        shmdt(mailbox.storage.shm_addr);
    }


    return 0;
}