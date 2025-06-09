#include "isotp_conn.h"

#include <zephyr/canbus/isotp.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/slist.h>

/**
 * @brief Dados do usuário anexados a cada buffer
 */
struct isotp_buf_user_data {
  uint32_t addr; // Endereço CAN
};

static int isotp_conn_lock(void);
static void isotp_conn_unlock(void);
static int isotp_conn_msg_lock(void);
static void isotp_conn_msg_unlock(void);

// Define o pool de buffers fora da classe
NET_BUF_POOL_DEFINE(isotp_tx_pool, ISOTP_NUM_BUFFERS, ISOTP_MAX_DATA_LEN,
                    sizeof(struct isotp_buf_user_data), NULL);

static const struct device *m_can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const struct isotp_fc_opts m_fc_opts = {.bs = 8, .stmin = 1};
static struct isotp_recv_ctx m_rctx;
static struct isotp_send_ctx m_sctx;
static sys_slist_t m_tx_queue;

K_SEM_DEFINE(isotp_conn_mutex, 0, 1);
K_SEM_DEFINE(isotp_conn_msg_mutex, 0, 1);

static int isotp_conn_lock(void) {
  return k_sem_take(&isotp_conn_mutex, K_MSEC(0));
}

static void isotp_conn_unlock(void) { return k_sem_give(&isotp_conn_mutex); }

static int isotp_conn_msg_lock(void) {
  return k_sem_take(&isotp_conn_msg_mutex, K_MSEC(0));
}

static void isotp_conn_msg_unlock(void) {
  return k_sem_give(&isotp_conn_msg_mutex);
}

void isotp_conn_tx_callback(int error_nr, void *arg) { isotp_conn_unlock(); }

int isotp_conn_init(void) {
  int ret;

  if (!device_is_ready(m_can_dev)) {
    printk("CAN: Dispositivo não está pronto.\n");
    return -EIO;
  }

  sys_slist_init(&m_tx_queue);

  const can_mode_t mode =
      (IS_ENABLED(CONFIG_SAMPLE_LOOPBACK_MODE) ? CAN_MODE_LOOPBACK : 0) |
      (IS_ENABLED(CONFIG_SAMPLE_CAN_FD_MODE) ? CAN_MODE_FD : 0);

  ret = can_set_mode(m_can_dev, mode);
  if (ret != 0) {
    printk("CAN: Falha ao configurar o modo [%2X]: %d\n", mode, ret);
    return ret;
  }

  ret = can_start(m_can_dev);
  if (ret != 0) {
    printk("CAN: Falha ao iniciar dispositivo: %d\n", ret);
    return ret;
  }

  printk("CAN inicializada\n");
  return 0;
}

int isotp_conn_bind(uint32_t rx_addr, uint32_t tx_addr) {
  int ret;
  struct isotp_msg_id isotp_rx;
  struct isotp_msg_id isotp_tx;

  memset(&isotp_rx, 0x0, sizeof(isotp_rx));
  memset(&isotp_tx, 0x0, sizeof(isotp_rx));

  isotp_rx.std_id = rx_addr;
  isotp_tx.std_id = tx_addr;

  printk("Registrando enderecos isotp\n");
  printk("RX: 0x%X - TX: 0x%X\n", isotp_rx.std_id, isotp_tx.std_id);

  isotp_conn_unlock();
  isotp_conn_msg_unlock();
  ret = isotp_bind(&m_rctx, m_can_dev, &isotp_rx, &isotp_tx, &m_fc_opts,
                   K_FOREVER);
  if (ret != ISOTP_N_OK) {
    printk("Erro ao configurar endereco isotp: %d\n", ret);
    return ret;
  }

  return 0;
}

void isotp_conn_unbind(void) {
  struct net_buf *buf;
  while ((buf = (struct net_buf *)sys_slist_get(&m_tx_queue)) != NULL) {
    net_buf_unref(buf);
  }
  isotp_unbind(&m_rctx);
}

int isotp_conn_transmit(uint32_t addr, const uint8_t *data, size_t len) {
  int ret;
  struct isotp_msg_id dst_addr;
  struct isotp_msg_id fc_addr;

  if (len > ISOTP_MAX_DATA_LEN) {
    printk("Erro: Tamanho de dados excede o máximo (%d > %d)\n", len,
           ISOTP_MAX_DATA_LEN);
    return -EINVAL;
  }

  ret = isotp_conn_lock();
  if (ret) {
    return ret;
  }

  memset(&dst_addr, 0x0, sizeof(dst_addr));
  memset(&fc_addr, 0x0, sizeof(fc_addr));

  dst_addr.std_id = addr;
  fc_addr.std_id = addr - 0x100;

  ret = isotp_send(&m_sctx, m_can_dev, data, len, &dst_addr, &fc_addr,
                   isotp_conn_tx_callback, NULL);

  return ret;
}

int isotp_conn_receive(uint8_t data[], size_t max_len) {
  int ret;
  struct net_buf *buf = NULL;
  size_t received_len = 0;
  int rem_len;

  ret = isotp_conn_lock();
  if (ret) {
    return ret;
  }

  do {
    rem_len = isotp_recv_net(&m_rctx, &buf, K_MSEC(0));
    if (rem_len < 0) {
      break;
    }

    while (buf != NULL) {
      if ((received_len + buf->len) > max_len) {
        printk("Erro: Buffer de recepção muito pequeno\n");
        while (buf != NULL) {
          net_buf_frag_del(NULL, buf);
        }

        isotp_conn_unlock();
        return 0;
      }

      memcpy(data + received_len, buf->data, buf->len);
      received_len += buf->len;
      buf = net_buf_frag_del(NULL, buf);
    }
  } while (rem_len);

  isotp_conn_unlock();

  return received_len;
}

int isotp_conn_add_message(uint32_t addr, const uint8_t *data, size_t len) {
  int ret;
  struct net_buf *buf;
  struct net_buf *old_buf;

  if (len > ISOTP_MAX_DATA_LEN) {
    printk("Erro: Tamanho de dados excede o máximo (%d > %d)\n", len,
           ISOTP_MAX_DATA_LEN);
    return -EINVAL;
  }

  ret = isotp_conn_msg_lock();
  if (ret) {
    return ret;
  }

  // Tenta alocar um novo buffer
  buf = net_buf_alloc(&isotp_tx_pool, K_NO_WAIT);

  // Se não há buffers disponíveis, remove o mais antigo
  if (!buf) {
    printk("Pool cheio, removendo mensagem mais antiga\n");
    old_buf = (struct net_buf *)sys_slist_get(&m_tx_queue);
    if (old_buf) {
      net_buf_unref(old_buf);
      // Tenta alocar novamente após liberar um buffer
      buf = net_buf_alloc(&isotp_tx_pool, K_NO_WAIT);
    }
  }

  // Se ainda não conseguiu um buffer, falha
  if (!buf) {
    printk("Erro: Falha crítica ao alocar buffer\n");
    isotp_conn_msg_unlock();
    return -EIO;
  }

  // Copia os dados e configura o endereço
  net_buf_add_mem(buf, data, len);
  struct isotp_buf_user_data *user_data = net_buf_user_data(buf);
  user_data->addr = addr;

  // Adiciona à fila de transmissão
  sys_slist_append(&m_tx_queue, &buf->node);

  isotp_conn_msg_unlock();

  return 0;
}

int isotp_conn_process_send(void) {
  int ret;
  struct isotp_msg_id dst_addr;
  struct isotp_msg_id fc_addr;
  struct isotp_buf_user_data *user_data;
  struct net_buf *buf;

  while (1) {

    ret = isotp_conn_lock();
    if (ret) {
      return ret;
    }

    if (sys_slist_is_empty(&m_tx_queue)) {
      isotp_conn_unlock();
      break;
    }

    buf = (struct net_buf *)sys_slist_get(&m_tx_queue);
    if (!buf) {
      isotp_conn_unlock();
      break;
    }

    user_data = net_buf_user_data(buf);
    memset(&dst_addr, 0x0, sizeof(dst_addr));
    memset(&fc_addr, 0x0, sizeof(fc_addr));

    dst_addr.std_id = user_data->addr;
    fc_addr.std_id = user_data->addr - 0x100;

    ret = isotp_send(&m_sctx, m_can_dev, buf->data, buf->len, &dst_addr,
                     &fc_addr, isotp_conn_tx_callback, NULL);
    if (ret != ISOTP_N_OK) {
      printk("Erro ao enviar mensagem para 0x%X [%d]\n", dst_addr.std_id, ret);
      // Em caso de erro, volta mensagem para fila
      // sys_slist_append(&m_tx_queue, &buf->node);
      // break;
    }

    net_buf_unref(buf);
  }

  return ret;
}
