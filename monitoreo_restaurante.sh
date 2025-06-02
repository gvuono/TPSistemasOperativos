PROGRAMA=./restaurante
LOGFILE="monitoreo_$(date +%Y%m%d_%H%M%S).log"

if [ ! -x "$PROGRAMA" ]; then
  echo "Error: No se encontró el ejecutable $PROGRAMA"
  exit 1
fi

echo "Iniciando monitoreo..."
echo "Log: $LOGFILE"

function capturar_estado() {
  echo "===== $(date) =====" >> "$LOGFILE"
  echo "[Procesos restaurante]" >> "$LOGFILE"
  ps -ef | grep restaurante | grep -v grep >> "$LOGFILE"

  echo "[Memoria compartida - ipcs -m]" >> "$LOGFILE"
  ipcs -m >> "$LOGFILE"

  echo "[Semáforos POSIX - ipcs -s]" >> "$LOGFILE"
  ipcs -s >> "$LOGFILE"

  echo "[top -bn1]" >> "$LOGFILE"
  top -bn1 | head -20 >> "$LOGFILE"
  echo "==============================" >> "$LOGFILE"
}

(
  while true; do
    capturar_estado
    sleep 3
  done
) &
MONITOR_PID=$!

$PROGRAMA

kill "$MONITOR_PID"

echo "[MONITOREO] Programa finalizado. Verificando limpieza..." | tee -a "$LOGFILE"

echo "===== VERIFICACIÓN FINAL =====" >> "$LOGFILE"
ipcs -m >> "$LOGFILE"
ipcs -s >> "$LOGFILE"

echo "[LIMPIEZA] Eliminando recursos de memoria compartida y semáforos..." | tee -a "$LOGFILE"

for id in $(ipcs -m | awk '/0x/ {print $2}'); do
  ipcrm -m "$id" && echo "[LIMPIEZA] Memoria compartida $id eliminada" >> "$LOGFILE"
done

for id in $(ipcs -s | awk '/0x/ {print $2}'); do
  ipcrm -s "$id" && echo "[LIMPIEZA] Semáforo $id eliminado" >> "$LOGFILE"
done

echo "[MONITOREO] Proceso finalizado. Log guardado en $LOGFILE"
