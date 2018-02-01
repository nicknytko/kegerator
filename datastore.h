#ifndef DATASTORE_H_
#define DATASTORE_H_

#include <stdint.h>

struct brewdata_t {
  char *name;
  uint32_t abv;
  uint64_t mLRemaining;
};

/**
 * Initialize and load the data store into memory.
 * @return 0 if successful
 */
int dataStoreInit();

/**
 * Clean up the data store and free the memory associated with the brew data
 */
void dataStoreQuit();

/**
 * Save the brew data to disk
 */
void dataStoreSave();

/**
 * Load the brew data from disk, freeing any current data from memory.
 */
void dataStoreLoad();

/**
 * Add a new brew element into the data array, full structure.
 */
void dataStoreInsertElement(struct brewdata_t *data);

/**
 * Remove a brew element from the data array.
 */
void dataStoreRemoveElement(uint32_t index);

/**
 * Add a new brew element into the data array, individual arguments.
 */
void dataStoreInsert(const char *name, uint32_t abv, uint64_t mLRemaining);

/**
 * Get an array of all the stored brews
 * @return An array of brewdata_t structs
 */
struct brewdata_t *dataStoreGetBrews();

/**
 * Get the number of brewdata items
 * @return unsigned integer count of items
 */
uint32_t dataStoreGetNumBrews();

#endif
