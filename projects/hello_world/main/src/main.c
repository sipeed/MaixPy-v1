/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <bsp.h>
#include <sysctl.h>
#include "FreeRTOS.h"
#include "task.h"

int core1_function(void *ctx)
{
    uint64_t core = current_coreid();
    printf("Core %ld Hello world\n", core);
    vTaskStartScheduler();
    while(1);
}

void test(void* arg)
{
    int count = 0;
    while(1)
    {
        printf("---1---\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        if(count++ == 10)
        {
            printf("----3---\r\n");
            vTaskDelete(NULL);
            printf("----4---\r\n");
        }
    }
    printf("----5---\r\n");
}
void test2(void* arg)
{
    while(1)
    {
        printf("---2---\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    sysctl_pll_set_freq(SYSCTL_PLL0, 800000000);
    uint64_t core = current_coreid();
    int data;
    printf("Core %ld Hello world\n", core);
    register_core1(core1_function, NULL);

    xTaskCreateAtProcessor(0, test, "1", 256, NULL, tskIDLE_PRIORITY+1, NULL );
    xTaskCreateAtProcessor(1, test2, "2", 256, NULL, tskIDLE_PRIORITY+1, NULL );
    printf("start sheduler\r\n");
    vTaskStartScheduler();
    printf("start sheduler end\r\n");
    while(1)
        continue;
    return 0;
}

