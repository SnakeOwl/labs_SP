/*
* 1 и 2 аргументы являются минимумом и максимумом соответственно
* 3 аргумент является именем директории
*/

#include <iostream>
#include <string>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;


int main(int argc, char** args)
{
    if (argc < 4)
    {
        std::cerr << "Need 3 arguments!" << '\n';
        return 1;
    }

    long min = std::stol(args[1]);
    long max = std::stol(args[2]);
    std::string directory = args[3];
    long size = 0;
    const std::string path = "/" + directory;

    for (const auto & entry : fs::recursive_directory_iterator(path))
    {
        try
        {
            if (fs::is_directory(entry.path()) || fs::is_symlink(entry.path()))
                continue;

            size = file_size(entry.path());
            if (min < size && size < max)
                std::cout << entry.path() << " \t" << size << " bytes." << '\n';
        }
        catch(const fs::filesystem_error& e)
        {
        std::cerr << e.what() << '\n';
        }
    }
    return 0;
}
