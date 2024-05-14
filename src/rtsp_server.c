/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
//===================================================================
// Based on https://github.com/enthusiasticgeek/gstreamer-rtsp-ssl-example/blob/master/rtsp_server.c#L146
// David Ruiz Luque <daavidruiz01@outlook.com>
//===================================================================
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gio/gio.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include "../include/config_struct.h"

void handleSignal(int signum){
    printf("\nSaliendo...\n");
    syslog(LOG_ERR, "Senal recibida %d. Saliendo...", signum);
    closelog();
    exit(signum);
}

/* this timeout is periodically run to clean up the expired sessions from the
 * pool. This needs to be run explicitly currently but might be done
 * automatically as part of the mainloop. */
static gboolean timeout(GstRTSPServer *server){
    GstRTSPSessionPool *pool;

    pool = gst_rtsp_server_get_session_pool(server);
    gst_rtsp_session_pool_cleanup(pool);
    g_object_unref(pool);

    return TRUE;
}

gboolean accept_certificate (GstRTSPAuth *auth,
                    GTlsConnection *conn,
                    GTlsCertificate *peer_cert,
                    GTlsCertificateFlags errors,
                    gpointer user_data) {

    GError *error = NULL;
    gboolean accept = FALSE;
    GTlsCertificate *ca_tls_cert = (GTlsCertificate *) user_data;
    GTlsDatabase* database = g_tls_connection_get_database(G_TLS_CONNECTION(conn));

    if (database) {
        GSocketConnectable *peer_identity;
        GTlsCertificateFlags validation_flags;
        g_debug ("TLS peer certificate not accepted, checking user database...\n");
        peer_identity = NULL;
        errors =
            g_tls_database_verify_chain (database, peer_cert,
                                         G_TLS_DATABASE_PURPOSE_AUTHENTICATE_CLIENT, peer_identity,
                                         g_tls_connection_get_interaction (conn), G_TLS_DATABASE_VERIFY_NONE,
                                         NULL, &error);
        g_print("errors value %d\n",errors);

        //g_object_unref (database);
        if (error || errors)
        {
            g_warning ("failure verifying certificate chain: %s",
                       error->message);
            g_assert (errors != 0);
            g_clear_error (&error);
        }
    }

    if (error == 0 || errors == 0) {
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char *argv[]){
    
    g_setenv("G_MESSAGES_DEBUG", "all", TRUE);
    
    // Configurar el manejador de señal 
    if (signal(SIGTERM, handleSignal) == SIG_ERR) {
        perror("Exit. -15 Signal");
    }else if(signal(SIGINT, handleSignal) == SIG_ERR){
        perror("Exit. Ctrl+C");
    }
    
    openlog("PioTGuard-RTSP Server", LOG_PID | LOG_NDELAY, LOG_USER);
    syslog(LOG_INFO, "Initializing RTSP...");

    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    /*Auth*/
    GstRTSPAuth *auth;
    GstRTSPToken *token;
    gchar *basic;
    GstRTSPPermissions *permissions;
    /*TLS*/
    GTlsCertificate *cert;
    GTlsCertificate *ca_cert;
    GTlsDatabase* database;
    GError *error = NULL;

    /*load config struct*/
    struct config rtsp_config;
    rtsp_config = get_config(FILENAME);

    syslog(LOG_INFO, "LOADING CONFIG");
    syslog(LOG_INFO, "%s", rtsp_config.rtsp_ca_cert);
    syslog(LOG_INFO, "%s", rtsp_config.rtsp_cert_pem);
    syslog(LOG_INFO, "%s", rtsp_config.rtsp_cert_key);
    syslog(LOG_INFO, "%s", rtsp_config.rtsp_server_port);
    syslog(LOG_INFO, "%s", rtsp_config.rtsp_server_mount_point);
    syslog(LOG_INFO, "%s", rtsp_config.rtsp_server_username);
    syslog(LOG_INFO, "%s", rtsp_config.rtsp_server_password);

    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    /*server instance*/
    server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, rtsp_config.rtsp_server_port);

    /*authenticaction manager & TLS*/
    auth = gst_rtsp_auth_new();
    syslog(LOG_INFO, "Starting TLS authentication");
    
    cert = g_tls_certificate_new_from_files(rtsp_config.rtsp_cert_pem,rtsp_config.rtsp_cert_key,&error);
    if(cert == NULL){
         g_printerr ("failed to parse PEM: %s\n", error->message);
        return -1;
    }

    database = g_tls_file_database_new (rtsp_config.rtsp_ca_cert, &error);
    gst_rtsp_auth_set_tls_database (auth, database);
    
    ca_cert = g_tls_certificate_new_from_file(rtsp_config.rtsp_ca_cert,&error);

    if(ca_cert == NULL){
        g_printerr ("failed to parse CA PEM: %s\n", error->message);
        return -1;
    }
    
    gst_rtsp_auth_set_tls_authentication_mode(auth, G_TLS_AUTHENTICATION_REQUIRED);
    gst_rtsp_auth_set_tls_certificate (auth, cert);
    g_signal_connect (auth, "accept-certificate", G_CALLBACK(accept_certificate), ca_cert);

    /*user token*/
    token = gst_rtsp_token_new (GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, rtsp_config.rtsp_server_username, NULL);
    basic = gst_rtsp_auth_make_basic (rtsp_config.rtsp_server_username, rtsp_config.rtsp_server_password);
    
    gst_rtsp_auth_add_basic (auth, basic, token);
    g_free (basic);
    gst_rtsp_token_unref (token);

    gst_rtsp_server_set_auth (server, auth);

    /* get the mounts for this server, every server has a default mapper object
    * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points(server);

    /* make a media factory for a test stream. The default media factory can use
    * gst-launch syntax to create pipelines.
    * any launch line works as long as it contains elements named pay%d. Each
    * element with pay%d names will be a stream */
    factory = gst_rtsp_media_factory_new();
    /*
    
    gst_rtsp_media_factory_set_launch(factory, "( "
                                            "videotestsrc ! video/x-raw,width=352,height=288,framerate=15/1 ! "
                                            "x264enc ! rtph264pay name=pay0 pt=96 "
                                        //    "audiotestsrc ! audio/x-raw,rate=8000 ! "
                                        //    "alawenc ! rtppcmapay name=pay1 pt=97 "
                                            ")");
    
    */
/*
    gst_rtsp_media_factory_set_launch(factory, "( "
                                             "v4l2src device=/dev/video0 ! video/x-h264, width=640, height=480, framerate=30/1 ! "
                                             "h264parse config-interval=1 ! "
                                             "rtph264pay name=pay0 pt=96 "
                                             ")");
*/ 
     
    gst_rtsp_media_factory_set_launch(factory, "( "
                                             "v4l2src device=/dev/video1 ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! "
                                             "x264enc speed-preset=ultrafast ! h264parse config-interval=1 ! "
                                             "rtph264pay name=pay0 pt=96 "
                                             ")");

    
    

//v4l2src device=/dev/video0 ! videoconvert ! videoscale ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! v4l2sink device=/dev/video17


    /* add permissions for the user media role */
    permissions = gst_rtsp_permissions_new ();
    gst_rtsp_permissions_add_role (permissions, rtsp_config.rtsp_server_username,
                                    GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, TRUE,
                                    GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, TRUE, NULL);
    
    gst_rtsp_media_factory_set_permissions (factory, permissions);
    gst_rtsp_permissions_unref (permissions);
    gst_rtsp_media_factory_set_profiles (factory, GST_RTSP_PROFILE_SAVP | GST_RTSP_PROFILE_SAVPF);
    
    /*factory to path*/
    gst_rtsp_mount_points_add_factory(mounts, rtsp_config.rtsp_server_mount_point, factory);
    
    /* don't need the ref to the mapper anymore */
    g_object_unref(mounts);

    if (gst_rtsp_server_attach (server, NULL) == 0){
        perror("failed to attach the server\n");
        return -1;
    }

    g_timeout_add_seconds(2, (GSourceFunc)timeout, server);

    /* start serving, this never stops */
    g_main_loop_run (loop);
    
    return 0;
}