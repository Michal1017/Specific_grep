#include <iostream>
#include <getopt.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <algorithm>
#include <chrono>
#include <map>
#include <atomic>
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

// data structure where information for terminal output will be stored
struct Terminal_output_info
{
    int num_searched_files = 0;
    int num_files_with_pattern = 0;
    int num_found_patterns = 0;
    std::string result_path = "";
    std::string log_path = "";
    int num_threads = 0;
    double elapsed_time = 0;
};

// function which return given by user arguments,
// this function has declaration together with definition because return type is auto
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

    return return_values{dir_param, log_file_param, result_file_param,
                         num_threads_param, pattern};
}

// function which write results in result file
void write_to_result_file(std::shared_ptr<std::vector<std::vector<std::string>>> &results,
                          std::string &result_file_name);

// function which write logs to log file
void write_to_log_file(std::shared_ptr<std::map<std::thread::id,
                                                std::vector<std::string>>> &LogMapPtr,
                       std::string &log_file_name);

// function which is looking for pattern word inside file, it is task for threads
void find_pattern_word(const std::filesystem::directory_entry &entry, const std::string &word,
                       std::shared_ptr<std::vector<std::vector<std::string>>> &results_ptr,
                       std::shared_ptr<Terminal_output_info> &info_ptr,
                       std::shared_ptr<std::map<
                           std::thread::id, std::vector<std::string>>>
                           &LogMapPtr,
                       std::mutex &mutex_);

// function which is looking for files which can search
void find_files(const fs::path &dir, const std::string &word, ThreadPool &pool,
                std::vector<std::future<void>> &futures, std::string &result_file_name,
                std::shared_ptr<Terminal_output_info> &info_ptr, std::string &log_file_name);

// function which print information on console
void print_terminal_info(std::shared_ptr<Terminal_output_info> &info_ptr);

// function which is looking for any error in given parameters
bool find_errors(std::string &dir_param_str, std::filesystem::path &dir_param,
                 int &num_threads_param, std::string &pattern);

int main(int argc, char **argv)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    // get arguments
    auto [dir_param_str, log_file_param, result_file_param, num_threads_param, pattern] =
        get_arguments(argc, argv);

    // convert string dictonary paremeter to fs::path type
    fs::path dir_param;

    if (find_errors(dir_param_str, dir_param, num_threads_param, pattern))
    {
        return 1;
    }

    // create shared pointer to Terminal_output_info object
    std::shared_ptr<Terminal_output_info> info_ptr = std::make_shared<Terminal_output_info>();

    // create thread pool
    ThreadPool pool(num_threads_param);
    std::vector<std::future<void>> futures;

    // looking for a pattern word
    find_files(dir_param, pattern, pool, futures, result_file_param, info_ptr, log_file_param);

    // give information to info_ptr
    info_ptr->result_path = fs::current_path().generic_string() + "\\" +
                            result_file_param + ".txt";
    info_ptr->log_path = fs::current_path().generic_string() + "\\" + log_file_param + ".log";
    info_ptr->num_threads = num_threads_param;

    // calculate time elapsed from beginning to the end of the program
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    info_ptr->elapsed_time =
        std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();

    // print information on console
    print_terminal_info(info_ptr);

    return 0;
}

void write_to_result_file(std::shared_ptr<std::vector<std::vector<std::string>>> &results,
                          std::string &result_file_name)
{
    // sort results vector by size of lines found in each file
    std::sort(results->begin(), results->end(),
              [](std::vector<std::string> &v1, std::vector<std::string> &v2)
              { return v1.size() > v2.size(); });

    // add .txt to result file name
    std::string result_file_name_ = result_file_name + ".txt";
    // open file
    std::ofstream result_file(result_file_name_, std::ios::out);
    // check if file is open
    if (!result_file)
    {
        // information if file is not open
        std::cerr << "Failed to open result file!" << std::endl;
    }
    else
    {
        // write data to file
        for (const auto &result : *results)
        {
            for (const auto &line : result)
            {
                result_file << line << std::endl;
            }
        }
        result_file.close();
    }
}

void write_to_log_file(std::shared_ptr<std::map<std::thread::id,
                                                std::vector<std::string>>> &LogMapPtr,
                       std::string &log_file_name)
{
    // Convert the map to a vector
    std::vector<std::pair<std::thread::id, std::vector<std::string>>>
        vec(LogMapPtr->begin(), LogMapPtr->end());
    // Sort the vector using a lambda function
    std::sort(vec.begin(), vec.end(),
              [](const std::pair<std::thread::id, std::vector<std::string>> &a,
                 const std::pair<std::thread::id, std::vector<std::string>> &b)
              { return a.second.size() > b.second.size(); });

    // add .log to result file name
    std::string log_file_name_ = log_file_name + ".log";
    // open file
    std::ofstream log_file(log_file_name_, std::ios::out);
    // check if file is open
    if (!log_file)
    {
        // information if file is not open
        std::cerr << "Failed to open log file!" << std::endl;
    }
    else
    {
        // write data to file
        for (auto &[threadId, data] : vec)
        {
            log_file << std::hash<std::thread::id>{}(threadId) << ": ";
            for (auto i = data.begin(); i != data.end(); i++)
            {
                if (i == std::prev(data.end()))
                    log_file << *i << std::endl;
                else
                    log_file << *i << ", ";
            }
        }
        log_file.close();
    }
}

void find_pattern_word(const std::filesystem::directory_entry &entry, const std::string &word,
                       std::shared_ptr<std::vector<std::vector<std::string>>> &results_ptr,
                       std::shared_ptr<Terminal_output_info> &info_ptr,
                       std::shared_ptr<std::map<
                           std::thread::id, std::vector<std::string>>>
                           &LogMapPtr,
                       std::mutex &mutex_)
{

    // open file to read
    std::ifstream file(entry.path());
    if (file.good())
    {
        // flag which inform us if any pattern was found in file
        bool pattern_found = false;
        // variables where we store lines from file and number of line
        int line_number = 0;
        std::string line;
        // get id of thread for log file
        auto id = std::this_thread::get_id();

        // vector which help put data into result file
        std::vector<std::string> results;

        // read file line by line
        while (std::getline(file, line))
        {
            // increase line number
            line_number++;
            // check if line in file has pattern word
            if (line.find(word) != std::string::npos)
            {
                // create line to write into result file
                std::string result_line = entry.path().generic_string() + ':' +
                                          std::to_string(line_number) + ": " + line;
                // add line to results vector
                results.push_back(result_line);
                // lock mutex because we add data to data which is shared by many threads
                std::unique_lock<std::mutex> lock(mutex_);
                // increase number of found patterns in all files
                info_ptr->num_found_patterns++;
                // increase number of found files with pattern
                if (!pattern_found)
                {
                    pattern_found = true;
                    info_ptr->num_files_with_pattern++;
                }
            }
        }
        // lock mutex again outside while loop
        std::unique_lock<std::mutex> lock(mutex_);
        // increase number of searched files
        info_ptr->num_searched_files++;
        // check if in map already exist actual threat id if exist then add new element to vector
        // of strings, if not add new element to map
        auto it = LogMapPtr->find(id);
        if (it == LogMapPtr->end())
        {
            LogMapPtr->insert(std::make_pair(id, std::vector<std::string>{
                                                     entry.path().filename().generic_string()}));
        }
        else
        {
            it->second.push_back(entry.path().filename().generic_string());
        }

        results_ptr->push_back(results);
        results.clear();
    }
}

void find_files(const fs::path &dir, const std::string &word, ThreadPool &pool,
                std::vector<std::future<void>> &futures, std::string &result_file_name,
                std::shared_ptr<Terminal_output_info> &info_ptr, std::string &log_file_name)
{
    // create shared pointer where data to result file will be stored
    std::shared_ptr<std::vector<std::vector<std::string>>> results_ptr =
        std::make_shared<std::vector<std::vector<std::string>>>();

    // create shared pointer to map where data to log file will be stored
    std::shared_ptr<std::map<std::thread::id, std::vector<std::string>>>
        LogMapPtr(new std::map<std::thread::id, std::vector<std::string>>);

    // create a mutex to safely write data to elements shared by many threads
    std::mutex mutex_;

    // search for files in folders
    for (const auto &entry : fs::recursive_directory_iterator(dir))
    {
        // check if file is ok
        if (fs::is_regular_file(entry) && fs::file_size(entry) > 0)
        {
            //  create task for threat pool which looking for pattern word in file
            auto future = pool.submit(find_pattern_word, entry, word, results_ptr, info_ptr, LogMapPtr, std::ref(mutex_));

            futures.push_back(std::move(future));
        }
    }

    // wait until all threads finish their work
    for (auto &future : futures)
    {
        future.wait();
    }

    // write data to log file
    write_to_log_file(LogMapPtr, log_file_name);

    // write data to result file
    write_to_result_file(results_ptr, result_file_name);
}

void print_terminal_info(std::shared_ptr<Terminal_output_info> &info_ptr)
{
    std::cout << "Searched files: " << info_ptr->num_searched_files << std::endl;
    std::cout << "Files with pattern: " << info_ptr->num_files_with_pattern << std::endl;
    std::cout << "Patterns number: " << info_ptr->num_found_patterns << std::endl;
    std::cout << "Result file: " << info_ptr->result_path << std::endl;
    std::cout << "Log file: " << info_ptr->log_path << std::endl;
    std::cout << "Used threads: " << info_ptr->num_threads << std::endl;
    std::cout << "Elapsed time: " << info_ptr->elapsed_time << " s" << std::endl;
}

bool find_errors(std::string &dir_param_str, std::filesystem::path &dir_param,
                 int &num_threads_param, std::string &pattern)
{
    // convert string dictonary paremeter to fs::path type
    if (std::filesystem::exists(dir_param_str))
    {
        dir_param = dir_param_str;
    }
    else
    {
        // if given path is wrong then print information and end program
        std::cerr << "Wrong filepath!" << std::endl;
        std::cerr << "Example filepath: C:\\Users\\Public" << std::endl;
        return 1;
    }

    // check if there is right number of threads
    if (num_threads_param <= 0)
    {
        std::cerr << "Wrong number of threads!" << std::endl;
        std::cerr << "Number of threads must be a positive number" << std::endl;
        return 1;
    }

    // check if there is argument with pattern word which is required
    if (pattern.size() == 0)
    {
        std::cerr << "Error: No pattern argument" << std::endl;
        std::cerr << "Program should be executed as below:" << std::endl;
        std::cerr << "./Specific_grep \"pattern_word\" " << std::endl;
        return 1;
    }

    return 0;
}