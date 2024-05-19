#include "commands.h"
#include "packet.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 12345
#define FILE_NAME_SIZE_MAX 32 + 1 /* null */
#define FILE_DOWNLOAD_TABLE "/home/vinay_divakar/file_download"
#define FILE_DOWNLOAD_PATH_NAME_SIZE_MAX 64

struct file_download_t {
  int fd;
  FILE *fp;
  size_t download_total;
  char filename[FILE_NAME_SIZE_MAX];
};

struct pollfd fds[1] = {};
int on = 1;

/**
 * @brief connects to the server
 *
 * @return 0 success, <0 error
 */
int _connect(void) {
  int err = 0;

  do {
    // socket create and verification
    err = socket(AF_INET, SOCK_STREAM, 0);
    if (err < 0) {
      printf("error socket create\r\n");
      break;
    }

    fds[0].fd = err;

    printf("socket create success fd %d\r\n", fds[0].fd);

    struct sockaddr_in servaddr = {};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    err = connect(fds[0].fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (err < 0) {
      printf("connection with the server failed...\n");
      break;
    }
  } while (0);

  return err;
}

/**
 * @brief sends a request packet to the server
 *
 * @param[in] packet points to the request to be sent
 * @return 0 success, <0 error
 */
int packet_send(packet_t *packet) {
  size_t packet_size_total = packet->packet_struct.length + PACKET_HEADER_SIZE;
  int err = send(fds[0].fd, packet->data, packet_size_total, 0);
  if (err < 0) {
    printf("error sending %d %d\r\n", err, errno);
  } else if (err != packet_size_total) {
    printf("error send mismacth %d %ld %d\r\n", err, packet_size_total, errno);
    err = -1;
  }
  return err;
}

/**
 * @brief reads from the socket
 *
 * @param[in] fd connection handler
 * @param[out] recv_buff points to the buffer to be populated with recv data
 * @param[in] recv_buff_size size of the recv buffer
 * @return number of bytes read or <0 on error
 */
int server_read(int fd, uint8_t *recv_buff, size_t recv_buff_size) {
  int result = 0, total_recieved = 0, received = 0;
  size_t offset = 0;

  do {
    received = recv(fd, recv_buff + offset, recv_buff_size - offset, 0);
    if (received == 0) {
      result = total_recieved;
      break;
    } else if (received < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        result = total_recieved;
        break;
      }
      result = -errno;
      break;
    } else {
      total_recieved += received;
      offset += received;

      if (offset >= recv_buff_size) {
        offset = 0;
      }
    }
  } while (true);

  return result;
}

/**
 * @brief performs file write
 */
int file_write(const char *table, const char *key, void *data, size_t size,
               size_t offset, FILE *fp, bool *eof) {
  int err = 0, close_err = 0;

  char path[FILE_DOWNLOAD_PATH_NAME_SIZE_MAX] = {};
  snprintf(path, sizeof(path), "%s/%s", table, key);

  printf("path: %s\r\n", path);

  do {
    fp = fopen(path, "a");
    if (!fp) {
      err = -errno;
      printf("error fopen %d\r\n", errno);
      break;
    }

    err = fseek(fp, offset, SEEK_SET);
    if (err < 0) {
      err = -errno;
      printf("error fseek %d %d\r\n", err, errno);
      break;
    }

    err = fwrite(data, 1, size, fp);
    if (ferror(fp)) { // check of errors
      err = -EIO;
      clearerr(fp);
      printf("error fwrite\r\n");
    } else if (feof(fp)) { // check for EOF
      printf("reached EOF\r\n");
      *eof = true;
    } else if (err != size) {
      printf("write lesser %d than expected size %ld\r\n", err, size);
    }

    close_err = fclose(fp);
    if (close_err) {
      err = -errno;
      printf("error close %d\r\n", errno);
    }
  } while (0);

  return err;
}

/**
 * @brief performs file download
 */
int file_download(struct file_download_t *ptr) {
  int err = 0;
  uint8_t buffer[32] = {};

  err = server_read(ptr->fd, buffer, sizeof(buffer) / sizeof(buffer[0]));
  if (err <= 0) {
    err = (err == 0) ? -ENETRESET : err;
  } else if (err == 1 && buffer[0] == '\0') {
    printf("EOF recd\r\n");
    err = EOF;
  } else {
    printf("recv: %.*s\r\n", err, (char *)buffer);
    // TODO: write received contents to file
  }

  return err;
}

int main(int argc, char *argv[]) {
  int err = 0;
  fds[0].fd = -1;

  if (argc > 2) {
    printf("invalid number of args %d", argc);
    exit(EXIT_FAILURE);
  }

  if (!argv[1]) {
    printf("invalid or missing filename input\r\n");
    exit(EXIT_FAILURE);
  }

  packet_t request = {.packet_struct.cmd = CMD_DOWNLOAD_FILE,
                      .packet_struct.length = strlen(argv[1])};

  size_t copy_size = (request.packet_struct.length < FILE_NAME_SIZE_MAX)
                         ? request.packet_struct.length
                         : FILE_NAME_SIZE_MAX;

  memcpy(request.packet_struct.data, argv[1], copy_size);
  request.packet_struct.data[copy_size] = '\n';

  printf("cmd:0x%02X file:%s size: %d\r\n", request.packet_struct.cmd,
         (char *)request.packet_struct.data, request.packet_struct.length);

  err = _connect();
  if (err < 0 && fds[0].fd >= 0) {
    close(fds[0].fd);
    exit(EXIT_FAILURE);
  }
  printf("socket connection success\r\n");

  err = packet_send(&request);
  if (err < 0) {
    close(fds[0].fd);
    exit(EXIT_FAILURE);
  }

  printf("command send success, \r\n");

  // configure socket to be non-blocking for reads
  err = ioctl(fds[0].fd, FIONBIO, (char *)&on);
  if (err < 0) {
    printf("error %d ioctl\r\n", err);
    close(fds[0].fd);
    exit(EXIT_FAILURE);
  }

  fds[0].events = POLLIN;

  struct file_download_t download = {.fd = fds[0].fd};
  memcpy(download.filename, request.packet_struct.data, copy_size);

  while (1) {
    err = poll(fds, 1, -1);
    if (err < 0) {
      printf("error %d errno %d polling\r\n", err, errno);
      break;
    }

    if (fds[0].revents & POLLIN) {
      fds[0].revents &= ~POLLIN;
      err = file_download(&download);
      if (err < 0) {
        printf("server closed connection or EOF recvd on fd %d, code %d\r\n",
               fds[0].fd, err);
        break;
      }
    } else {
      printf("unexpected event %d on fd %d\r\n", fds[0].revents, fds[0].fd);
      break;
    }
  }

  close(fds[0].fd);
}
