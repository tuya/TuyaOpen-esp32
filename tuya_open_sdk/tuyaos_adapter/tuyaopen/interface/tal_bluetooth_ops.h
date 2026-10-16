/**
 * @file tal_bluetooth_ops.h
 * @brief 契约层（interface）：tal_bluetooth 平台 ops 卡 + TAL_BLE_* 镜像类型集
 *
 * **分层定位**：同 tal_api_ops.h——core 与 port 之间的中性契约，物理上放在
 * port 下（每个芯片平台各一份，见 platform/<P>/port/interface）。依赖方向：
 *   core ──include──▶ 本头 ◀──include── port      # 谁都不依赖谁
 *   core ──extern tal_bluetooth_ops_platform / 经卡调用──▶ port   # 上层→下层，允许
 *   port ──▶ core                                  # 禁止（include 与调用都不许）
 *
 * 背景：core 的 iot-core/ble/tal_bluetooth.c 原直调 tkl_ble_*（上游 TKL
 * 平台接口）并在内部用 TKL_BLE_* 类型搭值对象。2026-10 解耦为经卡分发：
 * 本文件定义 TAL_BLUETOOTH_OPS_T（27 成员，与 port 的 port_bluetooth.h 接口
 * 面一一对应）与**镜像类型集**（TAL_BLE_* 前缀，逐字段镜像 port 的
 * port_bluetooth_def.h）；公开类型集在同目录 tal_bluetooth_def.h
 * （core 的公开 API tal_bluetooth.h 亦引用它）。
 *
 * 约定：
 * - port 侧适配层（linux: port/src/port_bt/port_bluetooth_ops.c 镜像强转；
 *   esp32: port/src/port_bluetooth_ops.c 空卡）以
 *   "布局逐字节一致 + 指针强转"接线，并用编译期 sizeof/offsetof 断言兜底
 *   （镜像一旦漂移即编译失败）；
 * - **注册次序约束**：gap/gatt 回调注册必须先于 stack_init 与
 *   gatts_service_add（bluez 层首次调用锁存回调指针，见
 *   tal_bluetooth.c 的 tal_ble_bt_init 次序）；
 * - gatts_service_add 的入参必须非 const（port 回写 handle）；
 * - 无 BLE 能力的平台（卡成员 NULL）由 tal_bluetooth.c 统一优雅降级。
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 *
 */
#ifndef __TAL_BLUETOOTH_OPS_H__
#define __TAL_BLUETOOTH_OPS_H__

#include "tal_bluetooth_def.h"   /* 公开集（TAL_BLE_DATA_T 等复用） */

#ifndef __TAL_BLUETOOTH_OPS_TYPES_H__
#define __TAL_BLUETOOTH_OPS_TYPES_H__

#include "tuya_cloud_types.h"
#include "tuya_error_code.h"

/* 说明：本类型集为 TKL BLE 类型集的 TAL 自持镜像（TAL_BLE_* 前缀）——
 * 卡的成员签名只能用 TAL 类型，port 侧适配层靠"布局逐字节一致 + 强转"
 * 接线（见本文件头的布局断言约定）。
 * 与公开集（tal_bluetooth_def.h）的冲突项已按改名表处理：
 *   DATA_T 复用公开 TAL_BLE_DATA_T；DATA_REPORT_T/SUBSCRBE_EVT_T/READ_CHAR_EVT_T
 *   加 GATT_ 前缀；ADV 报告枚举值加 GAP_ 前缀；SCAN_TYPE 宏不镜像（用位域）。 */

#define TAL_BLE_ROLE_SERVER                                                 (0x01)  /**< Gatt Server Role, for ble peripheral.*/
#define TAL_BLE_ROLE_CLIENT                                                 (0x02)  /**< Gatt Client Role, for ble central. */

#define TAL_BLE_GAP_ADDR_TYPE_PUBLIC                                        (0x00)  /**< Public (identity) address.*/
#define TAL_BLE_GAP_ADDR_TYPE_RANDOM                                        (0x01)  /**< Random (identity) address. */


#define TAL_BLE_GAP_ADV_SET_DATA_SIZE_MAX                                   (31)    /**< Maximum data length for an advertising set.
                                                                                        If more advertising data is required, use extended advertising instead. */
#define TAL_BLE_GAP_ADV_SET_DATA_SIZE_EXTENDED_MAX_SUPPORTED                (255)   /**< Maximum supported data length for an extended advertising set. */

/**
 * Advertising types defined in Bluetooth Core Specification v5.0, Vol 6, Part B, Section 4.4.2.
 * */
#define TAL_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED                      (0x01)  /**< CONN and scannable undirected
                                                                                        advertising events. */
#define TAL_BLE_GAP_ADV_TYPE_CONN_NONSCANNABLE_DIR_HIGHDUTY_CYCLE           (0x02)  /**< CONN non-scannable directed advertising
                                                                                        events. Advertising interval is less that 3.75 ms.
                                                                                        Use this type for fast reconnections.
                                                                                        @note Advertising data is not supported. */
#define TAL_BLE_GAP_ADV_TYPE_CONN_NONSCANNABLE_DIRECTED                     (0x03)  /**< CONN non-scannable directed advertising
                                                                                        events.
                                                                                        @note Advertising data is not supported. */
#define TAL_BLE_GAP_ADV_TYPE_NONCONN_SCANNABLE_UNDIRECTED                   (0x04)  /**< Non-CONN scannable undirected
                                                                                        advertising events. */
#define TAL_BLE_GAP_ADV_TYPE_NONCONN_NONSCANNABLE_UNDIRECTED                (0x05)  /**< Non-CONN non-scannable undirected
                                                                                        advertising events. */
#define TAL_BLE_GAP_ADV_TYPE_EXTENDED_CONN_NONSCANNABLE_UNDIRECTED          (0x06)  /**< CONN non-scannable undirected advertising
                                                                                        events using extended advertising PDUs. */
#define TAL_BLE_GAP_ADV_TYPE_EXTENDED_CONN_NONSCANNABLE_DIRECTED            (0x07)  /**< CONN non-scannable directed advertising
                                                                                        events using extended advertising PDUs. */
#define TAL_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_SCANNABLE_UNDIRECTED          (0x08)  /**< Non-CONN scannable undirected advertising
                                                                                        events using extended advertising PDUs.
                                                                                        @note Only scan response data is supported. */
#define TAL_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_SCANNABLE_DIRECTED            (0x09)  /**< Non-CONN scannable directed advertising
                                                                                        events using extended advertising PDUs.
                                                                                        @note Only scan response data is supported. */
#define TAL_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_NONSCANNABLE_UNDIRECTED       (0x0A)  /**< Non-CONN non-scannable undirected advertising
                                                                                        events using extended advertising PDUs. */
#define TAL_BLE_GAP_ADV_TYPE_EXTENDED_NONCONN_NONSCANNABLE_DIRECTED         (0x0B)  /**< Non-CONN non-scannable directed advertising
                                                                                        events using extended advertising PDUs. */

#define TAL_BLE_GAP_PHY_AUTO                                                (0x00)  /**< Automatic PHY selection.*/
#define TAL_BLE_GAP_PHY_1MBPS                                               (0x01)  /**< 1 Mbps PHY. */
#define TAL_BLE_GAP_PHY_2MBPS                                               (0x02)  /**< 2 Mbps PHY. */
#define TAL_BLE_GAP_PHY_CODED                                               (0x04)  /**< Coded PHY. */

#define TAL_BLE_GAP_ADV_STATE_IDLE                                          (0x00)  /**< Idle, no advertising */
#define TAL_BLE_GAP_ADV_STATE_START                                         (0x01)  /**< Start Advertising. A temporary state, haven't received the result.*/
#define TAL_BLE_GAP_ADV_STATE_ADVERTISING                                   (0x02)  /**< Advertising State */
#define TAL_BLE_GAP_ADV_STATE_STOP                                          (0x04)  /**< Stop Advertising. A temporary state, haven't received the result.*/

#define TAL_BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION                       (0x13)  /**< Remote User Terminated Connection. */
#define TAL_BLE_HCI_REMOTE_DEV_TERMINATION_DUE_TO_LOW_RESOURCES             (0x14)  /**< Remote Device Terminated Connection due to low resources.*/
#define TAL_BLE_HCI_REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF                 (0x15)  /**< Remote Device Terminated Connection due to power off. */
#define TAL_BLE_HCI_LOCAL_HOST_TERMINATED_CONNECTION                        (0x16)  /**< Local Host Terminated Connection. */
            
#define TAL_BLE_GATT_SERVICE_MAX_NUM                                        (6)     /**< [20220614] Increase maximum service into 6 */
#define TAL_BLE_GATT_CHAR_MAX_NUM                                           (6)

#define TAL_BLE_GATT_INVALID_HANDLE                                         (0xFFFF) /**< Invalid Connect Handle */
                    
/**@brief GAP address parameters. */
typedef struct {
    uint8_t     type;                                   /**< Mac Address Type, Refer to @ TAL_BLE_GAP_ADDR_TYPE_PUBLIC or TAL_BLE_GAP_ADDR_TYPE_RANDOM*/
    uint8_t     addr[6];                                /**< Mac Address, Address size, 6 bytes */
} TAL_BLE_GAP_ADDR_T;

 typedef enum {
    TAL_BLE_GAP_ADV_DATA,                                   /**< Adv Data - Only */
    TAL_BLE_GAP_RSP_DATA,                                   /**< Scan Response Data - Only */
    TAL_BLE_GAP_ADV_RSP_DATA,                               /**< Adv Data + Scan Response Data */
    TAL_BLE_GAP_NONCONN_ADV_DATA,                           /**< None-Connectable Adv Data - Only */
    TAL_BLE_GAP_EXTENDED_ADV_DATA,                          /**< [Bluetooth 5.0]Extended Adv Data - Only */
} TAL_BLE_GAP_ADV_TYPE_E;

/**@brief GAP advertising parameters. */
typedef struct {
    uint8_t                 adv_type;                   /**< Adv Type. Refer to TAL_BLE_GAP_ADV_TYPE_CONN_SCANNABLE_UNDIRECTED etc.*/
    TAL_BLE_GAP_ADDR_T      direct_addr;                /**< For Directed Advertising, you can fill in direct address */
    
    uint16_t                adv_interval_min;           /**< Range: 0x0020 to 0x4000  Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec */
    uint16_t                adv_interval_max;           /**< Range: 0x0020 to 0x4000  Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec */
    uint8_t                 adv_channel_map;            /**< Advertising Channel Map, 0x01 = adv channel index 37,  0x02 = adv channel index 38,
                                                                0x04 = adv channel index 39. Default Value: 0x07*/
} TAL_BLE_GAP_ADV_PARAMS_T;

/**@brief GAP adv report parameters. */
typedef struct {
    TAL_BLE_GAP_ADV_TYPE_E  adv_type;                   /**< Advertising report type. Refer to @TAL_BLE_GAP_ADV_TYPE_E */
    TAL_BLE_GAP_ADDR_T      peer_addr;                  /**< Bluetooth address of the peer device. */
    char                  rssi;                       /**< Received Signal Strength Indication in dBm of the last packet received. */
    uint8_t                 channel_index;              /**< Channel Index on which the last advertising packet is received (37-39).channel index = 37, it means that we do advertisement in channel 37. */
    TAL_BLE_DATA_T          data;                       /**< Received advertising or scan response data.  */
} TAL_BLE_GAP_ADV_REPORT_T;

/**@brief GAP scanning parameters. */
typedef struct {
    uint8_t                 extended;                   /**< If 1, the scanner will accept extended advertising packets.
                                                            If set to 0, the scanner will not receive advertising packets
                                                            on secondary advertising channels, and will not be able
                                                            to receive long advertising PDUs. */
    uint8_t                 active : 1;                 /**< [Tuya Need]!!!! If 1, perform active scanning by sending scan requests.
                                                            This parameter is ignored when used with @ref gap_connect. */
    uint8_t                 scan_phys;                  /**< Refer to @TAL_BLE_GAP_PHY_1MBPS. TAL_BLE_GAP_PHY_2MBPS*/
    uint16_t                interval;                   /**< Scan interval in 625 us units. */
    uint16_t                window;                     /**< Scan window in 625 us units. */
    uint16_t                timeout;                    /**< Scan timeout in 10 ms units. */
    uint8_t                 scan_channel_map;           /**< Scan Channel Index, refer to @TAL_BLE_GAP_ADV_PARAMS_T*/
} TAL_BLE_GAP_SCAN_PARAMS_T;

/** @brief  Definition of LE connection request parameter.*/
typedef struct {
    uint16_t                conn_interval_min;          /**< Minimum value for the connection interval.  */
    uint16_t                conn_interval_max;          /**< Maximum value for the connection interval. */
    uint16_t                conn_latency;               /**< Slave latency for the connection in number of connection events.*/
    uint16_t                conn_sup_timeout;           /**< Supervision timeout for the LE Link.*/
    
    uint16_t                connection_timeout;
} TAL_BLE_GAP_CONN_PARAMS_T;

typedef enum {
    TAL_BLE_UUID_TYPE_16,                               /**< UUID 16 bit */
    TAL_BLE_UUID_TYPE_32,                               /**< UUID 32 bit */
    TAL_BLE_UUID_TYPE_128,                              /**< UUID 128 bit */
} TAL_BLE_UUID_TYPE_E;

/** @brief  Bluetooth Low Energy UUID type, encapsulates both 16-bit and 128-bit UUIDs. */
typedef struct {
    TAL_BLE_UUID_TYPE_E     uuid_type;                  /**< UUID Type, Refer to @TAL_BLE_UUID_TYPE_E */

    union {
        uint16_t            uuid16;                     /**< 16-bit UUID value  */
        uint32_t              uuid32;                     /**< 32-bit UUID value */
        uint8_t             uuid128[16];                /**< Little-Endian UUID bytes. 128bit uuid*/
    }uuid;
} TAL_BLE_UUID_T;

/**< GATT characteristic property bit field values */
typedef enum {
    TAL_BLE_GATT_CHAR_PROP_BROADCAST            = 0x01, /**< If set, permits broadcasts of the Characteristic Value using Server Characteristic Configuration Descriptor. */
    TAL_BLE_GATT_CHAR_PROP_READ                 = 0x02, /**< If set, permits reads of the Characteristic Value */
    TAL_BLE_GATT_CHAR_PROP_WRITE_NO_RSP         = 0x04, /**< If set, permit writes of the Characteristic Value without response */
    TAL_BLE_GATT_CHAR_PROP_WRITE                = 0x08, /**< If set, permits writes of the Characteristic Value with response */
    TAL_BLE_GATT_CHAR_PROP_NOTIFY               = 0x10, /**< If set, permits notifications of a Characteristic Value without acknowledgment */
    TAL_BLE_GATT_CHAR_PROP_INDICATE             = 0x20, /**< If set, permits indications of a Characteristic Value with acknowledgment */
    TAL_BLE_GATT_CHAR_PROP_WRITE_AUTHEN_SIGNED  = 0x40, /**< If set, permits signed writes to the Characteristic Value */
    TAL_BLE_GATT_CHAR_PROP_EXT_PROP             = 0x80, /**< If set, additional characteristic properties are defined in the Characteristic */
}TAL_BLE_CHAR_PROP_TYPE_E;

/**< GATT attribute permission bit field values */
typedef enum {
    TAL_BLE_GATT_PERM_NONE                      = 0x01, /**< No operations supported, e.g. for notify-only */
    TAL_BLE_GATT_PERM_READ                      = 0x02, /**< Attribute read permission. */
    TAL_BLE_GATT_PERM_WRITE                     = 0x04, /**< Attribute write permission. */
    TAL_BLE_GATT_PERM_READ_ENCRYPT              = 0x08, /**< Attribute read permission with encryption. */
    TAL_BLE_GATT_PERM_WRITE_ENCRYPT             = 0x10, /**< Attribute write permission with encryption. */
    TAL_BLE_GATT_PERM_READ_AUTHEN               = 0x20, /**< Attribute read permission with authentication. */
    TAL_BLE_GATT_PERM_WRITE_AUTHEN              = 0x40, /**< Attribute write permission with authentication. */
    TAL_BLE_GATT_PERM_PREPARE_WRITE             = 0x80, /**< Attribute prepare write permission. */
} TAL_BLE_ATTR_PERM_E;

typedef struct {
    uint16_t        handle;                             /**< [Output] After init the characteristic, we will get the char-handle, we need to restore it */

    TAL_BLE_UUID_T  char_uuid;                          /**< Characteristics UUID */
    uint8_t         property;                           /**< Characteristics property , Refer to TAL_BLE_CHAR_PROP_TYPE_E */
    uint8_t         permission;                         /**< Characteristics value attribute permission */
    uint8_t         value_len;                          /**< Characteristics value length */
} TAL_BLE_CHAR_PARAMS_T;

typedef enum {
    TAL_BLE_UUID_UNKNOWN                    = 0x0000,   /**< Reserved UUID. */
    TAL_BLE_UUID_SERVICE_PRIMARY            = 0x2800,   /**< Primary Service. */
    TAL_BLE_UUID_SERVICE_SECONDARY          = 0x2801,   /**< Secondary Service. */
    TAL_BLE_UUID_SERVICE_INCLUDE            = 0x2802,   /**< Include. */
    TAL_BLE_UUID_CHARACTERISTIC             = 0x2803,   /**< Characteristic. */
} TAL_BLE_SERVICE_TYPE_E;

typedef struct {
    uint16_t                    handle;                 /**< After init the service, we will get the svc-handle */

    TAL_BLE_UUID_T              svc_uuid;               /**< Service UUID */
    TAL_BLE_SERVICE_TYPE_E      type;                   /**< Service Type */
    
    uint8_t                     char_num;               /**< Number of characteristic */
    TAL_BLE_CHAR_PARAMS_T       *p_char;                /**< Pointer of characteristic */
} TAL_BLE_SERVICE_PARAMS_T;

typedef struct {
    uint8_t                     svc_num;                /**< If we only use service(0xFD50), the svc_num will be set into 1 */
    TAL_BLE_SERVICE_PARAMS_T    *p_service;
} TAL_BLE_GATTS_PARAMS_T;

typedef enum {
    TAL_BLE_EVT_STACK_INIT = 0x01,                      /**< Init Bluetooth Stack, return  refer@TAL_BLE_GAP_PARAMS_EVT_T.result */
    
    TAL_BLE_EVT_STACK_DEINIT,                           /**< Deinit Bluetooth Stack, Requirements from Tuyaos ZT*/

    TAL_BLE_EVT_STACK_RESET,                            /**< Reset Bluetooth Stack, Requirements from Bluetooth Gateway */

    TAL_BLE_GAP_EVT_CONNECT,                            /**< Connected as peripheral role */

    TAL_BLE_GAP_EVT_DISCONNECT,                         /**< Disconnected */

    TAL_BLE_GAP_EVT_ADV_STATE,                          /**< Advertising State */

    TAL_BLE_GAP_EVT_ADV_REPORT,                         /**< Scan result report */

    TAL_BLE_GAP_EVT_CONN_PARAM_REQ,                     /**< Parameter update request */

    TAL_BLE_GAP_EVT_CONN_PARAM_UPDATE,                  /**< Parameter update successfully */
    
    TAL_BLE_GAP_EVT_CONN_RSSI,                          /**< Got RSSI value of link peer device */
} TAL_BLE_GAP_EVT_TYPE_E;

typedef enum {      
    TAL_BLE_GATT_EVT_MTU_REQUEST = 0x01 ,               /**< MTU exchange request event, For Ble peripheral, we need to do reply*/

    TAL_BLE_GATT_EVT_MTU_RSP,                           /**< MTU exchange respond event, For Ble Central, the ble central has finished the MTU-Request */

    TAL_BLE_GATT_EVT_PRIM_SEV_DISCOVERY,                /**< [Ble Central] Discovery Service */

    TAL_BLE_GATT_EVT_CHAR_DISCOVERY,                    /**< [Ble Central] Discovery Characteristics*/

    TAL_BLE_GATT_EVT_CHAR_DESC_DISCOVERY,               /**< [Ble Central] Discovery descriptors */

    TAL_BLE_GATT_EVT_NOTIFY_TX,                         /**< [Ble peripheral] Transfer data Callback, only report Result */

    TAL_BLE_GATT_EVT_WRITE_REQ,                         /**< [Ble Peripheral] Get Client-Write Char Request*/

    TAL_BLE_GATT_EVT_NOTIFY_INDICATE_RX,                /**< [Ble Central] Get Notification or Indification data */

    TAL_BLE_GATT_EVT_READ_RX,                           /**< [Ble Central] Get Char-Read Data */

    TAL_BLE_GATT_EVT_SUBSCRIBE,                         /**< [Ble Peripheral] Subscribe Request*/

    TAL_BLE_GATT_EVT_READ_CHAR_VALUE,                   /**< [Ble Peripheral] read char value event*/

} TAL_BLE_GATT_EVT_TYPE_E;

typedef struct {
    uint8_t                         role;               /**< BLE role for this connection, see @ref TAL_BLE_ROLE_SERVER, or TAL_BLE_ROLE_CLIENT */
    TAL_BLE_GAP_ADDR_T              peer_addr;          /**< Reserved, [Ble Central],For some platform, we will get the peer address after connect one device */
    TAL_BLE_GAP_CONN_PARAMS_T       conn_params;        /**< Report Connection Parameters */
} TAL_BLE_GAP_CONNECT_EVT_T;

typedef struct {
    uint8_t                         role;               /**< BLE role for this disconnection */
    int                           reason;             /**< Report Disconnection Reason */
} TAL_BLE_GAP_DISCONNECT_EVT_T;

typedef struct {
    uint16_t                        char_handle;        /**< Notify Characteristic Handle */
    int                           result;             /**< Notify Result */
} TAL_BLE_NOTIFY_RESULT_EVT_T;

typedef struct {
    TAL_BLE_UUID_T              uuid;               /**< Discovery Service UUID */
    uint16_t                    start_handle;       /**< Discovery Start Handle */
    uint16_t                    end_handle;         /**< Discovery End Handle */
} TAL_BLE_GATT_SVC_HANDLE_T;

typedef struct {
    uint8_t                         svc_num;            /**< Number of Services */

    TAL_BLE_GATT_SVC_HANDLE_T       services[TAL_BLE_GATT_SERVICE_MAX_NUM];
} TAL_BLE_GATT_SVC_DISC_TYPE_T;

typedef struct {
    TAL_BLE_UUID_T              uuid;               /**< Discovery Service UUID */
    uint16_t                    handle;             /**< Discovery Char Handle */
}TAL_BLE_GATT_CHAR_HANDLE_T;

typedef struct {
    uint8_t                         char_num;           /**< Number of Characteristics */

    TAL_BLE_GATT_CHAR_HANDLE_T      characteristics[TAL_BLE_GATT_CHAR_MAX_NUM];
} TAL_BLE_GATT_CHAR_DISC_TYPE_T;

typedef struct {
    uint16_t                        cccd_handle;        /**< Discovery Descriptor Handle, Return CCCD Handle */
} TAL_BLE_GATT_DESC_DISC_TYPE_T;

typedef struct {
    uint16_t                        char_handle;        /**< Specify one characteristic handle */
    TAL_BLE_DATA_T                  report;             /**< Report Data, Refer to @ TAL_BLE_DATA_T */
} TAL_BLE_GATT_DATA_REPORT_T;

typedef struct {
    TAL_BLE_GAP_EVT_TYPE_E              type;           /**< Gap Event */
    uint16_t                            conn_handle;    /**< Connection Handle */
    int                               result;         /**< Will Refer to HOST STACK Error Code */

    union {
        TAL_BLE_GAP_CONNECT_EVT_T       connect;        /**< Receive connect callback, This value can be used with TAL_BLE_GAP_EVT_CONNECT and TAL_BLE_GAP_EVT_CONNECT*/
        TAL_BLE_GAP_DISCONNECT_EVT_T    disconnect;     /**< Receive disconnect callback*/
        TAL_BLE_GAP_ADV_REPORT_T        adv_report;     /**< Receive Adv and Respond report*/
        TAL_BLE_GAP_CONN_PARAMS_T       conn_param;     /**< We will update connect parameters.This value can be used with TAL_BLE_GAP_EVT_CONN_PARAM_REQ and TAL_BLE_GAP_EVT_CONN_PARAM_UPDATE*/
        char                          link_rssi;      /**< Peer device RSSI value */
    }gap_event;
} TAL_BLE_GAP_PARAMS_EVT_T;
typedef struct {
    uint16_t                        char_handle;                /**< Specify one characteristic handle */
    uint8_t                         reason;
    uint8_t                         prev_notify     :    1;     /**< previously subscribed */
    uint8_t                         cur_notify      :    1;     /** currently notifications. */
    uint8_t                         prev_indicate   :    1;     /** previously indications. */
    uint8_t                         cur_indicate    :    1;     /** currently subscribed to indications. */
} TAL_BLE_GATT_SUBSCRIBE_EVT_T;
typedef struct {
    uint16_t                        char_handle;                /**< Specify one characteristic handle */
    uint16_t                        offset;
} TAL_BLE_GATT_READ_CHAR_EVT_T;

typedef struct {
    TAL_BLE_GATT_EVT_TYPE_E             type;           /**< Gatt Event */
    uint16_t                            conn_handle;    /**< Connection Handle */
    int                               result;         /**< Will Refer to HOST STACK Error Code */

    union {
        uint16_t                        exchange_mtu;   /**< This value can be used with TAL_BLE_GATT_EVT_MTU_REQUEST and TAL_BLE_GATT_EVT_MTU_RSP*/
        TAL_BLE_GATT_SVC_DISC_TYPE_T    svc_disc;       /**< Discovery All Service */
        TAL_BLE_GATT_CHAR_DISC_TYPE_T   char_disc;      /**< Discovery Specific Characteristic */
        TAL_BLE_GATT_DESC_DISC_TYPE_T   desc_disc;      /**< Discovery Specific Descriptors*/
        TAL_BLE_NOTIFY_RESULT_EVT_T     notify_result;  /**< This value can be used with TAL_BLE_GATT_EVT_NOTIFY_TX*/
        TAL_BLE_GATT_DATA_REPORT_T           write_report;   /**< This value can be used with TAL_BLE_GATT_EVT_WRITE_REQ*/
        TAL_BLE_GATT_DATA_REPORT_T           data_report;    /**< This value can be used with TAL_BLE_GATT_EVT_NOTIFY_INDICATE_RX*/
        TAL_BLE_GATT_DATA_REPORT_T           data_read;      /**< After we do read attr in central mode, we will get the callback*/
        TAL_BLE_GATT_SUBSCRIBE_EVT_T          subscribe;      /**< ccc callback event, used with TAL_BLE_GATT_EVT_SUBSCRIBE*/
        TAL_BLE_GATT_READ_CHAR_EVT_T         char_read;      /**< read char event, used with TAL_BLE_GATT_EVT_READ_CHAR_VALUE*/
    }gatt_event;
} TAL_BLE_GATT_PARAMS_EVT_T;

/**< GAP Callback Register function definition */
typedef void(*TAL_BLE_GAP_EVT_FUNC_CB)(TAL_BLE_GAP_PARAMS_EVT_T *p_event);

/**< GATT Callback Register function definition */
typedef void(*TAL_BLE_GATT_EVT_FUNC_CB)(TAL_BLE_GATT_PARAMS_EVT_T *p_event);

#endif


/* ---------------------------------------------------------------------------
 * BLE 平台 ops 卡：port 组装（linux 为 port_bt 适配层；无 BLE 能力的平台
 * 全成员 NULL）。成员签名用上面的镜像类型；与 port 的 port_bluetooth.h
 * 27 个接口一一对应。
 * --------------------------------------------------------------------------- */
typedef struct {
    /* stack */
    OPERATE_RET (*stack_init)(uint8_t role);
    OPERATE_RET (*stack_deinit)(uint8_t role);
    OPERATE_RET (*stack_gatt_link)(uint16_t *p_link);
    /* 事件注册（回调参数为镜像事件类型；注册须先于 stack_init） */
    OPERATE_RET (*gap_callback_register)(const TAL_BLE_GAP_EVT_FUNC_CB gap_evt);
    OPERATE_RET (*gatt_callback_register)(const TAL_BLE_GATT_EVT_FUNC_CB gatt_evt);
    /* GAP */
    OPERATE_RET (*gap_addr_set)(TAL_BLE_GAP_ADDR_T const *p_peer_addr);
    OPERATE_RET (*gap_address_get)(TAL_BLE_GAP_ADDR_T *p_peer_addr);
    OPERATE_RET (*gap_adv_start)(TAL_BLE_GAP_ADV_PARAMS_T const *p_adv_params);
    OPERATE_RET (*gap_adv_stop)(void);
    OPERATE_RET (*gap_adv_rsp_data_set)(TAL_BLE_DATA_T const *p_adv, TAL_BLE_DATA_T const *p_scan_rsp);
    OPERATE_RET (*gap_adv_rsp_data_update)(TAL_BLE_DATA_T const *p_adv, TAL_BLE_DATA_T const *p_scan_rsp);
    OPERATE_RET (*gap_scan_start)(TAL_BLE_GAP_SCAN_PARAMS_T const *p_scan_params);
    OPERATE_RET (*gap_scan_stop)(void);
    OPERATE_RET (*gap_connect)(TAL_BLE_GAP_ADDR_T const *p_peer_addr, TAL_BLE_GAP_SCAN_PARAMS_T const *p_scan_params,
                               TAL_BLE_GAP_CONN_PARAMS_T const *p_conn_params);
    OPERATE_RET (*gap_disconnect)(uint16_t conn_handle, uint8_t hci_reason);
    OPERATE_RET (*gap_conn_param_update)(uint16_t conn_handle, TAL_BLE_GAP_CONN_PARAMS_T const *p_conn_params);
    OPERATE_RET (*gap_rssi_get)(uint16_t conn_handle);
    /* GATT server */
    OPERATE_RET (*gatts_service_add)(TAL_BLE_GATTS_PARAMS_T *p_service); /* 非 const：port 回写 handle */
    OPERATE_RET (*gatts_value_set)(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length);
    OPERATE_RET (*gatts_value_notify)(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data, uint16_t length);
    OPERATE_RET (*gatts_exchange_mtu_reply)(uint16_t conn_handle, uint16_t server_rx_mtu);
    /* GATT client */
    OPERATE_RET (*gattc_all_service_discovery)(uint16_t conn_handle);
    OPERATE_RET (*gattc_all_char_discovery)(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle);
    OPERATE_RET (*gattc_char_desc_discovery)(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle);
    OPERATE_RET (*gattc_write_without_rsp)(uint16_t conn_handle, uint16_t char_handle, uint8_t *p_data,
                                           uint16_t length);
    OPERATE_RET (*gattc_read)(uint16_t conn_handle, uint16_t char_handle);
    OPERATE_RET (*gattc_exchange_mtu_request)(uint16_t conn_handle, uint16_t client_rx_mtu);
} TAL_BLUETOOTH_OPS_T;

/**
 * @brief Get the platform ops of tal_bluetooth
 *
 * @return pointer to the platform ops, never NULL (defined in the port)
 */
TAL_BLUETOOTH_OPS_T *tal_bluetooth_get_ops(void);

#endif /* __TAL_BLUETOOTH_OPS_H__ */
