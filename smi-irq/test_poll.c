#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

int main()
{
    int fd = open("/dev/smi_irq", O_RDONLY);

    if(fd < 0)
    {
        perror("Couldn't open /dev/smi_irq\n");
        return 1;
    }

    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN
    };

    printf("Waiting for interrupt ...\n");
    int count = 0;
    while(1)
    {
        int ret = poll(&pfd, 1, -1);
        if(ret > 0)
        {
            read(fd, NULL, 0);
            printf("%u Interrupt fired\n", count);
            count += 1;
        }
    }

    close(fd);

    return 0;
}