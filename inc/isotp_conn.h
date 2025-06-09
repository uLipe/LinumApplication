#ifndef ISOTP_CONN_H
#define ISOTP_CONN_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define ISOTP_MAX_DATA_LEN 128
#define ISOTP_NUM_BUFFERS 20

int isotp_conn_init(void);
int isotp_conn_bind(uint32_t rx_addr, uint32_t tx_addr);
void isotp_conn_unbind(void);
int isotp_conn_transmit(uint32_t addr, const uint8_t *data, size_t len);
int isotp_conn_add_message(uint32_t addr, const uint8_t *data, size_t len);
int isotp_conn_process_send(void);
int isotp_conn_receive(uint8_t data[], size_t max_len);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif // ISOTP_CONN_H
