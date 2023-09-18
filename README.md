# LRCP

LRCP is Lightweight Relay Control Protocol,
connection-oriented, multiplex-supported.

pprc is peer-to-peer relay chat programme,
written with ANSI C, based on LRCP.

## Syntax

```
pprc [dest-ip] [dest-port] [port]
```

Establish connection directly to peer.
UNIX user name will be used to identify you with your ip.

## Requisites

- OpenSSL

## Compiling

```
make -j `nproc`
```
