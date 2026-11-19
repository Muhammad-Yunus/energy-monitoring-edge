#include "door_sensor.h"
#include "http_client.h"
#include <stdio.h>
#include <string.h>

volatile uint8_t door_state_changed = 0;
static uint8_t   door_state = 0;

void DoorSensor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    GPIO_InitStruct.Pin  = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Clear any pending EXTI flag before enabling NVIC */
    EXTI->PR = GPIO_PIN_15;

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    door_state = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_RESET) ? 0 : 1;

    printf("[door] init, state=%s\n", door_state ? "closed" : "open");
}

void DoorSensor_ExtiHandler(void)
{
    door_state_changed = 1;
    printf("[door] EXTI trigger, state=%s\n",
        HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_SET ? "open" : "closed");
}

int  DoorSensor_IsChanged(void) { return door_state_changed; }
void DoorSensor_ClearFlag(void) { door_state_changed = 0; }

uint8_t DoorSensor_GetState(void)
{
    /* PC15: HIGH=open, LOW=closed */
    return (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_SET) ? 1 : 0;
}

int DoorSensor_Post(uint8_t state)
{
    char body[128];
    snprintf(body, sizeof(body),
        "{\"status\":200,\"data\":{\"event\":\"door\",\"state\":\"%s\"}}",
        state ? "open" : "closed");

    int rc = HTTP_Post(API_HOST_IP, API_PORT, body);
    if (rc != 0) {
        printf("[door] POST failed rc=%d state=%s\n", rc, state ? "open" : "closed");
    } else {
        printf("[door] POST ok state=%s\n", state ? "open" : "closed");
    }
    return rc;
}
