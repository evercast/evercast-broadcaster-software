#include <sys/time.h>

#include "counter_log.h"

typedef struct CounterEntry {
    char *label;
    uint64_t last_flag;
};

static DARRAY(struct CounterEntry *) entries;

void cl_init() {
    da_init(entries);
}

void cl_free() {
    da_free(entries);
}

int cl_assign_label(int code, char *label) {
    while (entries.num <= code) {
        da_push_back_new(entries);
        entries.array[entries.num - 1] = (struct CounterEntry*)bzalloc(sizeof(struct CounterEntry));
    }

    entries.array[code]->label = label;
    return 0;
}

void cl_record(int index, uint64_t flags) {
    char *label;
    uint64_t value;

    if (index >= entries.num) {
        label = "Unknown label";
        value = flags;
    } else {
        if (flags == entries.array[index]->last_flag) {
            return;
        }

        entries.array[index]->last_flag = flags;
        label = entries.array[index]->label;
        value = flags;
    }

    struct timeval now;
    gettimeofday(&now, NULL);

    blog(LOG_DEBUG, "%lu: New value for %s: %d", now.tv_sec * 1000000 + now.tv_usec, label, value);
}