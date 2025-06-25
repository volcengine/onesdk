#include "time.h"

time_t aws_timegm(struct tm *const t) {
   return mktime(t);
}

void aws_localtime(time_t time, struct tm *t) {
    localtime_r(&time, t);
}

void aws_gmtime(time_t time, struct tm *t) {
    gmtime_r(&time, t);
}