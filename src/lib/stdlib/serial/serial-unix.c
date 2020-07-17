#include <ctype.h>
#include <cube/cubeext.h>
#include <dirent.h>
#include <errno.h> /* ERROR Number Definitions          */
#include <fcntl.h> /* File Control Definitions          */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>/* POSIX Terminal Control Definitions*/
#include <time.h>
#include <unistd.h> /* UNIX Standard Definitions         */

#ifndef CRTSCTS
#define CRTSCTS 020000000000
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode)&S_IFMT) == S_IFDIR)
#endif

static int parse_serial_speed(int baudrate);

cube_native_var *list_serial()
{
    cube_native_var *list = NATIVE_LIST();

    char *path = "/sys/class/tty/";

    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (dir->d_name[0] != '.')
                ADD_NATIVE_LIST(list, NATIVE_STRING_COPY(dir->d_name));
        }
        closedir(d);
    }

    return list;
}

int open_serial(const char *device)
{
    int fd = open(device, O_RDWR | O_NOCTTY);
    return fd;
}

void close_serial(int fd)
{
    close(fd);
}

int configure_serial(int fd, int baudrate, int timeout, int parity, int stopBits, int dataBits, int rts, int xon,
                     int can)
{
    struct termios SerialPortSettings;
    tcgetattr(fd, &SerialPortSettings);

    int speed = -1;
    tcflag_t c_cflag = SerialPortSettings.c_cflag;

    if (baudrate >= 0)
    {
        speed = parse_serial_speed(baudrate);
        if (speed < 0)
            return -1;
    }

    if (parity >= 0)
    {
        if (parity == 0)
            c_cflag &= ~PARENB;
        else if (parity == 1)
        {
            c_cflag |= PARENB;
            c_cflag |= PARODD;
        }
        else if (parity == 2)
        {
            c_cflag |= PARENB;
            c_cflag &= ~PARODD;
        }
        else
            return -2;
    }

    if (stopBits >= 0)
    {
        if (stopBits == 1)
            c_cflag &= ~CSTOPB;
        else if (stopBits == 2)
            c_cflag |= CSTOPB;
        else
            return -3;
    }

    if (dataBits >= 0)
    {
        c_cflag &= ~CSIZE;
        if (dataBits == 5)
            c_cflag |= CS5;
        else if (dataBits == 6)
            c_cflag |= CS6;
        else if (dataBits == 7)
            c_cflag |= CS7;
        else if (dataBits == 8)
            c_cflag |= CS8;
        else
            return -4;
    }

    if (rts >= 0)
    {
        if (rts == 0)
            c_cflag &= ~CRTSCTS;
        else if (rts == 1)
            c_cflag |= CRTSCTS;
        else
            return -5;
    }

    if (xon >= 0)
    {
        if (xon == 0)
            c_cflag &= ~(IXON | IXOFF | IXANY);
        else if (xon == 1)
            c_cflag |= (IXON | IXOFF | IXANY);
        else
            return -6;
    }

    if (can >= 0)
    {
        if (can == 0)
            c_cflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        else if (can == 1)
            c_cflag |= (ICANON | ECHO | ECHOE | ISIG);
        else
            return -7;
    }

    int rc;
    if (speed >= 0)
    {
        rc = cfsetispeed(&SerialPortSettings, (speed_t)speed);
        rc = cfsetospeed(&SerialPortSettings, (speed_t)speed);
    }

    c_cflag |= CREAD | CLOCAL;
    if (timeout >= 0)
    {
        timeout = 100 * round(timeout / 100.0);
        timeout /= 100;

        if (timeout == 0)
            SerialPortSettings.c_cc[VMIN] = 1;
        else
            SerialPortSettings.c_cc[VMIN] = 0;
        SerialPortSettings.c_cc[VTIME] = (unsigned char)timeout;
    }
    else
    {
        SerialPortSettings.c_cc[VMIN] = 0;
        SerialPortSettings.c_cc[VTIME] = 0;
    }

    SerialPortSettings.c_cflag = c_cflag;
    rc = tcsetattr(fd, TCSANOW, &SerialPortSettings);

    return 1;
}

int write_serial(int fd, const void *data, size_t len)
{
    return write(fd, data, len);
}

cube_native_var *read_serial(int fd, size_t len)
{
    cube_native_var *ret = NATIVE_NULL();
    unsigned char *data = malloc(sizeof(unsigned char) * len);
    int read_len = read(fd, data, len);
    if (read_len >= 0)
        TO_NATIVE_BYTES_ARG(ret, read_len, data);
    return ret;
}

// Utils
static int parse_serial_speed(int baudrate)
{
    speed_t speed = B9600;
    switch (baudrate)
    {
        case 0:
            speed = B0;
            break;
        case 50:
            speed = B50;
            break;
        case 75:
            speed = B75;
            break;
        case 110:
            speed = B110;
            break;
        case 134:
            speed = B134;
            break;
        case 150:
            speed = B150;
            break;
        case 200:
            speed = B200;
            break;
        case 300:
            speed = B300;
            break;
        case 600:
            speed = B600;
            break;
        case 1200:
            speed = B1200;
            break;
        case 1800:
            speed = B1800;
            break;
        case 2400:
            speed = B2400;
            break;
        case 4800:
            speed = B4800;
            break;
        case 9600:
            speed = B9600;
            break;
        case 19200:
            speed = B1200;
            break;
        case 38400:
            speed = B38400;
            break;
        case 57600:
            speed = B57600;
            break;
        case 115200:
            speed = B115200;
            break;
        case 230400:
            speed = B230400;
            break;
        case 460800:
            speed = B460800;
            break;
        case 500000:
            speed = B500000;
            break;
        case 576000:
            speed = B576000;
            break;
        case 921600:
            speed = B921600;
            break;
        case 1000000:
            speed = B1000000;
            break;
        case 1152000:
            speed = B1152000;
            break;
        case 1500000:
            speed = B1500000;
            break;
        case 2000000:
            speed = B2000000;
            break;
        case 2500000:
            speed = B2500000;
            break;
        case 3000000:
            speed = B3000000;
            break;
        case 3500000:
            speed = B3500000;
            break;
        case 4000000:
            speed = B4000000;
            break;

        default:
            return -1;
            break;
    }
    return speed;
}
