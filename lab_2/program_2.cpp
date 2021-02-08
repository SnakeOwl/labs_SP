/*
*   Эта программа вызывается из первой, именно она производит копирование файлов.
*/

#include <iostream>
#include <string>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

int main(int n_args, char **args)
{
    const std::string path = args[1];
    const std::string path_to = args[2];

    try
    {
        fs::copy(path, path_to, fs::copy_options::skip_existing);
        std::cout << "File copied. Success!" << '\n';
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << e.what() << '\n';
    }
    return 0;
}