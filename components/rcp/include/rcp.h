#ifndef __RCP_H__
#define __RCP_H__

#include "esp_now.h"

#define OP_NO (0)
#define OP_OFF (1)
#define OP_ON (2)

typedef struct rcp_cmd {
  uint8_t op0;
  uint8_t op1;
} rcp_cmd_t;

typedef void (*rcp_recv_cb_t)(const rcp_cmd_t *cmd);

typedef void (*rcp_send_cb_t)(esp_now_send_status_t status);

void rcp_master_init(const uint8_t *peer_addr, rcp_send_cb_t send_cb);

void rcp_slave_init(const uint8_t *peer_addr, rcp_recv_cb_t recv_cb);

void rcp_send(const rcp_cmd_t *cmd);

#endif /* __ESP_NOW_H__ */
