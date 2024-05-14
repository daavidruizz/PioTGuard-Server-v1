#!/bin/bash
# Uso: ./record_video.sh <nombre_de_archivo> <duracion_en_segundos>

export PATH=/usr/bin:/bin:/usr/sbin:/sbin:$PATH

RECORD_PATH="/home/guard/records/"
FILENAME=$1
DURATION=$2
PROCESS_NAME="guard-recording"

# Buscar cualquier proceso existente con el mismo nombre y matarlo
PID=$(ps aux | grep "$PROCESS_NAME" | grep -v grep | awk '{print $2}')
if [ ! -z "$PID" ]; then
    echo "Encontrado proceso existente con PID $PID, matando..."
    kill $PID
    # Espera un momento para que el proceso se cierre correctamente
    sleep 5
fi

echo "Grabando: ${FILENAME}"
/usr/bin/gst-launch-1.0 -e v4l2src device=/dev/video2 num-buffers=$(($DURATION * 30)) \
  ! queue max-size-buffers=10 max-size-bytes=0 max-size-time=0 \
  ! video/x-raw,format=I420,framerate=30/1 \
  ! videoconvert \
  ! x264enc bitrate=5000 speed-preset=ultrafast \
  ! mp4mux \
  ! filesink location="${RECORD_PATH}${FILENAME}"

echo "Video guardado en: ${RECORD_PATH}${FILENAME}"
