typedef struct rcp_cmd {
  uint8_t servo_index;
  uint8_t servo_state;  // 0 is off 1 is on
} rcp_cmd_t;

typedef void (*rcp_recv_cb_t)(const rcp_cmd_t *cmd);

void rcp_master_init(const uint8_t *peer_addr);

void rcp_slave_init(const uint8_t *peer_addr, rcp_recv_cb_t recv_cb);

void rcp_send(const rcp_cmd_t *cmd);
