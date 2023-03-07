#include <iostream>
#include <getopt.h>
#include <filesystem>
#include "ThreadPool/ThreadPool.h"

namespace fs = std::filesystem;

// structure built for long versions of option arguments
static struct option long_options[] =
    {
        {"dir", required_argument, 0, 'd'},
        {"log_file", required_argument, 0, 'l'},
        {"result_file", required_argument, 0, 'r'},
        {"threads", required_argument, 0, 't'},
        {0, 0, 0, 0}};

// function which return given by user arguments
auto get_arguments(int argc, char **argv)
{
    // structure of values which will be return by function
    struct return_values
    {
        std::string str1, str2, str3;
        int i1;
        std::string str4;
    };

    // default values of optional arguments
    std::string dir_param = fs::current_path().generic_string();
    std::string log_file_param = "Specific_grep.log";
    std::string result_file_param = "Specific_grep.txt";
    int num_threads_param = 4;
    std::string pattern = "";

    // integer variable which help iterate through optional arguments in while loop
    int c;

    // while loop for find parameters of optional arguments
    while ((c = getopt_long(argc, argv, "d:l:r:t:", long_options, NULL)) != -1)
    {
        switch (c)
        {
        case 'd':
            dir_param = optarg;
            break;
        case 'l':
            log_file_param = optarg;
            break;
        case 'r':
            result_file_param = optarg;
            break;
        case 't':
            num_threads_param = atoi(optarg);
            break;
        case '?':
            std::cerr << "Unknown option: " << optopt << std::endl;
            break;
        }
    }

    // check if there is obligatory pattern word argument
    if (optind < argc)
    {
        pattern = argv[optind];
    }

    return return_values{dir_param, log_file_param, result_file_param, num_threads_param, pattern};
}

int main(int argc, char **argv)
{

    auto [dir_param, log_file_param, result_file_param, num_threads_param, pattern] = get_arguments(argc, argv);

    // check if there is argument with pattern word which is required
    if (pattern.size() == 0)
    {
        std::cerr << "Error: No pattern argument" << std::endl;
        std::cerr << "Program should be executed as below:" << std::endl;
        std::cerr << "./Specific_grep \"pattern word\" " << std::endl;
        return 1;
    }

    std::cout << "Directory: " << dir_param << std::endl;
    std::cout << "Log file name: " << log_file_param << std::endl;
    std::cout << "Name of result file: " << result_file_param << std::endl;
    std::cout << "Number of threads: " << num_threads_param << std::endl;
    std::cout << "Pattern word: " << pattern << std::endl;

    ThreadPool pool(4);
}
