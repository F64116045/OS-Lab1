#include "receiver.h"
#include <time.h>  // 用於時間測量
#define SEM_SEND "/sem_send"
#define SEM_RECV "/sem_recv"

void receive(message_t* message_ptr, mailbox_t* mailbox_ptr) {
    if (mailbox_ptr->flag == 1) {
        // Message Passing 機制
        msgrcv(mailbox_ptr->storage.msqid, message_ptr, sizeof(message_t), 0, 0);
    } else if (mailbox_ptr->flag == 2) {
        // Shared Memory 機制
        memcpy(message_ptr, mailbox_ptr->storage.shm_addr, sizeof(message_t));
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <method>\n", argv[0]);
        return 1;
    }

    int method = atoi(argv[1]);  // 1: Message Passing, 2: Shared Memory
    mailbox_t mailbox;
    mailbox.flag = method;
    message_t message;

    // 初始化通信機制
    if (method == 1) {
        // 初始化 Message Passing 的佇列
        key_t key = ftok("somefile", 65);  // 生成唯一的 key
        mailbox.storage.msqid = msgget(key, 0666);  // 獲取訊息佇列
    } else if (method == 2) {
        // 初始化 Shared Memory
        key_t key = ftok("somefile", 65);  // 生成唯一的 key
        int shmid = shmget(key, sizeof(message_t), 0666);  // 獲取共享記憶體
        mailbox.storage.shm_addr = (char*) shmat(shmid, NULL, 0);  // 附加共享記憶體
    }

    // 時間測量開始
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);  // 開始計時

    sem_t *sem_send = sem_open(SEM_SEND, O_CREAT, 0666, 0); 
    sem_t *sem_recv = sem_open(SEM_RECV, O_CREAT, 0666, 1); 

    // 不斷接收消息，直到接收到退出訊息
    while (1) {
        sem_wait(sem_send);  // 等待發送方發送消息
        receive(&message, &mailbox);  // 接收消息
        printf("\033[1m\033[33mReceive message\033[0m: %s", message.content);
        sem_post(sem_recv);  // 通知發送方可以發送下一條消息

        if (strcmp(message.content, "exit\n") == 0) break;  // 檢查退出訊息

        printf("CHECK\n");
    }

    sem_close(sem_send);
    sem_close(sem_recv);
    sem_unlink(SEM_SEND);
    sem_unlink(SEM_RECV);
    
    // 時間測量結束
    clock_gettime(CLOCK_MONOTONIC, &end);  // 結束計時
    double elapsed_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1e9;

    // 打印接收時間
    printf("\nTotal receiving time: %f seconds\n", elapsed_time);

    // 分離共享記憶體並刪除
    if (method == 2) {
        shmdt(mailbox.storage.shm_addr);  // 分離共享記憶體
        shmctl(shmget(ftok("somefile", 65), sizeof(message_t), 0666), IPC_RMID, NULL);  // 刪除共享記憶體
    }

    if(method == 1){
        msgctl(mailbox.storage.msqid, IPC_RMID, NULL);
    }

    return 0;
}
