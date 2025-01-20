#include "receiver.h"

void receive(message_t* message_ptr, mailbox_t* mailbox_ptr){
    if (mailbox_ptr->flag == 1){
        mq_receive(mailbox_ptr->storage.msqid, message_ptr->message, 1024, NULL);  //in sender's attr.mq_msgsize = 1024;
    }
    else{
        strcpy(message_ptr->message, mailbox_ptr->storage.shm_addr);
    }
}

int main(int argc, char* argv[]){
    mailbox_t mailbox;
    message_t message;
    mailbox.flag = atoi(argv[1]);

    if (mailbox.flag == 1){
        struct mq_attr attr;        //define in <mqueue.h>
        attr.mq_flags = 0;         // 默認阻塞模式
        attr.mq_maxmsg = 10;       // 訊息佇列最多可以容納 10 條訊息
        attr.mq_msgsize = 1024;    // 每條訊息的最大大小為 1024 字節
        attr.mq_curmsgs = 0;       // 訊息佇列初始為空（只讀）
        
        mailbox.storage.msqid = mq_open("/msqid", O_CREAT | O_RDONLY, 0666, &attr);   //need to add /
        if (mailbox.storage.msqid == (mqd_t)-1) {
            perror("mq_open");
            exit(1);
        }
    }
    else{
        int shm_fd = shm_open("shm_fd", O_CREAT | O_RDWR, 0666);
        mailbox.storage.shm_addr = mmap(0, sizeof(message_t), PROT_READ, MAP_SHARED, shm_fd, 0);
    }

    sem_t *sender_sem = sem_open("sender_sem", O_CREAT, 0644, 1);         //可以發送
    sem_t *receiver_sem = sem_open("receiver_sem", O_CREAT, 0644, 0);     //先被鎖住
    struct timespec start, end;
    double total_time = 0;
    while(1){
        sem_wait(receiver_sem);  //鎖住自己，執行完後會減一

        clock_gettime(CLOCK_MONOTONIC, &start);
        receive(&message, &mailbox);
        clock_gettime(CLOCK_MONOTONIC, &end);
        total_time = total_time + (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;

        if (strcmp(message.message, "complete") == 0){
            printf("\nSender exist!");
            break;
        }
        printf("Receiving Message : %s", message.message);
        sem_post(sender_sem);  // 解鎖對面
    }

    printf("\nTotal time taken in receiving msg: %f seconds\n", total_time);

    if (mailbox.flag == 1) {
        mq_close(mailbox.storage.msqid);
        mq_unlink("/msqid");
    } else{
        munmap(mailbox.storage.shm_addr, sizeof(message_t));
        shm_unlink("shm_fd");
    }
    sem_close(sender_sem);
    sem_close(receiver_sem);
    sem_unlink("sender_sem");   //這邊是字串
    sem_unlink("receiver_sem");

    return 0;
}