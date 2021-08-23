#pragma once
#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>

#include "mana/network/servercommon.h"

#define PORT 8829
#define BUFLEN 512

#ifdef PLATFORM_WIN32
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define sleep(a) Sleep((a))

struct ZealSocket {
  SOCKET udp_socket_h, tcp_socket_h;
  struct sockaddr_in serv_addr, client_addr;
  socklen_t s_len, recv_len;
  WSADATA wsa_data;
};
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define sleep(a) usleep((a)*1000)

struct ZealSocket {
  int socket_h;
  struct sockaddr_in serv_addr, client_addr;
};
#endif

static inline int socket_init(struct ZealSocket *zeal_socket) {
#ifdef PLATFORM_WIN32
  WSADATA wsa;
  SOCKET s;
  struct sockaddr_in server;
  char *message, server_reply[2000];
  int recv_size = 4096;

  printf("\nInitialising Winsock...");
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    printf("Failed. Error Code : %d", WSAGetLastError());
    return 1;
  }

  printf("Initialised.");

  //Create a socket
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
    printf("Could not create socket : %d", WSAGetLastError());
  }

  printf("Socket created.\n");

  server.sin_addr.s_addr = inet_addr("52.13.247.47");
  server.sin_family = AF_INET;
  server.sin_port = htons(2757);

  //Connect to remote server
  if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
    puts("connect error");
    return 1;
  }

  puts("Connected");

  //Send some data
  message = "GET / HTTP/1.1\r\n\r\n";
  if (send(s, message, strlen(message), 0) < 0) {
    puts("Send failed");
    return 1;
  }
  puts("Data Send\n");

  //Receive a reply from the server
  //if ((recv_size = recv(s, server_reply, 2000, 0)) == SOCKET_ERROR) {
  //  puts("recv failed");
  //}

  puts("Reply received\n");

  //Add a NULL terminating character to make it a proper string before printing
  server_reply[recv_size] = '\0';
  puts(server_reply);
#else
#endif
  return 0;
}

static inline int socket_delete(struct ZealSocket *zeal_socket) {
#ifdef PLATFORM_WIN32
  closesocket(zeal_socket->udp_socket_h);
  WSACleanup();
#else
#endif
  return 0;
}

static inline int socket_recieve(struct ZealSocket *zeal_socket, char *buf) {
#ifdef PLATFORM_WIN32

#else
#endif
  return 0;
}

static inline int socket_send(struct ZealSocket *zeal_socket, char *buf) {
#ifdef PLATFORM_WIN32

#else
#endif
  return 0;
}

#endif  // SOCKET_H
