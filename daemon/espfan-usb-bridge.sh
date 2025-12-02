#!/bin/bash
set -u

PORT="/dev/ttyUSB0"
FAN_COUNT=5
LAST_PWM=( -1 -1 -1 -1 -1 )

# --- najít hwmon zařízení "espfan" ---
find_hwmon() {
    for d in /sys/class/hwmon/hwmon*; do
        if [ -f "$d/name" ] && [ "$(cat "$d/name")" = "espfan" ]; then
            echo "$d"
            return
        fi
    done
}

HWMON=$(find_hwmon)
[ -z "$HWMON" ] && { echo "Nenalezen hwmon espfan"; exit 1; }

echo "Používám hwmon: $HWMON"

# --- UART nastavení ---
stty -F "$PORT" 115200 -echo -icanon min 1 time 1

read_uart() {
    timeout 1 head -n 1 < "$PORT" | tr -d '\r'
}

# --- hlavní smyčka ---
while true; do

    for ((i=0; i<FAN_COUNT; i++)); do

        PWM_PATH="$HWMON/pwm$((i+1))"
        RPM_PATH="$HWMON/fan$((i+1))_input"

        # 1) --- čtení PWM od uživatele (sysfs) ---
        CUR_PWM=$(tr -d '[:space:]' < "$PWM_PATH")
        if [[ "$CUR_PWM" =~ ^[0-9]+$ ]] && [ "$CUR_PWM" -le 255 ]; then
            if [ "${LAST_PWM[$i]}" != "$CUR_PWM" ]; then
                echo -en "SET FAN $i $CUR_PWM\r" > "$PORT"
                sleep 0.2
                read_uart >/dev/null
                LAST_PWM[$i]=$CUR_PWM
            fi
        fi

        # 2) --- čtení RPM ---
        echo -en "GET RPM $i\r" > "$PORT"
        sleep 0.2
        R=$(read_uart)
        RPM=$(echo "$R" | awk '{print $3}')
        if [[ "$RPM" =~ ^[0-9]+$ ]]; then
            echo "$RPM" > "$RPM_PATH"
        fi

        # 3) --- čtení DUTY ---
        echo -en "GET DUTY $i\r" > "$PORT"
        sleep 0.2
        D=$(read_uart)
        DUTY=$(echo "$D" | awk '{print $3}')
        if [[ "$DUTY" =~ ^[0-9]+$ ]]; then
            echo "$DUTY" > "$PWM_PATH"
            LAST_PWM[$i]=$DUTY
        fi

    done

    sleep 0.2
done

