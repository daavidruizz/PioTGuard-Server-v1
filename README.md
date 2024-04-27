# PioTGuard
    STREAM PART
STREAM SOURCE: rtsp_server.c
Compile: gcc -g src/rtsp_server.c -o rtsp_server `/usr/bin/pkg-config gstreamer-1.0 gstreamer-rtsp-1.0 gstreamer-rtsp-server-1.0 --libs --cflags`
Execute: ./rtsp_server

Ubuntu Client

Client source: rtsp_client.c
Compile -> Makefile
Execute: ./rtsp_client rtsps://192.168.1.211:8554/test

