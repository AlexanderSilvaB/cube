#include <cube/cubeext.h>
#include <windows.h>

#define MAX_PORTS 1024

static bool inited = false;
static HANDLE handles[MAX_PORTS];

static void init_serial();
static int add_port(HANDLE port);
static HANDLE get_port(int fd);
static void remove_port(int fd);
static int parse_serial_speed(int baudrate);

int open_serial(const char *device)
{
    HANDLE port =
        CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    int fd = add_port(port);
    return fd;
}

void close_serial(int fd)
{
    HANDLE port = get_port(fd);
    if (port == INVALID_HANDLE_VALUE)
        return;
    CloseHandle(port);
    remove_port(fd);
}

int configure_serial(int fd, int baudrate, int timeout, int parity, int stopBits, int dataBits, int rts, int xon,
                     int can)
{
    HANDLE port = get_port(fd);
    if (port == INVALID_HANDLE_VALUE)
        return 0;

    BOOL success;

    DCB state;
    memset(&state, '\0', sizeof(DCB));
    state.DCBlength = sizeof(DCB);
    success = GetCommState(port, &state);
    if (!success)
        return 0;

    int speed = -1;
    if (baudrate >= 0)
    {
        speed = parse_serial_speed(baudrate);
        if (speed < 0)
            return -1;
    }

    if (parity >= 0)
    {
        if (parity == 0)
            state.Parity = NOPARITY;
        else if (parity == 1)
            state.Parity = ODDPARITY;
        else if (parity == 2)
            state.Parity = EVENPARITY;
        else
            return -2;
    }

    if (stopBits >= 0)
    {
        if (stopBits == 0)
            state.StopBits = ONE5STOPBITS;
        else if (stopBits == 1)
            state.StopBits = ONESTOPBIT;
        else if (stopBits == 2)
            state.StopBits = TWOSTOPBITS;
        else
            return -3;
    }

    if (dataBits >= 0)
    {

        if (dataBits == 5)
            state.ByteSize = 5;
        else if (dataBits == 6)
            state.ByteSize = 6;
        else if (dataBits == 7)
            state.ByteSize = 7;
        else if (dataBits == 8)
            state.ByteSize = 8;
        else
            return -4;
    }

    state.BaudRate = speed;

    success = SetCommState(port, &state);
    if (!success)
    {
        return 0;
    }

    if (rts >= 0)
    {
        if (rts == 0)
        {
            state.fOutxCtsFlow = FALSE;
            state.fDsrSensitivity = FALSE;
            state.fRtsControl = RTS_CONTROL_DISABLE;
        }
        else if (rts == 1)
        {
            state.fOutxCtsFlow = TRUE;
            state.fRtsControl = RTS_CONTROL_ENABLE;
        }
        else
            return -5;
    }

    if (xon >= 0)
    {
        if (xon == 0)
        {
            state.fOutX = FALSE;
            state.fInX = FALSE;
        }
        else if (xon == 1)
        {
            state.fOutX = TRUE;
            state.fInX = TRUE;
        }
        else
            return -6;
    }

    // if (can >= 0)
    // {
    //     if (can == 0)
    //         c_cflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //     else if (can == 1)
    //         c_cflag |= (ICANON | ECHO | ECHOE | ISIG);
    //     else
    //         return -7;
    // }

    COMMTIMEOUTS timeouts = {0};
    COMMTIMEOUTS cto;
    GetCommTimeouts(port, &cto);
    if (timeout >= 0)
    {
        cto.ReadIntervalTimeout = timeout;
        cto.ReadTotalTimeoutConstant = 0;
        cto.ReadTotalTimeoutMultiplier = 0;
    }
    else
    {
        cto.ReadIntervalTimeout = MAXDWORD;
        cto.ReadTotalTimeoutConstant = 0;
        cto.ReadTotalTimeoutMultiplier = 0;
    }

    success = SetCommTimeouts(port, &cto);
    if (!success)
    {
        return 0;
    }
}

int write_serial(int fd, const void *data, size_t len)
{
    HANDLE port = get_port(fd);
    if (port == INVALID_HANDLE_VALUE)
        return -1;

    DWORD written;
    BOOL success = WriteFile(port, data, len, &written, NULL);
    if (!success)
    {
        return -1;
    }
    return written;
}

cube_native_var *read_serial(int fd, size_t len)
{
    HANDLE port = get_port(fd);

    cube_native_var *ret = NATIVE_NULL();
    unsigned char *data = malloc(sizeof(unsigned char) * len);
    DWORD received;
    BOOL success = ReadFile(port, data, len, &received, NULL);
    if (!success)
    {
        return ret;
    }
    if (received >= 0)
        TO_NATIVE_BYTES_ARG(ret, received, data);
    return ret;
}

// Utils
static void init_serial()
{
    for (int i = 0; i < MAX_PORTS; i++)
        handles[i] = NULL;
}

static int find_port(HANDLE port)
{
    if (port == INVALID_HANDLE_VALUE)
        return -1;

    for (int i = 0; i < MAX_PORTS; i++)
    {
        if (handles[i] == NULL)
            return i;
    }

    CloseHandle(port);

    return -1;
}

static HANDLE get_port(int fd)
{
    if (fd < 0 || fd >= MAX_PORTS)
        return INVALID_HANDLE_VALUE;
    return handles[fd];
}

static int add_port(HANDLE port)
{
    int fd = 0;
    for (int i = 0; i < MAX_PORTS; i++)
    {
        if (handles[i] == NULL)
        {
            handles[i] = port;
            return i;
        }
    }

    return -1;
}

static void remove_port(int fd)
{
    if (fd >= 0 && fd < MAX_PORTS)
        handles[fd] = NULL;
}

static int parse_serial_speed(int baudrate)
{
    int speed = 9600;
    switch (baudrate)
    {
        case 0:
            speed = 0;
            break;
        case 50:
            speed = 50;
            break;
        case 75:
            speed = 75;
            break;
        case 110:
            speed = 110;
            break;
        case 134:
            speed = 134;
            break;
        case 150:
            speed = 150;
            break;
        case 200:
            speed = 200;
            break;
        case 300:
            speed = 300;
            break;
        case 600:
            speed = 600;
            break;
        case 1200:
            speed = 1200;
            break;
        case 1800:
            speed = 1800;
            break;
        case 2400:
            speed = 2400;
            break;
        case 4800:
            speed = 4800;
            break;
        case 9600:
            speed = 9600;
            break;
        case 19200:
            speed = 1200;
            break;
        case 38400:
            speed = 38400;
            break;
        case 57600:
            speed = 57600;
            break;
        case 115200:
            speed = 115200;
            break;
        case 230400:
            speed = 230400;
            break;
        case 460800:
            speed = 460800;
            break;
        case 500000:
            speed = 500000;
            break;
        case 576000:
            speed = 576000;
            break;
        case 921600:
            speed = 921600;
            break;
        case 1000000:
            speed = 1000000;
            break;
        case 1152000:
            speed = 1152000;
            break;
        case 1500000:
            speed = 1500000;
            break;
        case 2000000:
            speed = 2000000;
            break;
        case 2500000:
            speed = 2500000;
            break;
        case 3000000:
            speed = 3000000;
            break;
        case 3500000:
            speed = 3500000;
            break;
        case 4000000:
            speed = 4000000;
            break;

        default:
            return -1;
            break;
    }
    return speed;
}
