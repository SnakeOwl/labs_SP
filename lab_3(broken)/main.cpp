/*
* это главная программа. Она должна вызывать новый процесс, который вызывает другие
*/

#define _POSIX_SOURCE
#include <iostream>
#include <string>
#include <csignal>
#include <string.h>
#include <sstream>
#include <boost/process.hpp>

#define N_CHILDREN_PROCESSES 1
#define NAME_CHILD_PROCESS "program_1"

namespace bp = boost::process;

using std::cout;
using std::endl;
using std::string;

static void create_children_processes(void);


int main(int n_args, char **args)
{
    create_children_processes();
    return 0;
}

static void
create_children_processes(void)
{
    long children_pid = 0;
    bp::ipstream pipe_stream;
    string buffer;
#if N_CHILDREN_PROCESSES > 0
    bp::child process(NAME_CHILD_PROCESS, bp::std_out > pipe_stream);
    while (pipe_stream && std::getline(pipe_stream, buffer) && !buffer.empty())
        cout << buffer << endl;
#endif
}