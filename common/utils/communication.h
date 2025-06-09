/**
* @file    communication.h
* @brief
*/

//==============================================================================
// Define to prevent recursive inclusion
//==============================================================================
#ifndef __COMMUNICATION_H_
#define __COMMUNICATION_H_

/* C++ detection */
#ifdef __cplusplus
extern "C"
{
#endif

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
//Exported constants
//==============================================================================

#define COMM_INFO_SIZE_HOST_PARAM   (30)
#define COMM_LENGTH_ADDR_IPV4       (16)
#define COMM_LENGTH_MAC_ADDR        (17)
#define COMM_SIZE_SSID_BUFF         (32)
#define COMM_SIZE_PASSWORD_BUFF     (64)
#define COMM_SIZE_TOPIC_MQTT        (128)

//==============================================================================
// Exported macro
//==============================================================================

//==============================================================================
// Exported types
//==============================================================================

/** This is the aligned version of ip4_addr_t,
   used as local variable, on the stack, etc. */
union addr_ip4
{
  uint32_t addr;
  struct
  {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
  };
};

enum conn_protocol
{
  COMM_TCP = 0,
  COMM_UDP,
  COMM_SSL,
  COMM_MAX,
};

enum mqtt_qos
{
  QOS_0 = 0,
  QOS_1,
  QOS_2,
  QOS_MAX
};

struct network_access
{
  char ssid[ COMM_SIZE_SSID_BUFF ];
  char password[ COMM_SIZE_PASSWORD_BUFF ];
};

struct network
{
  char ip_address[ COMM_LENGTH_ADDR_IPV4 ];
  char netmask[ COMM_LENGTH_ADDR_IPV4 ];
  char gateway[ COMM_LENGTH_ADDR_IPV4 ];
  char dns[ COMM_LENGTH_ADDR_IPV4 ];
  bool dhcp;
};

struct conn_info
{
  struct network net;
  char mac[COMM_LENGTH_MAC_ADDR];
  bool is_connected;
};

/**
 * @brief
 */
struct conn_host_info_t
{
  enum conn_protocol protocol; /**< Protocol used to connect to Host. */
  char host[ COMM_INFO_SIZE_HOST_PARAM ]; /**< Host ip address that the client need connect. */
  int port; /**< Port of host to connect. */
  bool is_connected;
};

typedef struct
{
  char ssid[ COMM_SIZE_SSID_BUFF ];
  char password[ COMM_SIZE_PASSWORD_BUFF ];
  int channel_id;
  int rssi; /* Received signal strength indication */
  char mac[COMM_LENGTH_MAC_ADDR];
  uint8_t encryption_mode;
} wireless_network;

struct eth_network_config
{
  struct network net;
  bool enable;
};

struct wifi_network_config_t
{
  struct network_access access;
  struct network net;
  bool enable;
};

/*
 * Estrutura contendo os dados de conexao do broker MQTT
 */
struct comm_mqtt_config
{
  char client_id[64]; // Unique per device.
  char user_name[64];
  char password[64];
  char host_ip[64];
  int port;
  enum conn_protocol protocol; // //t -> tcp , s-> secure tcp, c-> secure tcp + certificates
  enum mqtt_qos qos;
  char pub_topic[COMM_SIZE_TOPIC_MQTT];
  char sub_topic[COMM_SIZE_TOPIC_MQTT];
  uint64_t id_device;
};

//==============================================================================
// Exported variables
//==============================================================================

//==============================================================================
// Exported functions prototypes
//==============================================================================

//==============================================================================
// Exported functions
//==============================================================================

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /*__COMMUNICATION_H_  */
