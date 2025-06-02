SERVER_EXEC="./servidor"
LOG="monitoreo.log"

echo "[*] Iniciando monitoreo del servidor en ejecución..." | tee $LOG

SERVER_PID=$(pgrep -f "$SERVER_EXEC")

if [ -z "$SERVER_PID" ]; then
    echo "[!] No se encontró el servidor en ejecución. Abortando." | tee -a $LOG
    exit 1
fi

echo "[*] PID del servidor encontrado: $SERVER_PID" | tee -a $LOG

echo "[*] Threads del servidor:" | tee -a $LOG
ps -Lp $SERVER_PID | tee -a $LOG

echo "[*] Sockets abiertos por servidor:" | tee -a $LOG
lsof -Pan -p $SERVER_PID -iTCP -sTCP:LISTEN | tee -a $LOG

echo "[*] Monitoreando actividad. Presiona Enter para finalizar..." | tee -a $LOG

(
    while true; do
        if ! kill -0 $SERVER_PID 2>/dev/null; then
            echo "[!] El proceso del servidor ha finalizado." | tee -a $LOG
            break
        fi

        echo "--- $(date '+%Y-%m-%d %H:%M:%S') ---" | tee -a $LOG
        echo "[*] Conexiones TCP activas (puerto 8080):" | tee -a $LOG
        ss -tnp | grep ":8080" | tee -a $LOG

        echo "[*] Threads actuales del servidor:" | tee -a $LOG
        ps -T -p $SERVER_PID | tee -a $LOG

        sleep 1
    done
) &

MONITOR_PID=$!

read -r

kill $MONITOR_PID 2>/dev/null

echo "[✔] Monitoreo finalizado. Log guardado en $LOG"
exit 0
