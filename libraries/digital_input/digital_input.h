/**
 * @file digital_input.h
 */

 #ifndef DIGITAL_INPUT_H_
 #define DIGITAL_INPUT_H_
 
 #include <stdbool.h>
 #include <stdint.h>
 #include <zephyr/kernel.h>
 #include <zephyr/drivers/gpio.h>
 #include <zephyr/input/input.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define DIGITAL_INPUT_MAX_COUNT 32
 #define DIGITAL_INPUT_EVENT_QUEUE_SIZE 10  // Tamanho padrão da fila de eventos
 
 typedef struct {
     int id;                             /* Input identifier */
     const struct gpio_dt_spec *gpio;    /* GPIO specification */
     bool active_high;                   /* true if input activates on high signal, false for low */
     const char *description;            /* Optional description */
 } digital_input_config_t;
 
 // Dados passados para callbacks externas
 typedef struct {
     int input_id;                      /* Input identifier */
     bool state;                        /* Current state (true=active, false=inactive) */
     uint32_t timestamp;                /* Timestamp of the event */
     uint32_t status_mask;              /* Current input status mask (all inputs) */
     const digital_input_config_t *config; /* Configuration of the input */
 } digital_input_event_t;
 
 // Tipo de função para callback de eventos
 typedef void(*digital_input_event_cb_t)(const digital_input_event_t *event);
 
 typedef struct {
     int id;                       /* Input ID */
     bool state;                   /* State to be set after hysteresis */
     void *input;                  /* Pointer to the input structure */
 } digital_input_timer_data_t;
 
 typedef struct {
     uint32_t hysteresis_ms;       /* Hysteresis time in milliseconds */
     digital_input_timer_data_t pending; /* Pending state waiting for confirmation */
     struct k_timer timer;         /* Timer for hysteresis */
 } digital_input_hysteresis_t;
 
 typedef struct {
     uint32_t status_mask;                         /* Current status mask (32 bits) */
     uint32_t status_reported_mask;                /* Last reported status mask (for callback) */
     digital_input_hysteresis_t hysteresis[DIGITAL_INPUT_MAX_COUNT]; /* Hysteresis settings for each input */
     struct k_mutex mutex;                         /* Mutex for controlling access */
     
     // Work queue e fila de mensagens
     struct k_work input_change_work;              /* Work para processamento de eventos */
     struct k_msgq event_queue;                    /* Fila de mensagens para eventos */
     char __aligned(4) event_msgq_buffer[DIGITAL_INPUT_EVENT_QUEUE_SIZE * sizeof(digital_input_event_t)];
     
     // Callback para código de aplicação
     digital_input_event_cb_t event_callback;      /* Callback para eventos de entrada digital */
     
     const digital_input_config_t *config_list;    /* List of input configurations */
     uint16_t config_count;                        /* Number of items in config list */
 } digital_input_t;
 
 /**
  * @brief Initialize the digital input system with the given configuration
  * 
  * @param input Pointer to digital_input_t structure
  * @param config_list Array of configurations for each input
  * @param config_count Number of configurations in the array
  * @param callback Optional callback function to be called when input state changes
  * @param queue_size Size of the event queue for input events
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_init(digital_input_t *input, const digital_input_config_t *config_list, 
                      uint16_t config_count, digital_input_event_cb_t callback,
                      uint16_t queue_size);
 
 /**
  * @brief Initialize with default queue size
  */
 int digital_input_init_default(digital_input_t *input, const digital_input_config_t *config_list,
                              uint16_t config_count, digital_input_event_cb_t callback);
 
 /**
  * @brief Set hysteresis for a specific input
  * 
  * @param input Pointer to digital_input_t structure
  * @param id The ID of the input
  * @param hysteresis_ms Hysteresis time in milliseconds
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_set_hysteresis(digital_input_t *input, int id, uint32_t hysteresis_ms);
 
 /**
  * @brief Get the hysteresis setting for a specific input
  * 
  * @param input Pointer to digital_input_t structure
  * @param id The ID of the input
  * @param hysteresis_ms Pointer to store the hysteresis time
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_get_hysteresis(digital_input_t *input, int id, uint32_t *hysteresis_ms);
 
 /**
  * @brief Get the current status of a specific input
  * 
  * @param input Pointer to digital_input_t structure
  * @param id The ID of the input to query
  * @param status Pointer to store the current status
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_get_status(digital_input_t *input, int id, uint32_t *status);
 
 /**
  * @brief Update the status of a specific input
  * This function applies hysteresis if configured
  * 
  * @param input Pointer to digital_input_t structure
  * @param id The ID of the input to update
  * @param status The new status (0 or 1)
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_update_status(digital_input_t *input, int id, uint32_t status);
 
 /**
  * @brief Set the status of all inputs at once using a bitmask
  * This function bypasses hysteresis and does not trigger callbacks
  * 
  * @param input Pointer to digital_input_t structure
  * @param mask Bitmask representing the status of all inputs
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_set_all_state(digital_input_t *input, uint32_t mask);
 
 /**
  * @brief Get all input statuses as a bitmask
  * 
  * @param input Pointer to digital_input_t structure
  * @param mask Pointer to store the status bitmask
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_get_status_all(digital_input_t *input, uint32_t *mask);
 
 /**
  * @brief Display list of all configured inputs
  * 
  * @param input Pointer to digital_input_t structure
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_show_list(digital_input_t *input);
 
 /**
  * @brief Display all active inputs
  * 
  * @param input Pointer to digital_input_t structure
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_show_active(digital_input_t *input);
 
 /**
  * @brief Flush the event queue
  * 
  * @param input Pointer to digital_input_t structure
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_flush_event_queue(digital_input_t *input);
 
 /**
  * @brief Cleanup resources used by digital input system
  * 
  * @param input Pointer to digital_input_t structure
  * @return int 0 if successful, negative error code otherwise
  */
 int digital_input_deinit(digital_input_t *input);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* DIGITAL_INPUT_H_ */