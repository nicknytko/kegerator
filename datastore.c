#include "datastore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_STORE_PATH "brew.db"

static struct brewdata_t *brewData = NULL;
static uint8_t numBrews = 0;

int dataStoreInit() {
    dataStoreLoad();
    return 0;
}

void dataStoreQuit() {
    dataStoreSave();
    for (uint8_t i = 0; i < numBrews; i++) {
        free(brewData[i].name);
    }
    free(brewData);
}

void dataStoreSave() {
    FILE *brewdb = fopen(DATA_STORE_PATH, "wb");
    fwrite(&numBrews, sizeof(numBrews), 1, brewdb);

    for (uint32_t i = 0; i < numBrews; i++) {
        struct brewdata_t *data = brewData + i;
        uint32_t nameLength = strlen(data->name);

        fwrite(&nameLength, sizeof(uint32_t), 1, brewdb);
        fwrite(data->name, sizeof(char), nameLength, brewdb);
        fwrite(&(data->abv), sizeof(uint32_t), 1, brewdb);
        fwrite(&(data->mLRemaining), sizeof(uint64_t), 1, brewdb);
    }

    fclose(brewdb);
}

void dataStoreLoad() {
    FILE *brewdb = fopen(DATA_STORE_PATH, "rb");
    if (brewdb == NULL) {
        return;
    }

    fread(&numBrews, sizeof(numBrews), 1, brewdb);

    if (brewData != NULL) {
        free(brewData);
    }
    brewData = calloc(numBrews, sizeof(struct brewdata_t));

    for (uint32_t i = 0; i < numBrews; i++) {
        struct brewdata_t *data = brewData + i;
        uint32_t nameLength;

        fread(&nameLength, sizeof(uint32_t), 1, brewdb);
        data->name = (char *)calloc(nameLength + 1, sizeof(char));
        fread(data->name, sizeof(char), nameLength, brewdb);
        data->name[nameLength] = '\0';

        fread(&(data->abv), sizeof(uint32_t), 1, brewdb);
        fread(&(data->mLRemaining), sizeof(uint64_t), 1, brewdb);
    }

    fclose(brewdb);
}

void dataStoreInsertElement(struct brewdata_t *data) {
    struct brewdata_t *oldData = brewData;
    brewData = calloc(numBrews + 1, sizeof(struct brewdata_t));
    memcpy(brewData, oldData, numBrews * sizeof(struct brewdata_t));
    memcpy(brewData + numBrews, data, sizeof(struct brewdata_t));

    struct brewdata_t *inserted = brewData + numBrews;
    inserted->name = calloc(strlen(data->name) + 1, sizeof(char));
    strcpy(inserted->name, data->name);

    free(oldData);
    numBrews++;
}

void dataStoreRemoveElement(uint32_t index) {
    if (index < numBrews) {
        for (uint32_t i = index; i < numBrews; i++) {
            memcpy(brewData + i, brewData + i + 1, sizeof(struct brewdata_t));
        }
        brewData = realloc(brewData, sizeof(struct brewdata_t) * numBrews - 1);
        numBrews--;
    }
}

void dataStoreInsert(const char *name, uint32_t abv, uint64_t mLRemaining) {
    struct brewdata_t data;
    data.name = (char *)name;
    data.abv = abv;
    data.mLRemaining = mLRemaining;

    dataStoreInsertElement(&data);
}

struct brewdata_t *dataStoreGetBrews() {
    return brewData;
}

uint32_t dataStoreGetNumBrews() { return numBrews; }
