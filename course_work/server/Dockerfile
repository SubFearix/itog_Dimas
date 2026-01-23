# Используем Ubuntu как базовый образ
FROM ubuntu:22.04

# Устанавливаем необходимые зависимости
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libsodium-dev \
    && rm -rf /var/lib/apt/lists/*

# Создаем рабочую директорию
WORKDIR /app

# Копируем исходные файлы сервера
COPY . /app/

# Создаем директорию для сборки
RUN mkdir -p build

# Собираем проект
WORKDIR /app/build
RUN cmake .. && make

# Создаем директорию для данных и копируем словари туда
RUN mkdir -p /app/data && \
    test -f /app/build/english.txt && cp /app/build/english.txt /app/ || { echo "Error: english.txt not found in build directory"; exit 1; } && \
    test -f /app/build/rockyou_1000k.txt && cp /app/build/rockyou_1000k.txt /app/ || { echo "Error: rockyou_1000k.txt not found in build directory"; exit 1; }

# Копируем entrypoint скрипт
COPY docker-entrypoint.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

# Возвращаемся в директорию с данными как рабочую директорию
WORKDIR /app/data

# Открываем порт 8080
EXPOSE 8080

# Устанавливаем entrypoint
ENTRYPOINT ["docker-entrypoint.sh"]

# Запускаем сервер
CMD ["/app/build/password_server", "8080"]
