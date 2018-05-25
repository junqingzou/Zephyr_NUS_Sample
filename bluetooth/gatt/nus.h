/** @file
 *  @brief Nordic NUS Service
 *  http://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.0.0/ble_sdk_app_nus_eval.html?cp=4_0_0_4_1_2_24 
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __NVS_H
#define __NVS_H

#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>

/** @def BT_UUID_NUS
 *  @brief Nordic UART Service
 */
#define BT_UUID_NUS            BT_UUID_DECLARE_128(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E)
/** @def BT_UUID_NUS_RX
 *  @brief NUS RX Service
 */
#define BT_UUID_NUS_RX         BT_UUID_DECLARE_128(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E)
/** @def BT_UUID_NUS_TX
 *  @brief NUS TX Service
 */
#define BT_UUID_NUS_TX         BT_UUID_DECLARE_128(0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E)

/**@brief   Nordic UART Service @ref BLE_NUS_EVT_RX_DATA event data.
 *
 * @details This structure is passed to an event when @ref BLE_NUS_EVT_RX_DATA occurs.
 */
typedef struct
{
    uint8_t const * p_data; /**< A pointer to the buffer with received data. */
    uint16_t        length; /**< Length of received data. */
} ble_nus_evt_rx_data_t;


/**@brief   Nordic UART Service data event structure.
 *
 * @details This structure is passed to an event coming from service.
 */
typedef struct
{
    struct bt_conn  *conn;     /**< Connection referencee. */
    ble_nus_evt_rx_data_t    rx_data;   /**< @ref BLE_NUS_EVT_RX_DATA event data. */
} ble_nus_data_evt_t;


/**@brief Nordic UART Service event handler type. */
typedef void (* ble_nus_data_handler_t) (ble_nus_data_evt_t * p_evt);


/**@brief   Nordic UART Service initialization structure.
 *
 * @details This structure contains the initialization information for the service. The application
 * must fill this structure and pass it to the service using the @ref ble_nus_init
 *          function.
 */
typedef struct
{
    ble_nus_data_handler_t data_handler; /**< Event handler to be called for handling received data. */
} ble_nus_init_t;

 
#ifdef __cplusplus
extern "C" {
#endif

s32_t nus_init(ble_nus_init_t *p_init);
s32_t nus_notify(struct bt_conn *conn, u8_t tx);

#ifdef __cplusplus
}
#endif

#endif /* __NVS_H */ 
