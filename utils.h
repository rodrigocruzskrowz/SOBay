#ifndef SOBAY_UTILS_H
#define SOBAY_UTILS_H

#include "stdio.h"
#include "ctype.h"
#include "string.h"
#include "unistd.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include "stdlib.h"

#include "frontend.h"

#include <errno.h>

#define MAX_SIZE 256
#define MAX_SIZE_FIFO 32
#define MAX_USERS 20
#define MAX_PROMOTORES 10
#define MAX_ITEMS 30

#define BKND_FIFO "bknd"
#define FRND_FIFO "user_%d"

#endif //SOBAY_UTILS_H
