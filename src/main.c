#include "mqtt_client.h"

int main() {
    // Inicializar el cliente MQTT y comenzar el bucle de mensajes
    mqtt_init();
    mqtt_loop();
    
    return 0;
}