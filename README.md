# A simple share file server in C

  With simple function have vulnerability because is construction
  without SSL or Cryptography in two points. Use to internal network
  and simple paper or docs.


# Dependency
  libmicrohttpd, dnsmasq, nginx

gcc upload_server.c -o upload_server -lmicrohttpd

# HTTP file server lan

```
    if (strstr(filename, "..") || strchr(filename, '/'))
    rejeita();
```

using event loop

┌──────────────┐
│   Cliente    │
│ (browser /   │
│  curl / app) │
└──────┬───────┘
       │ HTTP
       ▼
┌─────────────────────────────┐
│        HTTP Server          │
│   (libmicrohttpd / socket)  │
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│        Router / API         │
│   (dispatch por método)     │
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│   File Service (Core)       │
│   - upload                  │
│   - download                │
│   - delete                  │
│   - rename                  │
│   - list                    │
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│  Storage Layer (FS local)   │
│  - valida path              │
│  - fopen/read/write/unlink  │
│  - locks                    │
└──────────────┬──────────────┘
               ▼
┌─────────────────────────────┐
│      Sistema de Arquivos    │
│           (/data)           │
└─────────────────────────────┘

if (method == GET && path == "/files")
    handle_list();
else if (method == GET && starts_with(path, "/files"))
    handle_download();
else if (method == POST && path == "/upload")
    handle_upload();
else if (method == DELETE && starts_with(path, "/files/"))
    handle_delete();


http server:
> apenas camada de conexão
> recebe request
>> router methods, path e query string
file service:
> file name
> path
> overwrite
> locks
> storage control
storage:
> write,read,delete,rename,list

nunca deve sair do diretorio base, prefixar /data
```
char fullpath[PATH_MAX];
snprintf(fullpath, sizeof(fullpath), "%s/%s", DATA_DIR, filename);
```

Internet
   │
   │ HTTPS (443)
   ▼
┌─────────────────────────────┐
│   Reverse Proxy (Nginx)     │
│   - TLS / Certificados     │
│   - HTTP/2                 │
│   - Rate limit             │
│   - Logs                   │
└──────────────┬──────────────┘
               │ HTTP puro
               ▼
┌─────────────────────────────┐
│     Seu servidor em C       │
│   (libmicrohttpd, 8080)     │
└──────────────┬──────────────┘
               ▼
           Filesystem


iptables -A INPUT -p tcp --dport 8080 -s 192.168.0.0/24 -j ACCEPT
iptables -A INPUT -p tcp --dport 8080 -j DROP


DNS
http://share/

```
server {
    listen 80;
    server_name share;

    client_max_body_size 100M;

    location / {
        proxy_pass http://127.0.0.1:8080;
    }
}
```

```
version: "3.8"

services:
  proxy:
    image: nginx:alpine
    container_name: share-proxy
    ports:
      - "80:80"
    volumes:
      - ./nginx.conf:/etc/nginx/conf.d/default.conf:ro
    depends_on:
      - app

  app:
    build: ./app
    container_name: share-app
    expose:
      - "8080"
```
