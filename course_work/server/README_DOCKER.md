# Password Manager Server - Docker Setup

Этот сервер менеджера паролей можно запустить в Docker контейнере.

## Требования

- Docker
- Docker Compose

## Быстрый старт

### Сборка и запуск с помощью docker-compose

```bash
cd course_work/server
docker-compose up -d
```

Сервер будет доступен на порту `8080`.

### Остановка сервера

```bash
docker-compose down
```

### Просмотр логов

```bash
docker-compose logs -f
```

### Пересборка образа

Если вы внесли изменения в код:

```bash
docker-compose up -d --build
```

## Альтернативный способ (без docker-compose)

### Сборка образа

```bash
docker build -t password-manager-server .
```

### Запуск контейнера

```bash
docker run -d \
  --name password_manager_server \
  -p 8080:8080 \
  -v $(pwd)/data:/app/data \
  password-manager-server
```

## Хранение данных

Все данные пользователей (users.json и vaults) хранятся в директории `./data`, которая монтируется как volume. Это гарантирует, что данные сохраняются даже при перезапуске контейнера.

## Подключение к серверу

Клиент может подключиться к серверу по адресу:
- Локально: `localhost:8080`
- С другого компьютера в сети: `<IP-адрес-сервера>:8080`

## Порты

По умолчанию сервер слушает порт `8080`. Этот порт пробрасывается из контейнера на хост-машину.

## Структура файлов

```
course_work/server/
├── Dockerfile           # Конфигурация Docker образа
├── docker-compose.yml   # Конфигурация docker-compose
├── .dockerignore        # Файлы, исключаемые из образа
├── data/                # Директория для данных (создается автоматически)
│   ├── users.json       # Файл пользователей
│   └── server_vaults/   # Директория с хранилищами паролей
└── ... (исходные файлы сервера)
```

## Переменные окружения

- `TZ`: Часовой пояс (по умолчанию `Europe/Moscow`)

## Решение проблем

### Порт уже занят

Если порт 8080 уже используется, измените его в `docker-compose.yml`:

```yaml
ports:
  - "8081:8080"  # Изменить 8081 на желаемый порт
```

### Контейнер не запускается

Проверьте логи:

```bash
docker-compose logs
```

### Пересоздание контейнера

```bash
docker-compose down
docker-compose up -d --build
```
