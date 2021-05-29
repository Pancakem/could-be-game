#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BUTTON_1 GPIO_NUM_15
#define BUTTON_2 GPIO_NUM_19
#define GPIO_INPUT_PIN_SEL ((1ULL<<BUTTON_1) | (1ULL<<BUTTON_2))
#define ESP_INTR_FLAG_DEFAULT 0

#define button_tag "BUTTON"

static xQueueHandle gpio_event_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t) arg;
  xQueueSendFromISR(gpio_event_queue, &gpio_num, NULL);
}

// redundant task for now
static void gpio_task_echo(void *arg) {
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_event_queue, &io_num, portMAX_DELAY)) {
	  ESP_LOGI(button_tag, "GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

void button_init() {
  gpio_config_t io_conf;

  // configure pin for button 1
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
  io_conf.pull_down_en = 0;
  // enable pull up
  io_conf.pull_up_en = 1;

  gpio_config(&io_conf);

  // configure pin for button 2
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
  io_conf.pull_down_en = 0;
  // enable pull up
  io_conf.pull_up_en = 1;
  
  gpio_config(&io_conf);

  // set up interrupts for both buttons
  gpio_set_intr_type(BUTTON_1, GPIO_INTR_ANYEDGE);
  gpio_set_intr_type(BUTTON_2, GPIO_INTR_ANYEDGE);

  gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));
  xTaskCreate(gpio_task_echo, "gpio_task_echo", 2048, NULL, 10, NULL);
  
  //install gpio isr service
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(BUTTON_1, gpio_isr_handler, (void*) BUTTON_1);
  //hook isr handler for specific gpio pin
  gpio_isr_handler_add(BUTTON_2, gpio_isr_handler, (void*) BUTTON_2); 
}
