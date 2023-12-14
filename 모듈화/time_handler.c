#include "time_handler.h"
#include <time.h>

void get_time_string(char *buffer, int size) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, size, "(%H:%M)", timeinfo);
}
