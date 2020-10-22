#include <stdio.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

volatile bool flag = false;

void signal_handler(int signal_number, siginfo_t *siginfo, void *code) {
    if (signal_number == SIGUSR1)
    {
        flag = true;
    }
    
}

void add_signal_handler() {
    sigset_t   set; 
    sigemptyset(&set);                                                             
    sigaddset(&set, SIGUSR1);

    struct sigaction act, old_act;
    
    act.sa_sigaction = signal_handler;
    act.sa_flags = SA_SIGINFO;
    act.sa_mask = set;
    
    sigaction(SIGUSR1, &act, &old_act);
}

double receive_double_from_queue() {
    
    int p;
    char buf[16];
    
     mqd_t queueDescriptor = mq_open("/myQueue", O_RDWR);

    mq_receive(queueDescriptor, buf, 16, &p);
    mq_close(queueDescriptor);
    return atof(buf);
}

bool send_double_to_queue(double value) {
    
    char str[16] = {0};
    mqd_t queueDescriptor = mq_open("/myQueue", O_RDWR);

    if (snprintf(str, 16, "%lf", value) > 16)
    {
        mq_close(queueDescriptor);
        return false;
    }
    if(mq_send(queueDescriptor, str, 6, 30) != -1) {
        mq_close(queueDescriptor);
        return true;
    }
    else {
        mq_close(queueDescriptor);
        return false;
    }
}

int main(int argc, char *argv []) {
    
    add_signal_handler();

    printf("Процесс умножения: %s\n", "Запущен!");

    double a = 0;
    double b = 0;
    double result = 0;

    kill((pid_t)getppid(), SIGUSR1);

    while(true) {
        while (!flag) {}
        
        flag = false;

        a = receive_double_from_queue();
        printf("Процесс умножения: значение %lf прочитано из очереди\n", a);

        b = receive_double_from_queue();
        printf("Процесс умножения: значение %lf прочитано из очереди\n", b);

        result = a * b;

        if (!send_double_to_queue(result))
        {
            printf("Процесс умножения: Ошибка отправки значения: %lf\n", result);
            return -1;
        }
        else
        {
            printf("Процесс умножения: Результат: %lf * %lf = %lf\n", a, b, result);
        }

        kill((pid_t)getppid(), SIGUSR2);     
    }
}