/** @file
 *  @brief Nordic NUS central sample application
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <misc/byteorder.h>
#include <gatt/nus.h>

/* AUTH_NUMERIC_COMPARISON result in in LESC Numeric Comparison authentication
 * Undefine this result in LESC Passkey Input
 */
#define AUTH_NUMERIC_COMPARISON

/** Start security procedure from Peripheral to NUS Central on nRF5 or SmartPhone */
/** BT_SECURITY_LOW(1)     No encryption and no authentication. */
/** BT_SECURITY_MEDIUM(2)  Encryption and no authentication (no MITM). */
/** BT_SECURITY_HIGH(3)    Encryption and authentication (MITM). */
/** BT_SECURITY_FIPS(4)    Authenticated Secure Connections */
#define BT_SECURITY     BT_SECURITY_FIPS

static struct bt_conn *default_conn;

static struct bt_uuid_128 nus_uuid = BT_UUID_INIT_128(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

static u8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, u16_t length)
{
	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	printk("[NOTIFICATION] data %c length %u\n", *(char *)data, length);

	return BT_GATT_ITER_CONTINUE;
}

static u8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("Discover complete\n");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	printk("[ATTRIBUTE] handle %u\n", attr->handle);

	if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_NUS)) {
  	    printk("BT_UUID_NUS found\n");
        memcpy(&nus_uuid, BT_UUID_NUS_RX, sizeof(nus_uuid));
		discover_params.uuid = &nus_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_NUS_RX)) {
  	    printk("BT_UUID_NUS_RX found\n");
        memcpy(&nus_uuid, BT_UUID_NUS_TX, sizeof(nus_uuid));
		discover_params.uuid = &nus_uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_NUS_TX)) {
      	printk("BT_UUID_NUS_TX found\n");
        struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
		memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = attr->handle + 1;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else {
  	    printk("BT_UUID_GATT_CCC found\n");
		subscribe_params.notify = notify_func;
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("[SUBSCRIBED]\n");
		}

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_STOP;
}

static void connected(struct bt_conn *conn, u8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);

#if 0
	if (conn == default_conn) {
    memcpy(&nus_uuid, BT_UUID_NUS, sizeof(nus_uuid));
		discover_params.uuid = &nus_uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = 0x0001;
		discover_params.end_handle = 0xffff;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

		int err = bt_gatt_discover(default_conn, &discover_params);
		if (err) {
			printk("Discover failed(err %d)\n", err);
			return;
		}
	}
#endif
}

static bool eir_found(u8_t type, const u8_t *data, u8_t data_len,
		      void *user_data)
{
	bt_addr_le_t *addr = user_data;

  //printk("[AD]: %u data_len %u\n", type, data_len);

	switch (type) {
	case BT_DATA_UUID128_SOME:
	case BT_DATA_UUID128_ALL:
	  {
  		if (data_len % 16 != 0) {
  			printk("AD malformed\n");
  			return true;
  		}
			struct bt_uuid* uuid;
			u8_t val[16];
			memcpy(val, data, 16);
			uuid = BT_UUID_DECLARE_128(val[0], val[1], val[2], val[3], val[4],
			                           val[5], val[6], val[7], val[8], val[9],
			                           val[10], val[11], val[12], val[13], val[14], val[15]);
			if (bt_uuid_cmp(uuid, BT_UUID_NUS)) {
				return false;
			}
			int err = bt_le_scan_stop();
			if (err) {
				printk("Stop LE scan failed (err %d)\n", err);
				return false;
			}

			default_conn = bt_conn_create_le(addr, BT_LE_CONN_PARAM_DEFAULT);
			return false;
		}
	}

	return true;
}

static void ad_parse(struct net_buf_simple *ad,
		     bool (*func)(u8_t type, const u8_t *data,
				  u8_t data_len, void *user_data),
		     void *user_data)
{
	while (ad->len > 1) {
		u8_t len = net_buf_simple_pull_u8(ad);
		u8_t type;

		/* Check for early termination */
		if (len == 0) {
			return;
		}

		if (len > ad->len) {
			printk("AD malformed\n");
			return;
		}

		type = net_buf_simple_pull_u8(ad);

		if (!func(type, ad->data, len - 1, user_data)) {
			return;
		}

		net_buf_simple_pull(ad, len - 1);
	}
}

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
	       dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_LE_ADV_IND || type == BT_LE_ADV_DIRECT_IND) {
		/* TODO: Move this to a place it can be shared */
		ad_parse(ad, eir_found, (void *)addr);
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
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

	if (level == BT_SECURITY_FIPS && conn == default_conn) {
        memcpy(&nus_uuid, BT_UUID_NUS, sizeof(nus_uuid));
		discover_params.uuid = &nus_uuid.uuid;
		discover_params.func = discover_func;
		discover_params.start_handle = 0x0001;
		discover_params.end_handle = 0xffff;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

		int err = bt_gatt_discover(default_conn, &discover_params);
		if (err) {
			printk("Discover failed(err %d)\n", err);
			return;
		}
	}
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

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

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

#if defined(AUTH_NUMERIC_COMPARISON)
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
static void auth_passkey_entry(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Pairing entry for %s\n", addr);
  if (conn == default_conn)
  {
    err = bt_conn_auth_passkey_entry(conn, 0x12345);
  	if (err) {
      printk("Entry failed(err %d)\n", err);
    }
    else
    {
  	  printk("Entered!\n");
  	}
  }
}

/* result in KEYBOARD and Passkey Input
 * check bt_conn_get_io_capa() in subsys/bluetooth/host/conn.c
 */
static struct bt_conn_auth_cb auth_cb_disaply_keyboard = {
	.passkey_display    = auth_passkey_display,
	.passkey_entry      = auth_passkey_entry,
	.passkey_confirm    = auth_passkey_confirm,
	.cancel             = auth_cancel,
	.pairing_confirm    = auth_pairing_confirm
};
#endif

void main(void)
{
	int err;
	err = bt_enable(NULL);

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);
#if defined(AUTH_NUMERIC_COMPARISON)
	bt_conn_auth_cb_register(&auth_cb_display_yesno);
#else
	bt_conn_auth_cb_register(&auth_cb_disaply_keyboard);
#endif
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);

	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}
