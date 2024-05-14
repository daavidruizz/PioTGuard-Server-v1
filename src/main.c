//gcc -o guard     src/main.c  -I./include     -I/usr/include/cjson     $(pkg-config --cflags --libs mariadb)     -lmosquitto     -lcjson
#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <mysql.h>
#include <string.h>
#include "mqtt_client.h"
#include "mariadb_client.h"
#include <cJSON.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

#define MQTTcaPath "/home/guard/certificates/mqtt/mqtt_ca.crt"
#define MQTTcertPath "/home/guard/certificates/mqtt/clients/localhost.crt"
#define MQTTcertKeyPath "/home/guard/certificates/mqtt/clients/localhost.key"

#define DBcaPath "/home/guard/certificates/mariadb/mariadb_ca.crt"
#define DBcertPath "/home/guard/certificates/mariadb/clients/localhost.crt"
#define DBcertKeyPath "/home/guard/certificates/mariadb/clients/localhost.key"

#define db_user_pass "140516"

#define COOLDOWN_PERIOD 180
static time_t last_execution = 0;

struct mosquitto *mosq = NULL;
MYSQL *conn;

void replace_spacesString(char* s, char replacement) {
    char* d = s;
    do {
        if (*d == ' ' || *d == ':') {
            *d = replacement;  // Reemplaza el espacio por el carácter 'replacement'
        }
    } while (*s++ = *d++);
}

void record_video(char *filename, char* duration) {
    // Construir el comando a ejecutar
    replace_spacesString(filename, '-');
    char command[512];
    snprintf(command, sizeof(command), "/bin/bash ./utils/record_video.sh %s %s &", filename, "50"); //150 aprox 5min
    
    // Lanzar el comando en segundo plano usando system()
    printf("Ejecutando: %s\n", command);
    int result = system(command);
    if (result == -1) {
        perror("Error al ejecutar system");
    } else {
        printf("Comando lanzado en segundo plano.\n");
    }
}

//===========================================
//================MARIADB====================
//===========================================
void mariadb_init(){
   if (!(conn = mysql_init(0)))
   {
      fprintf(stderr, "unable to initialize connection struct\n");
      exit(1);
   }

   mysql_ssl_set(conn, DBcertKeyPath, DBcertPath, DBcaPath, NULL, NULL);

   if (mysql_options(conn, MARIADB_OPT_TLS_PASSPHRASE, db_user_pass) != 0) {
      fprintf(stderr, "Error al establecer la frase de contraseña: %s\n", mysql_error(conn));
      mysql_close(conn);
      exit(-1);
   }

   // Connect to the database
   if (!mysql_real_connect(
         conn,                 // Connection
         MARIADB_HOST,// Host
         DB_USER,            // User account
         db_user_pass,   // User password
         DB_NAME,               // Default database
         MARIADB_PORT,                 // Port number
         NULL,                 // Path to socket file
         0                     // Additional options
      ))
   {
      // Report the failed-connection error & close the handle
      fprintf(stderr, "Error connecting to Server: %s\n", mysql_error(conn));
      mysql_close(conn);
      exit(1);
   }
}

void insertRecordName(char *filename, char *sensor){
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[2];
    char query[] = "INSERT INTO videos (nombre_video, sensor) VALUES (?, ?)";
    
    stmt = mysql_stmt_init(conn);
    if (!stmt) {
        fprintf(stderr, "mysql_stmt_init() failed\n");
        return;
    }
    
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        fprintf(stderr, "mysql_stmt_prepare(), ERROR: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }

    memset(bind, 0, sizeof(bind));

    // Nombre del video (VARCHAR)
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = filename;
    bind[0].buffer_length = strlen(filename);
    bind[0].is_null = 0;

    // Sensor (VARCHAR)
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = sensor;
    bind[1].buffer_length = strlen(sensor);
    bind[1].is_null = 0;

    printf("BIND VIDEO COMPLETADOS %s, %s\n", filename, sensor);

    // Enlazar parámetros
    if (mysql_stmt_bind_param(stmt, bind)) {
        fprintf(stderr, "mysql_stmt_bind_param() failed\n");
        fprintf(stderr, "%s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }

    // Ejecutar la consulta
    if (mysql_stmt_execute(stmt)) {
        fprintf(stderr, "mysql_stmt_execute() failed: %s\n", mysql_stmt_error(stmt));
    } else {
        printf("Registro insertado correctamente.\n");
    }
    
    // Limpiar
    mysql_stmt_close(stmt);
}

void insertEvent(cJSON *data){

    MYSQL_STMT *stmt;
    MYSQL_BIND bind[5];
    char query[] = "INSERT INTO "EVENTS_TABLE" \
                    ("EVENTS_TABLE_ENABLED", \
                    "EVENTS_TABLE_SENSOR_NAME", \
                    "EVENTS_TABLE_SENSOR_VALUE", \
                    "EVENTS_TABLE_SENDOR_ID", \
                    "EVENTS_TABLE_DATE") \
                    VALUES (?, ?, ?, ?, ?)";
    
    stmt = mysql_stmt_init(conn);
    if (!stmt) {
        fprintf(stderr, "mysql_stmt_init() failed\n");
        cJSON_Delete(data);
    }
    
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        fprintf(stderr, "mysql_stmt_prepare(), ERROR: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        cJSON_Delete(data);
    }

    memset(bind, 0, sizeof(bind));

    cJSON *json_item;
    int enabled;
    char *sensor_name;
    int sensor_value;
    int sensor_id;
    char *time;

    json_item = cJSON_GetObjectItem(data, "enabled");
    if (json_item != NULL) enabled = json_item->valueint;

    json_item = cJSON_GetObjectItem(data, "sensor");
    if (json_item != NULL) sensor_name = json_item->valuestring;

    json_item = cJSON_GetObjectItem(data, "value");
    if (json_item != NULL) sensor_value = json_item->valueint;

    json_item = cJSON_GetObjectItem(data, "sensorID");
    if (json_item != NULL) sensor_id = json_item->valueint;

    json_item = cJSON_GetObjectItem(data, "date");
    if (json_item != NULL) time = json_item->valuestring;
    
    printf("JSON COMPLETADOS\n");



    // Enabled (TINYINT)
    bind[0].buffer_type = MYSQL_TYPE_TINY;
    bind[0].buffer = (char *)&enabled; // 'enabled' debe ser un char o un signed/unsigned char
    bind[0].is_null = 0;
    
    // Sensor name (VARCHAR)
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = sensor_name; // 'sensor_name' debe ser una cadena de caracteres
    bind[1].buffer_length = strlen(sensor_name);
    
    // Sensor value (INT)
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &sensor_value; //INT
    bind[2].is_null = 0;
    
    // Sensor ID (SMALLINT)
    bind[3].buffer_type = MYSQL_TYPE_SHORT;
    bind[3].buffer = (char *)&sensor_id; // 'sensor_id' debe ser un short int
    bind[3].is_null = 0;
    
    // Event timestamp (DATETIME)
    bind[4].buffer_type = MYSQL_TYPE_STRING;
    bind[4].buffer = time; // 'time' debe ser una cadena de caracteres formateada
    bind[4].buffer_length = strlen(time);
    
    printf("BIND COMPLETADOS\n");

    // Enlazar parámetros
    if (mysql_stmt_bind_param(stmt, bind)) {
        fprintf(stderr, "mysql_stmt_bind_param() failed\n");
        fprintf(stderr, "%s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        cJSON_Delete(data);
    }

    // Ejecutar la consulta
    if (mysql_stmt_execute(stmt)) {
        fprintf(stderr, "mysql_stmt_execute(), failed: %s\n", mysql_stmt_error(stmt));
    } else {
        printf("Registro insertado correctamente.\n");
    }
    

    // Limpiar
    mysql_stmt_close(stmt);
    cJSON_Delete(data);
}

//===========================================
//================MOSQUITTO==================
//===========================================
static void mqttCallback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message){
    
    if (message->payloadlen) {
        printf("Received message on topic '%s': %s\n", message->topic, (char *)message->payload);
    } else {
        printf("Received message on topic '%s' but no payload\n", message->topic);
    }

    if (strcmp(message->topic, ALARM_TRIGGER) == 0){
        cJSON *data = cJSON_CreateObject();
        data = cJSON_Parse((char *)message->payload);

        if(data == NULL){
            fprintf(stderr, "Error antes de: [%s]\n", cJSON_GetErrorPtr());
        }else{
            
            time_t current_time;
            double seconds;

            // Obtener el tiempo actual
            time(&current_time);
            // Calcular la diferencia en segundos desde la última ejecución
            seconds = difftime(current_time, last_execution);
            printf("DIFFTIME %f\n",seconds);
            // Chequear si ha pasado el período de cooldown
            if (seconds > COOLDOWN_PERIOD || last_execution == 0) {

                cJSON *json_item;
                char *sensor = "";
                char *date = "";
                
                json_item = cJSON_GetObjectItem(data, "date");
                if (json_item != NULL) date = json_item->valuestring;
                
                json_item = cJSON_GetObjectItem(data, "sensor");
                if (json_item != NULL) sensor = json_item->valuestring;
                
                char filename[100];
                sprintf(filename, "%s_%s.mp4", date, sensor);
                char sensorAux[25];

                sprintf(sensorAux, "%s", sensor);
                record_video(filename, "10");

                printf("Procesando el mensaje:\n");
                insertEvent(data);
                insertRecordName(filename, sensorAux);
                
                last_execution = current_time;
                printf("CURRENT TIME %f\n",last_execution);
            } else {
                printf("Solicitud ignorada, cooldown en progreso.\n");
            }
        }
    }
}

static int pw_callback(char *buf, int size, int rwflag, void *userdata) {
    const char *password = "140516";  // Idealmente, la contraseña no debería estar codificada directamente aquí.

    if (size < strlen(password) + 1) {
        return 0; // El buffer proporcionado no es suficiente
    }

    strncpy(buf, password, size);
    buf[size - 1] = '\0'; // Asegúrate de que el buffer esté terminado en null
    return strlen(buf);   // Retornar la longitud de la contraseña escrita en el buffer
}

void mqtt_init(void){
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if(mosq == NULL){
        fprintf(stderr, "Error: Out of memory.\n");
		return;
    }

    int rc = mosquitto_tls_set(mosq, MQTTcaPath, "credentials/", MQTTcertPath, MQTTcertKeyPath, pw_callback);
    if(rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Failed to set TLS: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return;
    }
    mosquitto_tls_opts_set(mosq, 0, "tlsv1.3", NULL);
    mosquitto_username_pw_set(mosq, "app", "140516");

    mosquitto_connect(mosq, MQTT_BROKER, MQTT_PORT, 60);

    mosquitto_message_callback_set(mosq, mqttCallback);

    mosquitto_subscribe(mosq, NULL, ALARM_TRIGGER, 2);
}

void mqtt_loop(void){
    mosquitto_loop_forever(mosq, -1, 1);
}

void mqtt_close(void){
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}

int main() {
    
    mqtt_init();
    mariadb_init();

    mqtt_loop();
    
    return 0;
}