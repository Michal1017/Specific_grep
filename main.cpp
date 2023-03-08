#include <iostream>
#include <getopt.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <algorithm>
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
    std::string log_file_param = "Specific_grep";
    std::string result_file_param = "Specific_grep";
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

// function which help to compare sizes of vectors of strings
bool compare_vectors(std::vector<std::string> &v1, std::vector<std::string> &v2)
{
    return v1.size() > v2.size();
}

// function which write results in result file
void write_to_result_file(std::vector<std::vector<std::string>> &results, std::string &result_file_name)
{
    // add .txt to result file name
    result_file_name = result_file_name + ".txt";
    // open file
    std::ofstream result_file(result_file_name, std::ios::out);
    // check if file is open
    if (!result_file)
    {
        // information if file is not open
        std::cerr << "Failed to open file!" << std::endl;
    }
    else
    {
        // write data to file
        for (const auto &result : results)
        {
            for (const auto &line : result)
            {
                result_file << line << std::endl;
            }
        }
        result_file.close();
    }
}

// function which find pattern words in files
void find_word(const fs::path &dir, const std::string &word, ThreadPool &pool, std::vector<std::future<void>> &futures, std::string &result_file_name)
{
    std::vector<std::vector<std::string>> results;
    std::shared_ptr<std::vector<std::string>> results_ptr = std::make_shared<std::vector<std::string>>();
    // search for files in folders
    for (const auto &entry : fs::recursive_directory_iterator(dir))
    {
        // check if file is ok
        if (fs::is_regular_file(entry) && fs::file_size(entry) > 0)
        {
            std::vector<std::string> result;
            // create task for threat pool which looking for pattern word in file
            auto future = pool.submit([&entry, &word, &result, results_ptr]()
                                      { 
                                        // open file to read
                                        std::ifstream file(entry.path());
                                        if (file.good())
                                        {
                                            int line_number = 0;
                                            std::string line;
                                            // read file line by line
                                            while (std::getline(file, line))
                                            {
                                                // increase line number
                                                line_number++;  
                                                // check if line in file has pattern word
                                                if (line.find(word) != std::string::npos)
                                                {
                                                    // create line to write into result file
                                                    std::string result_line = entry.path().generic_string() + ':' + std::to_string(line_number) + ": " + line;
                                                    // add line to vectors of results
                                                    result.push_back(result_line);
                                                    results_ptr->push_back(result_line);
                                                }
                                            }
                                        } });
            futures.push_back(std::move(future));
            // wait until all threads finish their work
            for (auto &future : futures)
            {
                future.wait();
            }
            // add vector of lines found inside one file to vector of all found matching lines
            results.emplace_back(result);
        }
    }

    // sort results vector by size of lines found in each file
    std::sort(results.begin(), results.end(), compare_vectors);

    write_to_result_file(results, result_file_name);

    // for (auto &r1 : results)
    // {
    //     for (auto &r2 : r1)
    //     {
    //         std::cout << r2 << std::endl;
    //     }
    // }
}

int main(int argc, char **argv)
{
    // get arguments
    auto [dir_param_str, log_file_param, result_file_param, num_threads_param, pattern] = get_arguments(argc, argv);

    // convert
    fs::path dir_param;
    if (std::filesystem::exists(dir_param_str))
    {
        dir_param = dir_param_str;
    }
    else
    {
        std::cerr << "Wrong filepath!" << std::endl;
        std::cerr << "Example filepath: C:\\Users\\Public" << std::endl;
        return 1;
    }

    std::cout << dir_param_str << std::endl;

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

    ThreadPool pool(num_threads_param);
    std::vector<std::future<void>> futures;
    find_word(dir_param, pattern, pool, futures, result_file_param);
}
