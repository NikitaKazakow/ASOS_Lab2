#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

const char *processes [5] = {
    "./bin/addition",
    "./bin/division",
    "./bin/multiplication",
    "./bin/root",
    "./bin/subtraction"
};

pid_t pids [5];

//Коэффициенты квадратного уравнения
double a, b, c;

//Решение квадратного уравнения
double x1, x2;

//Флаг успешного запуска дочернего процесса
volatile bool process_ready_flag = false;

//Флаг успешного завершения расчетов в дочернем процессе
volatile bool process_calc_finish = false;

//Обработчик сигналов
void signal_handler(int signal_number, siginfo_t *siginfo, void *code) {
    if (signal_number == SIGUSR2) {
        process_calc_finish = true;
    }
    if (signal_number == SIGUSR1) {
        process_ready_flag = true;
    }
}

//Функци создания очереди
bool create_queue() {
    static struct mq_attr m;
    m.mq_maxmsg = 10;
    m.mq_msgsize = 16;
    m.mq_flags = 0;
    m.mq_curmsgs = 0;

    mq_unlink("/myQueue");

    mqd_t queueDescriptor = mq_open("/myQueue", O_RDWR|O_CREAT, S_IRWXU, &m);
    mq_close(queueDescriptor);

    if (queueDescriptor != -1) {
        return true;
    }
    return false;
}

//Функция создания процессов
void create_processes() {
    char *e[] = {"", NULL};
    pid_t pid;
    size_t i = 0;
    for (i = 0; i < 5; i++) {
        pids[i] = fork();
        if (!pids[i]) {
            execv(processes[i], e);
        }
        else
        {
            while (!process_ready_flag) { }
            process_ready_flag = false;   
        }
    }
}

//Функция чтения числа из очереди сообщений
double receive_double_from_queue() {
    
    int p;
    char buf[16];
    
     mqd_t queueDescriptor = mq_open("/myQueue", O_RDWR);

    mq_receive(queueDescriptor, buf, 16, &p);
    mq_close(queueDescriptor);
    return atof(buf);
}

//Функция записи числа в очередь сообщений
bool send_double_to_queue(double value) {
    
    char str[16] = {0};
    mqd_t queueDescriptor = mq_open("/myQueue", O_RDWR);

    if (snprintf(str, 16, "%lf", value) > 16)
    {
        mq_close(queueDescriptor);
        return false;
    }
    if(mq_send(queueDescriptor, str, 6, 30) != -1) {
        printf("Процесс сложения: %s %lf %s\n", "Сообщение:", value, "отправлено");
        mq_close(queueDescriptor);
        return true;
    }
    else {
        printf("Процесс сложения: %s\n", "Ошибка отправки сообщения!");
        mq_close(queueDescriptor);
        return false;
    }
}

//Функция завершения всех дочерних процессов
void stop_all_child_proces() {
    for (size_t i = 0; i < 5; i++)
    {
        kill(pids[i], SIGSTOP);
    }
}

//Функция добавления обработчика сигналов
void add_signals_handler() {
    sigset_t   set; 
    sigemptyset(&set);                                                             
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);

    struct sigaction act, old_act;
    
    act.sa_sigaction = signal_handler;
    act.sa_flags = SA_SIGINFO;
    act.sa_mask = set;
    
    sigaction(SIGUSR1, &act, &old_act);
    sigaction(SIGUSR2, &act, &old_act);
}

//Функция решения уравнения
void calculate() {
    double temp = 0;
    double discriminant_root = -1;

    //Считаем b^2
    send_double_to_queue(b);
    send_double_to_queue(b);
    
    kill(pids[2], SIGUSR1);
    while(!process_calc_finish) { }
    process_calc_finish = false;
    
    temp = receive_double_from_queue();

    //Считаем 4*A*C
    send_double_to_queue(a);
    send_double_to_queue(c);
    send_double_to_queue(4.0);
    
    kill(pids[2], SIGUSR1);
    while(!process_calc_finish) { }
    process_calc_finish = false;
    kill(pids[2], SIGUSR1);
    while(!process_calc_finish) { }
    process_calc_finish = false;

    //Считаем дискриминант
    send_double_to_queue(temp);

    kill(pids[4], SIGUSR1);
    while(!process_calc_finish) { }
    process_calc_finish = false;

    temp = receive_double_from_queue();
    if (temp < 0) {
        printf("Основной процесс: Значение дискриминанта меньше 0 (%lf). Уравнение не имеет действительных решений..\n", temp);
        stop_all_child_proces();
        mq_unlink("/myQueue");
        exit(0);
    }
    else {
        send_double_to_queue(temp);
    }
    

    //Считаем корень из дискриминанта
    kill(pids[3], SIGUSR1);
    while(!process_calc_finish) { }
    process_calc_finish = false;

    discriminant_root = receive_double_from_queue();

    //Считаем 2A
    send_double_to_queue(2.0);
    send_double_to_queue(a);

    kill(pids[2], SIGUSR1);
    while(!process_calc_finish) { }
    process_calc_finish = false;

    temp = receive_double_from_queue();

    //Считаем -b - √D
    send_double_to_queue(discriminant_root);
    send_double_to_queue(-b);

    kill(pids[4], SIGUSR1);
    while(!process_calc_finish) { }
    process_calc_finish = false;
    
    //Счтаем x1
    send_double_to_queue(temp);

    kill(pids[1], SIGUSR1);
    while(!process_calc_finish) { }
    process_calc_finish = false;

    x1 = receive_double_from_queue();

    //Считаем -b + √D
    send_double_to_queue(discriminant_root);
    send_double_to_queue(-b);

    kill(pids[0], SIGUSR1);
    while(!process_calc_finish) { }
    process_calc_finish = false;

    //Считаем x2
    send_double_to_queue(temp);

    kill(pids[1], SIGUSR1);
    while(!process_calc_finish) { }
    process_calc_finish = false;

    x2 = receive_double_from_queue();
}

int main(int argc, char *argv []) {

    add_signals_handler();

    printf("%s\n", "Основной процесс: Запущен!");
    printf("Основной процесс: %s\n%s", "Введите коэффициенты квадратного уравнения: Ax^2 + Bx + C", "A: ");
    scanf("%lf", &a);
    printf("%s", "B: ");
    scanf("%lf", &b);
    printf("%s", "C: ");
    scanf("%lf", &c);

    if(!create_queue()) {
        printf("Основной процесс: %s\n", "Ошибка создания очереди сообщений!");
        return -1;
    }
    else {
        printf("Основной процесс: %s\n", "Очередь сообщений успешно создана!");
    }
    
    printf("Основной процесс: %s\n", "Ожидание запуска дочерних процессов...");

    create_processes();

    calculate();

    printf("Основной процесс: Результат решения уравнения:\nX1 = %lf\nX2 = %lf\n", x1, x2);

    stop_all_child_proces();
    
    mq_unlink("/myQueue");   

    return 0;
}