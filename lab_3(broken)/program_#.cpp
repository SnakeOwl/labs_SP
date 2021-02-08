#define _POSIX_SOURCE
#include <unistd.h>
#include <iostream>
#include <string>
#include <csignal>
#include <string.h>
#include <sstream>
#include <chrono>
#include <boost/process.hpp>
#include <thread>

//У меня по факту 1 + 8 программ, тут буду писать какая программа по счету
#define N_CHILD 1                        //номер текущей программы
#define N_CHILDREN_PROCESSES 1           //количество дочерних процессов
#define NAME_CHILD_PROCESS "program_2"   //программа, получатель сообщения
#define NAME_CHILD_PROCESS_2 "program_4" //если требуется обращение ко второму процессу
#define MAX_SIGNALS 101

namespace bp = boost::process;

using std::cout;
using std::endl;
using std::string;

long children_pids[N_CHILDREN_PROCESSES];
short X, Y;                 //counter signals SIGUSR1, SIGUSR2
bp::ipstream pipe_stream;   //поток для вывода данных из процесса
bp::ipstream pipe_stream_2; //поток для вывода данных из процесса 2

static void init(void);
static void create_children_processes(void);
static void handler(int sig);
static void send_signals_to_children();
int main(int n_args, char **args)
{
    init();

    struct sigaction act;
    memset(&act, 0, sizeof(act)); //задает 0 с адреса act до sizeof(act)
    act.sa_handler = handler;
    //задает обработчик сигналов
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    //задает сигнал в маску для перехвата
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGTERM);
    act.sa_mask = set;
    sigaction(SIGUSR1, &act, 0); //вроде задает какой сигнал, кому обрабатывать
    sigaction(SIGUSR2, &act, 0);
    sigaction(SIGTERM, &act, 0);
#if N_CHILDREN_PROCESSES > 0
    create_children_processes();
#endif
    while (1)
        ;
    return 0;
}
static void
create_children_processes(void)
{
#if N_CHILDREN_PROCESSES > 0
    bp::ipstream temp;
#if N_CHILD != 8
    bp::child process(NAME_CHILD_PROCESS, bp::std_out > pipe_stream);
#if N_CHILDREN_PROCESSES == 2
    bp::child process_3(NAME_CHILD_PROCESS_2, bp::std_out > pipe_stream_2);
#endif
#endif
    bp::child process_2("pgrep " + string(NAME_CHILD_PROCESS), bp::std_out > temp);
    string buffer;
    std::getline(temp, buffer);
    children_pids[0] = std::stol(buffer);
#if N_CHILD != 8
#if N_CHILDREN_PROCESSES == 2
    bp::ipstream temp_2;
    bp::child process_4("pgrep " + string(NAME_CHILD_PROCESS_2), bp::std_out >
                                                                     temp_2);
    std::getline(temp_2, buffer);
    children_pids[1] = std::stol(buffer);
#endif
#endif
    send_signals_to_children();
#endif
}
static void
send_signals_to_children(void)
{
#if N_CHILDREN_PROCESSES > 0
    int signal;
    string signal_to = "-";
    if (X == MAX_SIGNALS || Y == MAX_SIGNALS)
    {
        signal_to = "SIGTERM";
        signal = SIGTERM;
    }
    else if (N_CHILD == 2 || N_CHILD == 7 || N_CHILD == 8)
    {
        signal_to = "SIGUSR2";
        signal = SIGUSR2;
    }
    else
    {
        signal_to = "SIGUSR1";
        signal = SIGUSR1;
    }
    for (;;)
    {
        auto now = std::chrono::high_resolution_clock::now();
        cout << N_CHILD << " pid: " << getpid()
             << " послал: " << signal_to
             << " текущее время (мксек): "
             << std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count()
             << std::endl;
        kill(children_pids[0], signal);
#if N_CHILDREN_PROCESSES == 2
        kill(children_pids[1], signal);
#endif
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
#endif
}
static void
handler(int sig)
{
    string signal = "-";
    if (sig == SIGUSR1)
    {
        signal = "SIGUSR1";
        X++;
    }
    else if (sig == SIGUSR2)
    {
        signal = "SIGUSR2";
        Y++;
    }
    else if (sig == SIGTERM)
    {
#if N_CHILDREN_PROCESSES > 0
        for (char i = 0; i < N_CHILDREN_PROCESSES; i++)
            kill(children_pids[i], SIGTERM);
        string buffer;
#if N_CHILD != 8
        while (pipe_stream && std::getline(pipe_stream, buffer) && !buffer.empty())
            cout << buffer << endl;
#if N_CHILDREN_PROCESSES == 2
        buffer.clear();
        while (pipe_stream_2 && std::getline(pipe_stream_2, buffer) && !buffer.empty())
            cout << buffer << endl;
#endif
#endif
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        cout << getpid() << " ppid завершил работу после "
             << X << " сигнала SIGUSR1 "
             << " и " << Y << " сигнала SIGUSR2"
             << endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        exit(SIGTERM);
    }
    auto now = std::chrono::high_resolution_clock::now();
    cout << N_CHILD << " pid: " << getpid()
         << " получил: " << signal
         << " текущее время (мксек): "
         << std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count()
         << std::endl;
    if (X == MAX_SIGNALS || Y == MAX_SIGNALS)
        for (char i = 0; i < N_CHILDREN_PROCESSES; i++)
            kill(children_pids[i], SIGTERM);
}
static void
init(void)
{
    for (short i = 0; i < N_CHILDREN_PROCESSES; i++)
        children_pids[i] = 0;
    X = 0;
    Y = 0;
}