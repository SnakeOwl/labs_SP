#define _XOPEN_SOURCE
#include <iostream>
#include <string>
#include <cstring>
#include <csignal>
#include <stdio.h>
#include <sstream>
#include <unistd.h>
#include <chrono>
#include <thread>
/***** semaphore *****/
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>

#define KEY 0x1111

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

/***** global variables *****/
static char block_exit = 0; /* блокировка процесса 0, чтобы не выводил канал раньше времени */
static char iterator = 0;   /* указывает на текущий процесс (всего их 3) */

static void signals_handler(int);

int main(int argc, char **args)
{
    static const char max_running_threads_number = 2;
    static int pids[max_running_threads_number + 1]; /* for store pids of processes */
    const char poem_lines = 8;
    const std::string poem[poem_lines] =
        {
            "Горные вершины",
            "Спят во тьме ночной;",
            "Тихие долины",
            "Полны свежей мглой;",
            "Не пылит дорога,",
            "Не дрожат листы...",
            "Подожди немного,",
            "Отдохнёшь и ты."};
    bool can_exit = 0;
    int p1[2]; /* pipe */
    pid_t pid;

    try
    {
        /***** initialisation *****/
        pids[0] = getpid();
        
        if (pipe(p1) != 0)
            throw "pipe() failed!";

        signal(SIGUSR1, signals_handler);

        /****** semaphore ******/
        struct sembuf p = {0, -1, SEM_UNDO};
        struct sembuf v = {0, +1, SEM_UNDO};
        int id = semget(KEY, 1, 0666 | IPC_CREAT);

        if (id < 0)
            throw "semget() failed!";
        union semun u;
        u.val = 1;

        if (semctl(id, 0, SETVAL, u) < 0)
            throw "semctl() failed!";

        /***** poem out *****/
        std::cout << "Стихотворение:" << '\n';
        for (int i = 0; i < poem_lines; i++)
            std::cout << poem[i] << '\n';
        std::cout << '\n';

        /***** create processes *****/
        for (int i = 0; i < max_running_threads_number; i++)
        {
            pid = fork();
            if (pid == 0) // если тут 0, значит это дочерний процесс!
            {
                iterator = i + 1;
                break;
            }
            else
            {
                pids[i + 1] = pid;
                block_exit++;
            }
        }
        printf("pid of %d is %d, parent %d\n", iterator, getpid(), getppid());

        /***** processing *****/
        const long buf_size = 40; // размер буффера для чтения/записи
        if (iterator == 0)
        {
            while (!block_exit)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            /***** read pipe *****/
            close(p1[1]);
            char buf[buf_size];
            for (int i = 0; i < poem_lines; i++)
            {
                int cs = 0;
                if ((cs = read(p1[0], buf, buf_size)) > -1)
                {
                    std::cout << buf << '\n';
                }
                else if (cs < 0)
                    throw "read() error";
            }
        }
        else
        {
            /***** write pipe *****/
            close(p1[0]);
            std::string line;
            for (int i = 0; i < poem_lines - 1; i += 2)
            {
                if (iterator == 1)
                    line = poem[i];
                else if (iterator == 2)
                    line = poem[i + 1];

                if (semop(id, &p, 1) < 0) //закрыть семафор
                    throw "semop() failed!";

                if (write(p1[1], line.c_str(), buf_size) == -1)
                    throw "write() error";

                if (semop(id, &v, 1) < 0) //открыть семафор
                    throw "semop() failed!";
            }
            kill(pids[0], SIGUSR1);
        }
    }
    catch (const char *msg)
    {
        std::cout << msg << '\n';
    }
    catch (...)
    {
        std::cout << " Что-то пошло не так." << '\n';
    }
    return 0;
}

static void
signals_handler(int signum)
{
    if (signum == SIGUSR1)
    {
        printf("%d процесс, pid: %d, ppid: %d, получил сигнал SIGUSR1\n", iterator, getpid(),
               getppid());
        if (iterator == 0)
            block_exit--;
    }
}