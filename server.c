#include <stdio.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "daemon_ipc.h"
#include "datastore.h"
#include "flowsensor.h"
#include "mongoose.h"

#define HTTP_PORT "80"
#define HTTP_JSON_HEADER                                \
    "HTTP/1.1 200 OK\r\nTransfer-Encoding: "            \
    "chunked\r\nContent-Type: application/json\r\n\r\n"
#define BROADCAST_DELAY 25 // milliseconds

static struct mg_serve_http_opts httpServerOptions;
static int serverRunning = true;

static void broadcast(struct mg_connection *sender, const char *buffer,
                      unsigned int buflen) {
    struct mg_connection *current;
    for (current = mg_next(sender->mgr, NULL); current != NULL;
         current = mg_next(sender->mgr, current)) {
        if (current != sender && (current->flags & 0x100) != 0) {
            mg_send_websocket_frame(current, WEBSOCKET_OP_TEXT, buffer, buflen);
        }
    }
}

static void handleRestApi(struct mg_connection *connection,
                          struct http_message *http) {
    mg_printf(connection, HTTP_JSON_HEADER);
    mg_printf_http_chunk(connection, "{\n");
    struct brewdata_t *brews = dataStoreGetBrews();
    uint8_t first = true;

    for (uint32_t i = 0; i < dataStoreGetNumBrews(); i++) {
        struct brewdata_t *data = brews + i;
        if (!first) {
            mg_printf_http_chunk(connection, ",\n");
        }
        first = false;

        mg_printf_http_chunk(connection, "\"%s\" : {\n", data->name);
        mg_printf_http_chunk(connection, "\"abv\": %u.%u,\n", data->abv / 10,
                             data->abv % 10);
        mg_printf_http_chunk(connection, "\"remaining_ml\": %u \n}",
                             data->mLRemaining);
    }
    mg_printf_http_chunk(connection, "}");
    mg_send_http_chunk(connection, "", 0);
}

static void eventHandler(struct mg_connection *connection, int event,
                         void *eventData) {
    switch (event) {
    case MG_EV_WEBSOCKET_FRAME: {
        struct websocket_message *wm = (struct websocket_message *)eventData;
        if (strncmp((const char *)wm->data, "zero", wm->size) == 0) {
            // TODO: make this better
#define TAP_BREW_NUMBER 0
#define ML_PER_PULSE 0.620921f
            unsigned int pulses = flowSensorGetPulses();
            unsigned int ml = (unsigned int)((float)pulses * ML_PER_PULSE);
            struct brewdata_t *brews = dataStoreGetBrews();
            brews[TAP_BREW_NUMBER].mLRemaining -= ml;

            flowSensorResetPulses();
        }
        break;
    }
    case MG_EV_HTTP_REQUEST: {
        struct http_message *http = (struct http_message *)eventData;
#ifdef DEBUG_PRINT
        printf("Received request for: %.*s\n", http->uri.len, http->uri.p);
#endif
        if (mg_vcmp(&(http->uri), "/api/") == 0) {
            handleRestApi(connection, http);
        } else {
            mg_serve_http(connection, eventData, httpServerOptions);
        }
        break;
    }
    default:
        break;
    }
}

static int requestTestHeader(int client) {
    char header[sizeof(IPC_CALL_HEADER)];
    ssize_t bytes = read(client, header, sizeof(IPC_CALL_HEADER));

    return strncmp(header, IPC_CALL_HEADER, sizeof(IPC_CALL_HEADER)) == 0;
}

static void handleDaemonRequest(int client) {
    if (!requestTestHeader(client)) {
        return;
    }

    enum daemon_ipc_type ipcType;
    read(client, &ipcType, sizeof(enum daemon_ipc_type));

    switch (ipcType) {
    case DAEMON_QUIT:
        serverRunning = false;
        break;
    case DAEMON_ADD_BREW: {
        struct daemon_ipc_brew_data_t data;
        read(client, &data, sizeof(struct daemon_ipc_brew_data_t));
        dataStoreInsert(data.brewName, data.abv, data.mLRemaining);
        break;
    }
    case DAEMON_LIST_BREWS: {
        struct daemon_ipc_brew_data_t *brews;
        uint32_t brewCount = dataStoreGetNumBrews();
        struct brewdata_t *brewData = dataStoreGetBrews();
        brews = calloc(brewCount, sizeof(struct daemon_ipc_brew_data_t));

        for (uint32_t i = 0; i < brewCount; i++) {
            strncpy(brews[i].brewName, brewData[i].name, 128);
            brews[i].abv = brewData[i].abv;
            brews[i].mLRemaining = brewData[i].mLRemaining;
        }
        write(client, &brewCount, sizeof(uint32_t));
        write(client, brews, sizeof(struct daemon_ipc_brew_data_t) * brewCount);

        free(brews);
        break;
    }
    case DAEMON_RENAME_BREW: {
        struct daemon_ipc_brew_rename_t *rename =
            malloc(sizeof(struct daemon_ipc_brew_rename_t));
        read(client, rename, sizeof(struct daemon_ipc_brew_rename_t));

        if (rename->index < dataStoreGetNumBrews()) {
            struct brewdata_t *brews = dataStoreGetBrews();
            free(brews[rename->index].name);
            char *newNameStr = (char *)malloc(strlen(rename->newName) + 1);
            strcpy(newNameStr, rename->newName);
            brews[rename->index].name = newNameStr;
        }
        free(rename);
    }
    case DAEMON_REMOVE_BREW: {
        uint32_t index;
        read(client, &index, sizeof(uint32_t));
        dataStoreRemoveElement(index);
        break;
    }
    default:
    case DAEMON_NONE:
    case DAEMON_UNKNOWN_TYPE:
        break;
    }
}

void serverQuit() { serverRunning = false; }

void serverMain(int socket) {
    /* Initialise mongoose server junk */

    struct mg_mgr manager;
    struct mg_connection *connection;
    mg_mgr_init(&manager, NULL);
    connection = mg_bind(&manager, HTTP_PORT, eventHandler);
    mg_set_protocol_http_websocket(connection);
    httpServerOptions.document_root = "http";
    httpServerOptions.enable_directory_listing = "false";

    /* Websocket broadcast delay */

    long lastBroadcast = clock() - BROADCAST_DELAY;

    /* For calculating flow rate */

    const int FLOW_RATE_AVG_AMT = 50;
    double avgFlowRate = 0;

    while (serverRunning) {
        mg_mgr_poll(&manager, 200);
        double instFlowRate = flowSensorGetFrequency();
        avgFlowRate += (instFlowRate - avgFlowRate) / FLOW_RATE_AVG_AMT;

        if ((clock() - lastBroadcast) > BROADCAST_DELAY) {
            unsigned int pulses = flowSensorGetPulses();
            char broadcastData[64];

            snprintf(broadcastData, 64, "pulses: %i", pulses);
            broadcast(connection, broadcastData, strlen(broadcastData));
            snprintf(broadcastData, 64, "flow: %f", avgFlowRate);
            broadcast(connection, broadcastData, strlen(broadcastData));

            lastBroadcast = clock();
        }

        int client = accept(socket, NULL, NULL);
        if (client >= 0) {
            handleDaemonRequest(client);
            close(client);
        }
    }
    mg_mgr_free(&manager);
}
