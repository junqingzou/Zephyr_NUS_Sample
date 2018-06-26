/** @file
 *  @brief Nordic NUS sample application
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

#include <gatt/nus.h>

/* AUTH_NUMERIC_COMPARISON result in in LESC Numeric Comparison authentication
 * Undefine this result in LESC Passkey Input
 */
#define AUTH_NUMERIC_COMPARISON

#define DEVICE_NAME		    CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

struct bt_conn *default_conn;
/** Start security procedure from Peripheral to NUS Central on nRF5 or SmartPhone */
/** BT_SECURITY_LOW(1)     No encryption and no authentication. */
/** BT_SECURITY_MEDIUM(2)  Encryption and no authentication (no MITM). */
/** BT_SECURITY_HIGH(3)    Encryption and authentication (MITM). */
/** BT_SECURITY_FIPS(4)    Authenticated Secure Connections !!CURRENTLY NOT USABLE!!*/
#define BT_SECURITY     BT_SECURITY_FIPS
bt_security_t           g_level = BT_SECURITY_NONE;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
	} 

  default_conn = bt_conn_ref(conn);
	printk("Connected\n");

  int ret = bt_conn_security(default_conn, BT_SECURITY);
  if (ret) {
		printk("Failed to set security (err %d)\n", ret);
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

#if defined(CONFIG_BT_SMP)
static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	printk("Identity resolved %s -> %s\n", addr_rpa, addr_identity);
}

static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Security changed: %s level %u\n", addr, level);
	g_level = level;
}
#endif /* defined(CONFIG_BT_SMP) */

static struct bt_conn_cb conn_callbacks = {
	.connected          = connected,
	.disconnected       = disconnected,
	.le_param_req       = NULL,
	.le_param_updated   = NULL,
#if defined(CONFIG_BT_SMP)
	.identity_resolved  = identity_resolved,
	.security_changed   = security_changed
#endif /* defined(CONFIG_BT_SMP) */
};

static void nus_data_handler(ble_nus_data_evt_t * p_evt)
{
   printk("NUS data received, len: %d, data: %c\n", 
     p_evt->rx_data.length, *(p_evt->rx_data.p_data));
}

static void bt_ready(int err)
{
    ble_nus_init_t init = {
      .data_handler = nus_data_handler
    };
     
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = nus_init(&init);
	if (err) {
		printk("NUS failed to init (err %d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

  bt_conn_auth_cancel(conn);
	printk("Pairing cancelled: %s\n", addr);
}

static void auth_pairing_confirm(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Pairing Confirm for %s\n", addr);
  if (conn == default_conn)
  {
    err = bt_conn_auth_pairing_confirm(conn);
  	if (err) {
      printk("Confirm failed(err %d)\n", err);
    }
    else
    {
  	  printk("Confirmed!\n");
  	}
  }
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey Display for %s: %06u\n", addr, passkey);
}

#if defined(AUTH_NUMERIC_COMPARISON)
static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Passkey Confirm for %s: %06u\n", addr, passkey);
  if (conn == default_conn)
  {
    err = bt_conn_auth_passkey_confirm(conn);
  	if (err) {
      printk("Confirm failed(err %d)\n", err);
    }
    else
    {
  	  printk("Confirmed!\n");
  	}
  }
}

/* result in DISPLAY_YESNO and Numeric Comparison
 * check bt_conn_get_io_capa() in subsys/bluetooth/host/conn.c
 */
static struct bt_conn_auth_cb auth_cb_display_yesno = {
	.passkey_display    = auth_passkey_display,
	.passkey_entry      = NULL,
	.passkey_confirm    = auth_passkey_confirm,
	.cancel             = auth_cancel,
	.pairing_confirm    = auth_pairing_confirm
};

#else
/* result in DISPLAY_ONLY and Passkey Input
 * check bt_conn_get_io_capa() in subsys/bluetooth/host/conn.c
 */
static struct bt_conn_auth_cb auth_cb_display_only = {
	.passkey_display    = auth_passkey_display,
	.passkey_entry      = NULL,
	.passkey_confirm    = NULL,
	.cancel             = auth_cancel,
	.pairing_confirm    = auth_pairing_confirm
};
#endif

void main(void)
{
	int err;
	char tx_char;
	int tx_index = 0;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
#if defined(AUTH_NUMERIC_COMPARISON)
	bt_conn_auth_cb_register(&auth_cb_display_yesno);
#else
	bt_conn_auth_cb_register(&auth_cb_display_only);
#endif
	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {
		k_sleep(MSEC_PER_SEC);

    tx_char = 'A' + tx_index%26;
		/* Send NUS notifications */
		if (g_level == BT_SECURITY && nus_notify(default_conn, tx_char) == 0)
		{
		   printk("NUS sent %c\n", tx_char);
		   tx_index++;
		}
	}
}
