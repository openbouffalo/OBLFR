#ifndef IPC_H
#define IPC_H
#include "oblfr_mailbox.h"
/** 
 * @brief Device Status Enum
 */
typedef enum device_status_e {
    DEVICE_DOWN, /*<- Device is Down */
    DEVICE_UP,   /*<- Device is up and ready to transmit/recieve */
} device_status_t;

/**
 * @brief Opaque handle to RPMSG endpoint
 */
typedef struct oblfr_queue_entry_s oblfr_queue_entry_t;

/**
 * @brief RPMSG callback function
 */
typedef void (*oblfr_rpmsg_cb_t)(void* data, size_t len, void* priv);

/* forward declaration */
typedef struct oblfr_device_cfg_s oblfr_device_cfg_t;

/**
 * @brief RPMSG Device Status Change Callback
 */
typedef void (*oblfr_rpmsg_device_status_cb_t)(oblfr_device_cfg_t *device, device_status_t status);

/**
 * @brief RPMSG endpoint configuration
 *
 * Configuresa rpmsg endpoint for communication with the remote processor
 * 
 * @param name Name of the endpoint
 * @param cb Callback function to be called when data is received
 * @param priv Private data to be passed to the callback function
 */
typedef struct oblfr_device_cfg_s
{
    char name[16];
    oblfr_rpmsg_cb_t cb;
    oblfr_rpmsg_device_status_cb_t status_cb;
    void *priv;
} oblfr_device_cfg_t;

/**
 * @brief Initialize a rpmsg device endpoint
 * 
 * @param cfg Endpoint configuration
 * @return Opaque handle to the endpoint
 */
oblfr_queue_entry_t *oblfr_rpmsg_device_add(oblfr_device_cfg_t *cfg);

/**
 * @brief Remove a rpmsg device endpoint
 * 
 * @param device Opaque handle to the endpoint
 * @return  OBLFR_OK on success
 *          OBLFR_ERROR on failure
 */
oblfr_err_t oblfr_rpmsg_device_remove(oblfr_queue_entry_t *device);

/**
 * @brief Send data to a rpmsg device endpoint
 * 
 * This uses a user allocated buffer and copies the data to shared memory for 
 * transmission to the remote processor
 * 
 * @param device Opaque handle to the endpoint
 * @param data Data to send
 * @param len Length of the data
 * @param timeout Timeout in milliseconds to wait for the data to be sent
 * @return  OBLFR_OK on success
 *          OBLFR_ERROR on failure
 */
oblfr_err_t oblfr_rpmsg_device_send(oblfr_queue_entry_t *device, void *data, size_t len, OBLFR_Timeout timeout);

/** 
 * @brief Allocate a buffer for sending data to a rpmsg device endpoint for zerocopy sends
 * 
 * @param device Opaque handle to the endpoint
 * @param out size the total size of the buffer returned
 * @param timeout Timeout in milliseconds to wait for the buffer to be allocated
 * @return Pointer to the buffer or NULL on error
 */
void  *oblfr_rpmsg_device_send_buffer_alloc(oblfr_queue_entry_t *device, uint32_t *size, OBLFR_Timeout timeout);

/**
 * @brief Send a zerocopy buffer to a rpmsg device endpoint
 * 
 * @param device Opaque handle to the endpoint
 * @param buffer Buffer to send (must be allocated with oblfr_rpmsg_device_send_buffer_alloc)
 * @param len Length of the message in the buffer
 * @return  OBLFR_OK on success
 *          OBLFR_ERROR on failure
 */
oblfr_err_t oblfr_rpmsg_device_send_buffer(oblfr_queue_entry_t *device, void *buffer, size_t len);

/** 
 * @brief get the maximum size of a message that can be sent to a rpmsg device endpoint
 */
uint32_t oblfr_rpmsg_get_mtu(void);

/** 
 * @brief Check if communication with the remote processor is ready
 * 
 * @return true if communication is ready
 *         false if communication is not ready
 */
bool oblfr_rpmsg_is_ready(void);

/**
 * @brief Initialize the RPMSG communication
 * 
 * Starts a background task that handles communication with the remote processor 
 * 
 * @return OBLFR_OK on success
 *         OBLFR_ERROR on failure
 */
oblfr_err_t init_rpmsg();

/** 
 * @brief dump the internal state of the rpmsg communication
 * 
 * @return OBLFR_OK on success
 */
oblfr_err_t oblfr_rpmsg_dump(void);

#endif