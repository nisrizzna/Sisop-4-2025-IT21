FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y fuse gcc make libfuse-dev pkg-config

RUN mkdir /mnt/antink_mount /mnt/original /mnt/logs

COPY antink.c .

RUN gcc -D_FILE_OFFSET_BITS=64 -Wall antink.c $(pkg-config fuse --cflags --libs) -o /antink

VOLUME ["/mnt/antink_mount", "/mnt/original", "/mnt/logs"]

CMD ["/antink", "/mnt/antink_mount", "-f"]
