/*
   BlueZ - Bluetooth protocol stack for Linux

   Copyright (C) 2010  Nokia Corporation
   Copyright (C) 2011-2012  Intel Corporation

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS,
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS
   SOFTWARE IS DISCLAIMED.
*/

#ifdef CONFIG_TIZEN_WIP

#define TIZEN_OP_CODE_BASE	0xff00
#define TIZEN_EV_BASE		0xff00

#define MGMT_OP_SET_ADVERTISING_PARAMS		(TIZEN_OP_CODE_BASE + 0x01)
struct mgmt_cp_set_advertising_params {
	__le16  interval_min;
	__le16  interval_max;
	__u8 filter_policy;
	__u8 type;
} __packed;
#define MGMT_SET_ADVERTISING_PARAMS_SIZE	6

#define MGMT_OP_SET_ADVERTISING_DATA		(TIZEN_OP_CODE_BASE + 0x02)
struct mgmt_cp_set_advertising_data {
	__u8    data[HCI_MAX_AD_LENGTH];
} __packed;
#define MGMT_SET_ADVERTISING_DATA_SIZE		HCI_MAX_AD_LENGTH
#define MGMT_SET_ADV_MIN_APP_DATA_SIZE		1

#define MGMT_OP_SET_SCAN_RSP_DATA		(TIZEN_OP_CODE_BASE + 0x03)
struct mgmt_cp_set_scan_rsp_data {
	__u8    data[HCI_MAX_AD_LENGTH];
} __packed;
#define MGMT_SET_SCAN_RSP_DATA_SIZE		HCI_MAX_AD_LENGTH
#define MGMT_SET_SCAN_RSP_MIN_APP_DATA_SIZE	1

#define MGMT_OP_ADD_DEV_WHITE_LIST		(TIZEN_OP_CODE_BASE + 0x04)
struct mgmt_cp_add_dev_white_list {
	__u8	bdaddr_type;
	bdaddr_t bdaddr;
} __packed;
#define MGMT_ADD_DEV_WHITE_LIST_SIZE		7

#define MGMT_OP_REMOVE_DEV_FROM_WHITE_LIST	(TIZEN_OP_CODE_BASE + 0x05)
struct mgmt_cp_remove_dev_from_white_list {
	__u8	bdaddr_type;
	bdaddr_t bdaddr;
} __packed;
#define MGMT_REMOVE_DEV_FROM_WHITE_LIST_SIZE	7

#define MGMT_OP_CLEAR_DEV_WHITE_LIST		(TIZEN_OP_CODE_BASE + 0x06)
#define MGMT_OP_CLEAR_DEV_WHITE_LIST_SIZE	0

/* BEGIN TIZEN_Bluetooth :: RSSI monitoring   */
#define MGMT_OP_SET_RSSI_ENABLE			(TIZEN_OP_CODE_BASE + 0x07)
#define MGMT_SET_RSSI_ENABLE_SIZE		10

struct mgmt_cp_set_enable_rssi {
	__s8    low_th;
	__s8    in_range_th;
	__s8    high_th;
	bdaddr_t bdaddr;
	__s8    link_type;
} __packed;

struct mgmt_cc_rsp_enable_rssi {
	__u8     status;
	__u8     le_ext_opcode;
	bdaddr_t bt_address;
	__s8    link_type;
} __packed;

#define MGMT_OP_GET_RAW_RSSI			(TIZEN_OP_CODE_BASE + 0x08)
#define MGMT_GET_RAW_RSSI_SIZE			7

struct mgmt_cp_get_raw_rssi {
	bdaddr_t bt_address;
	__u8     link_type;
} __packed;

#define MGMT_OP_SET_RSSI_DISABLE		(TIZEN_OP_CODE_BASE + 0x09)
#define MGMT_SET_RSSI_DISABLE_SIZE		7
struct mgmt_cp_disable_rssi {
	bdaddr_t   bdaddr;
	__u8     link_type;
} __packed;
struct mgmt_cc_rp_disable_rssi {
	__u8 status;
	__u8 le_ext_opcode;
	bdaddr_t bt_address;
	__s8    link_type;
} __packed;
/* END TIZEN_Bluetooth :: RSSI monitoring */
/* BEGIN TIZEN_Bluetooth :: name update changes */
#define MGMT_EV_DEVICE_NAME_UPDATE		(TIZEN_EV_BASE + 0x01)
struct mgmt_ev_device_name_update {
	struct mgmt_addr_info addr;
	__le16  eir_len;
	__u8    eir[0];
} __packed;
/* END TIZEN_Bluetooth :: name update changes */
/* BEGIN TIZEN_Bluetooth :: Add handling of RSSI Events   */
#define MGMT_EV_RSSI_ALERT			(TIZEN_EV_BASE + 0x02)
struct mgmt_ev_vendor_specific_rssi_alert {
	bdaddr_t bdaddr;
	__s8     link_type;
	__s8     alert_type;
	__s8     rssi_dbm;
} __packed;

#define MGMT_EV_RAW_RSSI			(TIZEN_EV_BASE + 0x03)
struct mgmt_cc_rp_get_raw_rssi {
	__u8     status;
	__s8     rssi_dbm;
	__u8     link_type;
	bdaddr_t bt_address;
} __packed;

#define MGMT_EV_RSSI_ENABLED			(TIZEN_EV_BASE + 0x04)

#define MGMT_EV_RSSI_DISABLED			(TIZEN_EV_BASE + 0x05)
/* END TIZEN_Bluetooth :: Handling of RSSI Events */
#endif   /* CONFIG_TIZEN_WIP */
