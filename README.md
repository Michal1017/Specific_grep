# Specific_grep

Program in C++ which behaves quite like grep -r. Program will be looking for pattern word in start directory (default is current directory) and subfolders,
which we give as parameter when we run a program. As an output program print information about
searching on console. Also program produce 2 files.

First file is the log file. This file contains thread ids and file names it's processed,
sorted from the thread id with the most files number and ending with the least.

Second file is the result file. In this file we have information file paths where pattern word was found,
number of line in file where pattern was found and line content.

## How to install

### Linux

1. Make sure you have minimum 3.0.0 version of cmake.
2. Clone repository in destination folder using command:
```
git clone https://github.com/Michal1017/Specific_grep
```
3. Create build folder and go into it:
```
mkdir build
cd build
```
4. Build project using commands:
```
cmake ..
make
```
5. Execute project using command:
```
sudo ./Specific_grep <pattern_word>
```
## How to use

To run program properly we have to pass one obligatory parameter which is pattern word.
Without obligatory parameter program doesn't work and only print information on console how to use it properly.

In project, we can use also 4 optional parameters:

1. ``` -d ``` or ``` --dir ``` specify start directory where program looks for files, default is current directory.
2. ``` -l ``` or ``` --log_file ``` specify name of log file, default it's a name of program
3. ``` -r ``` or ``` --result_file ``` specify name of result file, default it's a name of program
4. ``` -t ``` or ``` --threads ``` specify number of threads in threads pool, default value is 4. 
