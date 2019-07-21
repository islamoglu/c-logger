# c-logger
Simple logging library for C and C++

This library creates a logging thread and creates pipe connections between each thread and logging thread.
Logging thread listen all threads and writes given log datas to a file or /dev/tty.
Logging format can be modified with definitions in c-logger.h header file.
