# Makima
**Status: 1.0α**

## Requirements
- A POSIX environment
- libcurl (Compiled with --enable-websockets)
- libjson-c

## Example
```c
#include <stdio.h>
#include <unistd.h>
#include <makima/makima.h>

static bool
on_message(const char *content,
           uint64_t author, uint64_t channel, uint64_t server)
{
    printf("%ld:%ld:%ld: %s\n", author, channel, server, content);
    return true;
}

int main(void)
{
    char *token = "TOKEN";
    int ret = makima_run(token, on_message);
    return ret;
}
```

```sh
make
sudo make install
gcc test.c -L/usr/local/lib -lmakima -lcurl -ljson-c
```
