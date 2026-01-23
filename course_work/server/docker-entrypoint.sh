#!/bin/bash
set -e

# Копируем словари, если они еще не существуют в data директории
if [ ! -f /app/data/english.txt ]; then
    echo "Copying english.txt to /app/data..."
    cp /app/english.txt /app/data/ || { echo "Error: english.txt not found"; exit 1; }
fi

if [ ! -f /app/data/rockyou_1000k.txt ]; then
    echo "Copying rockyou_1000k.txt to /app/data..."
    cp /app/rockyou_1000k.txt /app/data/ || { echo "Error: rockyou_1000k.txt not found"; exit 1; }
fi

# Переходим в директорию данных
cd /app/data

# Запускаем сервер
exec "$@"
