/*
* Специфика задания (не знал про функцию fork).
* Будет 2 программы:
* Это первая программа.
* в первой будет происходить копирование данных, она будет брать пути переноса из аргументов
* во второй будет происходить считывание файлов и вызов первой программы.
*
* Я сначала сделал все на потоках, поэтому теперь переделаю вызов первой программы из
* каждого потока
*/

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <utility>
#include <list>
#include <functional>
#include <experimental/filesystem>
#include <boost/process.hpp>

namespace bp = boost::process;
namespace fs = std::experimental::filesystem;

/***** global variables *****/
const std::string second_program_name = "program_2";
std::mutex mtx;
std::list<std::pair<std::string, long>> list_files;

/***** glogal funcitons *****/
static void scan_files_and_prepare(const std::string path, const std::string path_to);
static void show_list(void);
static void sort_by(const long n_sort); // 1 - file name, 2 - file size
static void thread_handler(const std::string path, const std::string path_to);

int main(int argc, char **args)
{
    if (argc < 4)
    {
        std::cerr << "Need 3 arguments!" << '\n';
        return 1;
    }

    /***** initialisation *****/
    const char amount_threads = 2;
    const std::string path = args[1];          // 1 аргумент - сканируемая директория
    const std::string path_to = args[2];       // 2 аргумент является именем создаваемой директории
    const long sort_kind = std::stol(args[3]); // 3 аргумент - вид сортировки (1 - имя файла, 2 - размер файла)

    /***** main *****/
    scan_files_and_prepare(path, path_to);
    sort_by(sort_kind);

    std::cout << "Начинаю копирование файлов" << '\n';
    std::list<std::thread> threads;
    
    for (long i = 0; i < amount_threads; i++)
        threads.push_back(std::thread(thread_handler, path, path_to));

    for (auto& thread : threads) 
        thread.join();

    std::cout << "Копирование файлов. завершено" << '\n';

    return 0;
}

static void
scan_files_and_prepare(const std::string path, const std::string path_to)
{
    long size = 0;

    std::cout << "Копирование папок." << '\n';
    try
    {
        fs::create_directory(path_to);
        fs::copy(path, path_to, fs::copy_options::directories_only | fs::copy_options::recursive | fs::copy_options::skip_existing);
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << e.what() << std::endl;
    }
    std::cout << "Копирование папок. завершено." << '\n';

    std::cout << "Считывание файлов." << '\n';
    for (const auto &entry : fs::recursive_directory_iterator(path))
    {
        try
        {
            if (fs::is_directory(entry.path()) || fs::is_symlink(entry.path()))
                continue;

            size = fs::file_size(entry.path());
            list_files.push_back(std::pair<std::string, long>(entry.path(), size));
        }
        catch (const fs::filesystem_error &e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    std::cout << "Считывание файлов. завершено." << '\n';
}

static void
show_list(void)
{
    for (const auto p : list_files)
        std::cout << p.first << "\t:\t" << p.second << '\n';
}

static void
sort_by(const long n_sort = 1) // 1 - file name, 2 - file size
{
    auto p_start = list_files.begin();
    auto p_end = list_files.end();

    std::cout << "Сортировка списка файлов." << std::endl;
    while (p_start != p_end)
    {
        auto p_s = p_start;
        auto p_ss = p_start;
        p_ss++;
        while (p_ss != p_end)
        {
            if (n_sort == 1)
            {
                if (p_s->second > p_ss->second)
                {
                    std::pair<std::string, long> t = std::make_pair(p_s->first, p_s->second);
                    p_s->first = p_ss->first;
                    p_s->second = p_ss->second;
                    p_ss->first = t.first;
                    p_ss->second = t.second;
                }
            }
            else
            {
                if (p_s->first > p_ss->first)
                {
                    std::pair<std::string, long> t = std::make_pair(p_s->first, p_s->second);
                    p_s->first = p_ss->first;
                    p_s->second = p_ss->second;
                    p_ss->first = t.first;
                    p_ss->second = t.second;
                }
            }
            p_ss++;
        }
        p_start++;
    }
    std::cout << "Сортировка списка файлов. завершена." << std::endl;
}

static void
thread_handler(const std::string path, const std::string path_to)
{
    if (list_files.empty())
        return;

    const long cut = path.size();
    
    while (!(list_files.empty()))
    {
        mtx.lock();
        auto p_start = list_files.begin();
        
        std::string old_path = p_start->first;
        std::string new_path = p_start->first;
        new_path.erase(0, cut);
        new_path = path_to + new_path;
        long size = p_start->second;

        std::cout << "id thread: " << std::this_thread::get_id() << '\n'
                << "полный путь и имя файла: " << old_path << '\n'
                << "новый путь и имя файла: " << new_path << '\n';

        list_files.pop_front();
        mtx.unlock();
    
        bp::ipstream pipe_stream;
        std::string args = old_path + " " + new_path;
        bp::child process(second_program_name + " " + args, bp::std_out > pipe_stream);
        
        std::string line;
        while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
        {
            mtx.lock();
            std::cout << line << '\n';
            mtx.unlock();
        }
        process.wait();
    }
}