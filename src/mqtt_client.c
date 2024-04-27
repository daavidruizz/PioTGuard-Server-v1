#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include "mqtt_client.h"
#include <string.h>

#define host "127.0.0.1"
#define port 8883

#define caPath "credentials/ca.crt"
#define certPath "credentials/sensor.crt"
#define certKeyPath "credentials/sensor.key"

struct mosquitto *mosq = NULL;

static void mqttCallback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message){
    if (message->payloadlen) {
        printf("Received message on topic '%s': %s\n", message->topic, (char *)message->payload);
    } else {
        printf("Received message on topic '%s' but no payload\n", message->topic);
    }
}

static int pw_callback(char *buf, int size, int rwflag, void *userdata) {
    // Suponiendo que la contraseña está almacenada de alguna manera accesible
    const char *password = "140516";  // Deberías tener una forma segura de manejar esto

    if (size < strlen(password) + 1) {
        return 0; // El buffer proporcionado no es suficiente
    }

    strcpy(buf, password);  // Copiar la contraseña al buffer proporcionado
    return strlen(buf);     // Retornar la longitud de la contraseña escrita en el buffer
}

void mqtt_init(void){
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if(mosq == NULL){
        fprintf(stderr, "Error: Out of memory.\n");
		return;
    }

    int rc = mosquitto_tls_set(mosq, caPath, "credentials/", certPath, certKeyPath, pw_callback);
    if(rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Failed to set TLS: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return;
    }
    mosquitto_tls_opts_set(mosq, 0, "tlsv1.3", NULL);
    mosquitto_username_pw_set(mosq, "app", "140516");

    mosquitto_connect(mosq, host, port, 60);

    mosquitto_message_callback_set(mosq, mqttCallback);

    mosquitto_subscribe(mosq, NULL, "/info/device/sensors", 2);
}

void mqtt_loop(void){
    mosquitto_loop_forever(mosq, -1, 1);
}

void mqtt_close(void){
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}