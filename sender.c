#include "sender.h"
#define SEM_SEND "/sem_send"
#define SEM_RECV "/sem_recv"


void send(message_t message, mailbox_t* mailbox_ptr){
    if (mailbox_ptr->flag == 1) {
        msgsnd(mailbox_ptr->storage.msqid, &message, sizeof(message_t), 0);
    } else if (mailbox_ptr->flag == 2) {
        memcpy(mailbox_ptr->storage.shm_addr, &message, sizeof(message_t));
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
        // 初始化 Message Passing 的佇列
        key_t key = ftok("somefile", 65);  // 生成唯一的 key
        mailbox.storage.msqid = msgget(key, IPC_CREAT | 0666);  // 創建訊息佇列
    } else if (method == 2) {
        // 初始化 Shared Memory
        key_t key = ftok("somefile", 65);  // 生成唯一的 key
        int shmid = shmget(key, sizeof(message_t), IPC_CREAT | 0666);  // 創建共享記憶體
        mailbox.storage.shm_addr = (char*) shmat(shmid, NULL, 0);  // 附加共享記憶體
    }

    FILE *input = fopen(argv[2], "r");
    if (input == NULL) {
        perror("Error opening file");
        return 1;
    }

     // 時間測量開始
    struct timespec start, end;
    sem_t *sem_send = sem_open(SEM_SEND, O_CREAT, 0666, 0); 
    sem_t *sem_recv = sem_open(SEM_RECV, O_CREAT, 0666, 1); 
    // 逐行讀取文件並發送消息
    while (fgets(message.content, sizeof(message.content), input)) {
        message.size = strlen(message.content);
        sem_wait(sem_recv);  // 等待接收方準備好
        clock_gettime(CLOCK_MONOTONIC, &start);  // 開始計時
        send(message, &mailbox);  // 發送消息
        clock_gettime(CLOCK_MONOTONIC, &end);
        printf("\033[1m\033[33mSending message\033[0m: %s", message.content);
        sem_post(sem_send);  // 通知接收方消息已經發送
    }

    // 發送結束訊息（例如 "exit" 來告知接收方結束通信）
    strcpy(message.content, "exit\n");
    message.size = strlen(message.content);
    send(message, &mailbox);

    double elapsed_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1e9;

    // 打印發送時間
    printf("\nTotal sending time: %f seconds\n", elapsed_time);

    // 關閉文件
    fclose(input);

    sem_close(sem_send);
    sem_close(sem_recv);
    sem_unlink(SEM_SEND);
    sem_unlink(SEM_RECV);

    // 分離共享記憶體
    if (method == 2) {
        shmdt(mailbox.storage.shm_addr);  // 分離共享記憶體
    }
    if(method == 1){
        msgctl(mailbox.storage.msqid, IPC_RMID, NULL);
    }

    return 0;
}