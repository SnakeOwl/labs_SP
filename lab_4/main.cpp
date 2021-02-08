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
/***** semaphore: *****/
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
static bool block_writers = 1;
static int iterator = 0; //указывает на текущий процесс (всего их 3)
static void signals_handler(int);

int main(int n_args, char **args)
{
    const int max_running_threads_number = 2;
    struct sembuf p = {0, -1, SEM_UNDO};
    struct sembuf v = {0, +1, SEM_UNDO};
    bool can_exit = 0;
    int pids[max_running_threads_number + 1]; //хранение ид созданных процессов
    int p1[2];
    pid_t pid;

    try
    {
        /***** initialisation *****/
        pids[0] = getpid(); // father
        signal(SIGUSR1, signals_handler);
        signal(SIGUSR2, signals_handler);

        /***** semaphore *****/
        int id = semget(KEY, 1, 0666 | IPC_CREAT); //присвятой семафор

        if (id < 0)
            throw "semget() failed!";

        union semun u;
        u.val = 1;

        if (semctl(id, 0, SETVAL, u) < 0)
            throw "semctl() failed!";

        /***** create processes *****/
        for (int i = 0; i < max_running_threads_number; i++)
        {
            if (pipe(p1) != 0)
                throw "pipe() failed!";
            pid = fork();
            if (pid == 0) // если тут 0, значит это дочерний объект!
            {
                iterator = i + 1;
                break;
            }
            else if (pid > 0) // если нет, то это id потомка для родителя!
                pids[i + 1] = pid;
        }
        printf("pid of %d is %d, parent %d\n", iterator, getpid(), getppid());

        /***** send signals *****/
        const long buf_size = 55; // размер буффера для чтения/записи
        const long max_lines = 1010;
        if (iterator == 0)
        {
            for (int i = 1; i <= max_running_threads_number; i++)
                kill(pids[i], SIGUSR2);

            /***** read pipe *****/
            const int step = 75;
            char buf[buf_size];
            short block_break = max_lines * 2;
            int counter = 0;
            close(p1[1]);
            while (block_break > 0)
            {
                for (int i = 0; i < step; i++)
                {
                    if (block_break == 0)
                        break;

                    if (read(p1[0], buf, buf_size) == -1)
                        throw "read() error in child";

                    std::cout << buf << " осталось итераций: " << block_break << '\n';
                    block_break--;
                }
                std::cout << "--- количество шагов по 75: " << ++counter << '\n';
            }
        }
        else
        {
            while (block_writers)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            /***** write pipe *****/
            close(p1[0]);
            const long step = 110;
            for (int j = 0; j < max_lines;)
            {
                if (semop(id, &p, 1) < 0) //закрыть семафор
                    throw "semop() failed!";

                for (int i = 0; i < step; i++, j++)
                {
                    if (j == max_lines)
                        break;
                        
                    std::string line = "line: " + std::to_string(__LINE__) + " pid: " + std::to_string(getpid());
                    line += " time(мксек): ";
                    auto now = std::chrono::high_resolution_clock::now();
                    line += std::to_string((unsigned long long)
                                               std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch())
                                                   .count());

                    // писать в канал нужно в p1[1], а получать из p1[0]
                    if (write(p1[1], line.c_str(), buf_size) == -1)
                        throw "write() error in child";
                }

                if (semop(id, &v, 1) < 0) //открыть семафор
                    throw "semop() failed!";
            }
            kill(pids[0], SIGUSR1);
        }
        std::cout << "Done!" << '\n';
    }
    catch (const char *msg)
    {
        std::cout << msg << '\n';
    }
    catch (...)
    {
        std::cout << "Что-то пошло не так." << '\n';
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
    }
    if (signum == SIGUSR2)
    {
        printf("%d процесс, pid: %d, ppid: %d, получил сигнал SIGUSR2\n", iterator, getpid(),
               getppid());
        block_writers = 0;
    }
}