#ifndef DAEMON_IPC_H_
#define DAEMON_IPC_H_

/* To prevent junk getting sent to our server, require a header
   on each request */

#include <stdint.h>
#define IPC_CALL_HEADER "rpi_kegerator"

enum daemon_ipc_type {
    DAEMON_NONE = 0,
    DAEMON_QUIT,
    DAEMON_ADD_BREW,
    DAEMON_LIST_BREWS,
    DAEMON_RENAME_BREW,
    DAEMON_REMOVE_BREW,
    DAEMON_UNKNOWN_TYPE
};

struct daemon_ipc_brew_data_t {
    char brewName[128];
    uint32_t abv;
    uint64_t mLRemaining;
};

struct daemon_ipc_brew_rename_t {
    uint32_t index;
    char newName[128];
};

#endif
