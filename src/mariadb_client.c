//gcc -o mariadb_client src/mariadb_client.c $(pkg-config --cflags --libs mariadb)

#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include "mariadb_client.h"

#define host "localhost"
#define port 3306

#define db_user "guard"
#define db_user_pass "140516"
#define db_name "PIOTGUARD_DB"

#define caPath "/home/guard/certificates/mariadb/mariadb_ca.crt"
#define certPath "/home/guard/certificates/mariadb/clients/localhost.crt"
#define certKeyPath "/home/guard/certificates/mariadb/clients/localhost.key"

void mariadb_init(){
   // Initialize Connection
   MYSQL *conn;
   if (!(conn = mysql_init(0)))
   {
      fprintf(stderr, "unable to initialize connection struct\n");
      exit(1);
   }

   mysql_ssl_set(conn, certKeyPath, certPath, caPath, NULL, NULL);

   if (mysql_options(conn, MARIADB_OPT_TLS_PASSPHRASE, db_user_pass) != 0) {
      fprintf(stderr, "Error al establecer la frase de contraseña: %s\n", mysql_error(conn));
      mysql_close(conn);
      return 1;
   }

   // Connect to the database
   if (!mysql_real_connect(
         conn,                 // Connection
         host,// Host
         db_user,            // User account
         db_user_pass,   // User password
         db_name,               // Default database
         3306,                 // Port number
         NULL,                 // Path to socket file
         0                     // Additional options
      ))
   {
      // Report the failed-connection error & close the handle
      fprintf(stderr, "Error connecting to Server: %s\n", mysql_error(conn));
      mysql_close(conn);
      exit(1);
   }

      if (mysql_query(conn, "SHOW SESSION STATUS LIKE 'Ssl_cipher'")) {
      fprintf(stderr, "Error al ejecutar la consulta: %s\n", mysql_error(conn));
      mysql_close(conn);
      return 1;
   }
}

void mariadb_close(){
   mysql_close(conn);
}