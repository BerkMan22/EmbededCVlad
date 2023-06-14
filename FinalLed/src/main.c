#include "main.h"


//bool is_valid(const char *str);
esp_err_t save_led_config(void);
esp_err_t get_led_config(void);
void after_restart(void);
void boot_default(void);

int freq,duty=1;

//functia pentru a configura ledul pentru freq si duty
static void config_led(void){
    // Configurarea timerului LED
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz =freq, //Hz
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);

    // Configurarea canalului LED
    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = LED_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);

    printf("SE CONFIGUREAZA LED-UL PENTRU %d HZ SI %d%% DUTY\n",freq,duty);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, (LED_MAX_RESOLUTION*duty)/100);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

}

//functie protocol

static void protocol(char *data){
    char* token = strtok(data,",");
    freq = atoi(token);
    token = strtok(NULL,",");
    duty = atoi(token);
}

//functie pentru a citi configuratia ledului prin UART
static void read_serial_cmd(){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(UART_NUM_0, BUF_SIZE *2 , 0, 0, NULL, 0);

    char data[BUF_SIZE];
    int data_index=0;

    while (1)
    {
        int len=uart_read_bytes(UART_NUM_0,(uint8_t*)&data+data_index,BUF_SIZE-data_index,20/ portTICK_PERIOD_MS);
        if (len > 0) {
            data_index += len;
            if (data[data_index - 1] == '\n' || data[data_index - 1] == '\r') {
                data[data_index] = 0;
                printf("RECEIVED STRING: %s", data);
                data_index = 0;

                //protocolul prin care se transmit datele este de tipul frecventa,numarul la care se imparte duty-ul
                protocol(data);

                printf("SE SALVEAZA CONFIGURATIA LEDULUI IN MEMORIE\n");
                esp_err_t err = nvs_flash_init();
                if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                ESP_ERROR_CHECK(nvs_flash_erase());
                err = nvs_flash_init();
                }
                ESP_ERROR_CHECK(err);
                err = save_led_config();
                if (err != ESP_OK) printf("EROARE (%s) LA SALVAREA CONFIGURATIEI LED PE FLASH!\n", esp_err_to_name(err));

                printf("RESTARTEAZA ESP32 PENTRU VERIFICARE FLASH\n");
                esp_restart();
                }
        }
    }   
}

//functie pentru a salva in falsh configuratia ledului
esp_err_t save_led_config(void){
    nvs_handle_t my_handle;
    esp_err_t err;

     // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    err = nvs_set_i32(my_handle, "freq", freq);
    if (err != ESP_OK) return err;

    err = nvs_set_i32(my_handle, "duty", duty);
    if (err != ESP_OK) return err;

    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    printf("CONFIGURATIA LED-ULUI A FOST SALVATA IN FLASH\n");

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}


//functie pentru a sterge din falsh configuratia ledului
esp_err_t delete_led_config(void) {
    nvs_handle_t my_handle;
    esp_err_t err;

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    err = nvs_erase_key(my_handle, "freq");
    if (err != ESP_OK) return err;

    err = nvs_erase_key(my_handle, "duty");
    if (err != ESP_OK) return err;

    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    printf("CONFIGURATIA LEDULUI A FOST STEARSA DE PE FLASH.\n");

    nvs_close(my_handle);

    return ESP_OK;
}

//functie pe pentru a obtine din falsh configuratia ledului
esp_err_t get_led_config(void){
    nvs_handle_t my_handle;
    esp_err_t err;

     // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    err = nvs_get_i32(my_handle, "freq", &freq);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    err = nvs_get_i32(my_handle, "duty", &duty);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    printf("DATE GASITE IN FLASH:%d HZ SI %d%% DUTY\n",freq,duty);

    nvs_close(my_handle);
    return ESP_OK;
}

//dupa ce dispozitivul se restarteaza
void after_restart(void){
     esp_err_t err = nvs_flash_init();
     if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
        }
    ESP_ERROR_CHECK(err);

    err = get_led_config(); //se cauta in memorie configuratia ledului
    if(err==ESP_OK) config_led(); //daca exista in memorie o configuratie salvata se configureaza ledul pentru datele din memorie

}


//Cand se apasa pe butonul boot
void boot_default(void){
    while (1)
    {
        if (gpio_get_level(BOOT_BUTTON_PIN) == 0)
        {
            freq = 1; //frecventa devine de 1 HZ si duty devine 50%
            duty = 50;
            delete_led_config(); //se sterge configuratia curenta a ledului de pe flash
            config_led(); //se configureaza ledul pentru freq si duty
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    
}



void app_main(void) {
    esp_rom_gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    //De fiecare data cand se restarteaza esp aceasta functie va fi apelata pentru a initializa ledul cu date pe flash daca sunt
    after_restart();

    //task-ul care se ocupa cu primirea configuratiei prin seriala
    xTaskCreate(read_serial_cmd,"read_serial_cmd",4096,NULL,10,NULL);

    //task-ul care se ocupa cu verificare apasarilor butonului boot
    xTaskCreate(boot_default,"boot_default",4096,NULL,10,NULL);
   

}