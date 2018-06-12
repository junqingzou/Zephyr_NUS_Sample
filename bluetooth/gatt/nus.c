/** @file
 *  @brief Nordic NUS Service
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#include "nus.h"

static struct bt_gatt_ccc_cfg nus_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t nus_rx;
static u8_t nus_tx_started;
static ble_nus_init_t ble_nus = {
  .data_handler = NULL
};

static void nus_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	nus_tx_started = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t on_write_rx(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			const void *buf, u16_t len, u16_t offset,
			u8_t flags)
{
	u8_t *value = attr->user_data;
  
	if (offset + len > sizeof(nus_rx)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);
	
	if (ble_nus.data_handler != NULL)
	{
	   ble_nus_data_evt_t evt;
	   
	   evt.conn             = conn;
	   evt.rx_data.length   = len;
	   evt.rx_data.p_data   = value;
	   ble_nus.data_handler(&evt);
	}

	return len;
}

static ssize_t on_read_rx(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &nus_rx, sizeof(nus_rx));
}

/* NUS Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_NUS),
	/* RX */
	BT_GATT_CHARACTERISTIC(BT_UUID_NUS_RX, BT_GATT_CHRC_WRITE|BT_GATT_CHRC_WRITE_WITHOUT_RESP,
	   BT_GATT_PERM_READ|BT_GATT_PERM_WRITE, on_read_rx, on_write_rx, &nus_rx),
	/* TX */    
	BT_GATT_CHARACTERISTIC(BT_UUID_NUS_TX, BT_GATT_CHRC_NOTIFY,
	   BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(nus_ccc_cfg, nus_ccc_cfg_changed)
};

static struct bt_gatt_service nus_svc = BT_GATT_SERVICE(attrs);

s32_t nus_init(ble_nus_init_t *p_init)
{
    if (p_init->data_handler == NULL)
    {
        return -1;
    }
    else
    {
        ble_nus.data_handler = p_init->data_handler;
    }
    
	return bt_gatt_service_register(&nus_svc);
}

s32_t nus_notify(struct bt_conn *conn, u8_t tx)
{
    u8_t tx_char = tx;
    
	if (!nus_tx_started) 
	{
		return -1;
	}

	return bt_gatt_notify(conn, &attrs[4], &tx_char, sizeof(tx_char));
}

