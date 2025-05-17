FROM ubuntu:latest

RUN apt update && apt install -y \
    gcc \
    pkg-config \
    libfuse-dev

WORKDIR /app

COPY antink.c .

RUN gcc antink.c -o antink -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs`

CMD ["./antink"]
