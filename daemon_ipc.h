#ifndef DAEMON_IPC_H_
#define DAEMON_IPC_H_

enum daemon_ipc_type
{
    DAEMON_NONE = 0,
    DAEMON_QUIT,
    DAEMON_ADD_BREW,
    DAEMON_LIST_BREWS,
    DAEMON_UNKNOWN_TYPE
};

struct daemon_ipc_brew_data_t
{
    char brewName[128];
    uint32_t abv;
    uint64_t mLRemaining;
};

#endif
