/*

   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <driver/adc.h>
#include "esp_timer.h"
#include <string.h>

#include "../../esp32-ttgo-display/main/ttgo.h"

#define UART_RX1 27
#define UART_RX2 26

#define UART_TX1 25
#define UART_TX2 37

extern uint8_t bgRed, bgGreen, bgBlue;
unsigned test = 0;
unsigned refreshNow = 1;

typedef struct AirData_t
{
    unsigned pm1;
    unsigned pm2_5;
    unsigned pm10;
    unsigned count_3;
    unsigned count_5;
    unsigned count1;
    unsigned count2_5;
    unsigned count5;
    unsigned count10;
    int animationCnt;
    int aqi;
    int pinNo;
    int sensorNumber;
    uart_port_t uart_num;
    int64_t sampleTime;
    int64_t oldSampleTime;
} AirData;

AirData airData1, airData2;

int displayMode = 0;

//The driver samples the SDA (input data) at rising edge of SCL,
//but shifts SDA (output data) at the falling edge of SCL

//After the read status command has been sent, the SDA line must be set to tri-state no later than at the
//falling edge of SCL of the last bit.

unsigned calcAqi(unsigned pm25)
{

    int interval_y0;
    int interval_y1;
    int interval_x0;
    int interval_x1;

    if (pm25 <= 12)
    {
        interval_y0 = 0;
        interval_y1 = 50;
        interval_x0 = 0;
        interval_x1 = 12;
    }
    else if (pm25 <= 35)
    {
        interval_y0 = 51;
        interval_y1 = 100;
        interval_x0 = 13;
        interval_x1 = 35;
    }
    else if (pm25 <= 55)
    {
        interval_y0 = 101;
        interval_y1 = 150;
        interval_x0 = 35;
        interval_x1 = 55;
    }
    else if (pm25 <= 150)
    {
        interval_y0 = 151;
        interval_y1 = 200;
        interval_x0 = 55;
        interval_x1 = 150;
    }
    else if (pm25 <= 250)
    {
        interval_y0 = 201;
        interval_y1 = 300;
        interval_x0 = 150;
        interval_x1 = 250;
    }
    else if (pm25 <= 350)
    {
        interval_y0 = 301;
        interval_y1 = 400;
        interval_x0 = 250;
        interval_x1 = 350;
    }
    else if (pm25 <= 500)
    {
        interval_y0 = 401;
        interval_y1 = 500;
        interval_x0 = 350;
        interval_x1 = 500;
    }
    else
    {
        return pm25;
    }
    int retval = interval_y0 + ((pm25 - interval_x0) * (interval_y1 - interval_y0) / (interval_x1 - interval_x0));

    return retval + test;
}

void test1()
{

    //TTGO Logo is top
    char s[30];
    char s1[30];
    char s2[30];
    int count = 0;
    int oldColorNumber = -1;

    airData1.animationCnt = 0;
    airData2.animationCnt = 0;
    airData1.oldSampleTime = 0;
    airData2.oldSampleTime = 0;

    int64_t nextRefresh = 0;

    //if (displayMode == 0)
    //{
    //    bgRed = 0x00;
    //    bgGreen = 0x00;
    //    bgBlue = 0x00;
    //    clearScreen(bgRed, bgGreen, bgBlue);
    //}

    while (1)
    {
        unsigned aqi = 33;
        int x,y;
        if (displayMode == 0)
        {

            int64_t currentTime = esp_timer_get_time();
            if (currentTime > nextRefresh)
            {
                printf("currentTime %lli\n", currentTime);
                nextRefresh = currentTime + 5000000;

                aqi = (airData1.aqi + airData2.aqi) / 2;

                unsigned colorNumber;

                if (aqi < 51)
                {
                    colorNumber = 0; //green
                    bgRed = 0x00;
                    bgGreen = 0xcf;
                    bgBlue = 0x00;
                }
                else if (aqi < 101)
                {
                    colorNumber = 1; //yellow
                    bgRed = 0xdf;
                    bgGreen = 0xdf;
                    bgBlue = 0x00;
                }
                else if (aqi < 151)
                {
                    colorNumber = 2; // Orange;
                    bgRed = 0xdf;
                    bgGreen = 0xb0;
                    bgBlue = 0x00;
                }
                else if (aqi < 201)
                {
                    colorNumber = 3; // Red;
                    bgRed = 0xef;
                    bgGreen = 0x00;
                    bgBlue = 0x00;
                }
                else if (aqi < 301)
                {
                    colorNumber = 4; // purple;
                    bgRed = 0xd0;
                    bgGreen = 0x00;
                    bgBlue = 0xd0;
                }
                else
                {
                    colorNumber = 5; // Maroon;
                    bgRed = 0xa0;
                    bgGreen = 0x00;
                    bgBlue = 0x30;
                }

                if ((oldColorNumber != colorNumber) || refreshNow)
                {
                    clearScreen(bgRed, bgGreen, bgBlue);
                    refreshNow = 0;
                }
                oldColorNumber = colorNumber;

                int x = 40;
                // int y = 10;
                const int yMid = 135 / 2;
                count++;

                snprintf(s, 30, "AQI");
                displayStr(s, x, yMid - 16, 0xf0, 0xf0, 0xf0, 32);
                snprintf(s, 30, "%d   ", aqi);
                displayStr(s, x + 60, yMid - 28, 0xf0, 0xf0, 0xf0, 64);

                snprintf(s, 30, "S1:");
                displayStr(s, 10, 135 - 32, 0, 0, 0, 32);

                snprintf(s, 30, "S2:");
                displayStr(s, 120, 135 - 32, 0, 0, 0, 32);

                //const int v = 3818 + (adc1_get_raw(ADC1_CHANNEL_6) * 1000 - 2080 * 1000) / 625;
                //snprintf(s, 30, "%d.%d V  ", v / 1000, ((v % 1000) + 50) / 100);
                //displayStr(s, 10, 135 - 32, 0xf0, 0xf0, 0xf0, 32);
            }

            if (airData1.animationCnt == 0)
            {
                snprintf(s1, 30, "%d    ", airData1.aqi);
                //displayStr(s, 10, 135 - 32, 0xf0, 0xf0, 0xf0, 32);
            }
            if (airData2.animationCnt == 0)
            {
                snprintf(s2, 30, "%d    ", airData2.aqi);
                //displayStr(s, 80, 135 - 32, 0xf0, 0xf0, 0xf0, 32);
            }

            if (airData1.animationCnt < 10)
            {

                const u_int8_t color = 0xff - airData1.animationCnt * 0x0f;
                //fillBox(210, 15, 14, 14, color, color, color);
                displayStr(s1, 60, 135 - 32, color, color, color, 32);
                airData1.animationCnt++;
            }

            if (airData2.animationCnt < 10)
            {

                const u_int8_t color = 0xff - airData2.animationCnt * 0x0f;
                //fillBox(210, 34, 14, 14, color, color, color);
                displayStr(s2, 180, 135 - 32, color, color, color, 32);
                airData2.animationCnt++;
            }

            airData1.oldSampleTime = airData1.sampleTime;
        } // displaymode == 1

        else if (displayMode == 1 || displayMode == 2)
        {
            AirData *ad;
            if (displayMode == 1) ad = &airData1;
            else ad = &airData2;

            if (refreshNow)
            {
                clearScreen(bgRed, bgGreen, bgBlue);
                refreshNow = 0;

                int x = 5;
                int y = 5;

                snprintf(s, 30, "Sensor %d PM Values",  ad->sensorNumber);
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                y = y + 32;
                snprintf(s, 30, "PM1 um:");
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                y = y + 32;
                snprintf(s, 30, "PM2.5 um:");
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                y = y + 32;
                snprintf(s, 30, "PM10 um:");
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                y = y + 32;

            }
            int x = 140;
            int y = 5+32;

            snprintf(s, 30, "%d  ", ad->pm1);
            displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;
            snprintf(s, 30, "%d  ", ad->pm2_5);
            displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;
            snprintf(s, 30, "%d  ", ad->pm10);
            displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;


        }
        else if (displayMode == 3 || displayMode == 4)
        {
            AirData *ad;
            if (displayMode == 1) ad = &airData1;
            else ad = &airData2;

            if (refreshNow)
            {
                clearScreen(bgRed, bgGreen, bgBlue);
                refreshNow = 0;

                int x = 5;
                int y = 5;

                snprintf(s, 30, "Sensor %d Prtcl Cnts", ad->sensorNumber);
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                y = y + 32;
                
                snprintf(s, 30, ".3");
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                y = y + 32;
                snprintf(s, 30, ".5");
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                y = y + 32;
                snprintf(s, 30, "1");
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                //y = y + 32;

                x = 120;
                y = 5 + 32;
                
                snprintf(s, 30, "2.5");
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                y = y + 32;
                snprintf(s, 30, "5");
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                y = y + 32;
                snprintf(s, 30, "10");
                displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
                y = y + 32;

            }

            x = 40;
            y = 5+32;

            snprintf(s, 30, "%d     ", ad->count_3);
            displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;
            snprintf(s, 30, "%d     ", ad->count_5);
            displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;
            snprintf(s, 30, "%d     ", ad->count1);
            displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;

            x = 120+50;
            y = 5+32;
            snprintf(s, 30, "%d   ", ad->count2_5);
            displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;
            snprintf(s, 30, "%d   ", ad->count5);
            displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;
            snprintf(s, 30, "%d   ", ad->count10);
            displayStr(s, x, y, 0xff, 0xff, 0xff, 32);

        }
        else
        {

            uint8_t random;
            random = (uint8_t)esp_random();
            bgRed = random;
            bgGreen = random;
            bgBlue = random;
            clearScreen(random, random, random >> 2);
            vTaskDelay(50 / portTICK_PERIOD_MS);

            //wrCmmd(ST7789_MADCTL);
            //wrData(0b01101000);

            int x = 20;
            int y = 20;

            snprintf(s, 30, "This ");
            x = displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;
            snprintf(s, 30, "is a ");
            x = displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;
            snprintf(s, 30, "Test ");
            x = displayStr(s, x, y, 0xff, 0xff, 0xff, 32);
            y = y + 32;
        }

        //vTaskDelay(1000 / portTICK_PERIOD_MS);
        //count++;

        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_set_intr_type(35, GPIO_PIN_INTR_POSEDGE);

    } // end of while(1)
}

void gpio_int(void *arg)
{
    gpio_set_intr_type(35, GPIO_PIN_INTR_DISABLE);
    //test = (test + 50) % 500;
    displayMode = (displayMode + 1) % 5;
    refreshNow = 1;
}

static const int RX_BUF_SIZE = 1024;

void init(void)
{

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); //channel 6 is gpio 34

    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    airData1.pinNo = UART_RX1;
    airData1.uart_num = UART_NUM_1;
    airData1.sensorNumber = 1;
    airData2.pinNo = UART_RX2;
    airData2.uart_num = UART_NUM_2;
    airData2.sensorNumber = 2;

    // We won't use a buffer for sending data.
    uart_driver_install(airData1.uart_num, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(airData1.uart_num, &uart_config);
    uart_set_pin(airData1.uart_num, UART_PIN_NO_CHANGE, airData1.pinNo, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(airData2.uart_num, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(airData2.uart_num, &uart_config);
    uart_set_pin(airData2.uart_num, UART_TX1, airData2.pinNo, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    gpio_set_direction(35, GPIO_MODE_INPUT);
    gpio_set_intr_type(35, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(35, gpio_int, NULL);
}

static void rxTasks(AirData *airData)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1)
    {
        const int rxBytes = uart_read_bytes(airData->uart_num, data, 32, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0)
        {
            data[rxBytes] = 0;
            //ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            //ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
            //printf("%d\n", airData->uart_num);

            if ((data[0] == 'B') &&
                (data[1] == 'M') &&
                (rxBytes == 32) &&
                (data[2] == (uint8_t)0) &&
                (data[3] == (uint8_t)28))
            {

                airData->pm1 = (((unsigned)data[4]) << 8) + (unsigned)data[5];
                airData->pm2_5 = (((unsigned)data[6]) << 8) + (unsigned)data[7];
                airData->pm10 = (((unsigned)data[8]) << 8) + (unsigned)data[9];
                airData->count_3 = (((unsigned)data[16]) << 8) + (unsigned)data[17];
                airData->count_5 = (((unsigned)data[18]) << 8) + (unsigned)data[19];
                airData->count1  = (((unsigned)data[20]) << 8) + (unsigned)data[21];

                //airData.count_3 =
                airData->sampleTime = esp_timer_get_time();
                airData->animationCnt = 0;
                airData->aqi = calcAqi(airData->pm2_5);
            }

            //printf();
            // ESP_LOGI(RX_TASK_TAG, "pm2.5 = %d .3um Count = %d ts=%lld", airData.pm2_5,
            //          airData.count_3, airData.sampleTime);
        }

        //else printf("no data\n");
    }
}

static void rx1_task(void *arg)
{
    rxTasks(&airData1);
}

static void rx2_task(void *arg)
{
    rxTasks(&airData2);
}



void app_main(void)
{

    initTTGO();
    init();
    xTaskCreate(rx1_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(rx2_task, "uart_rx_task2", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
    //xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);

    test1();
}
