# Makima
**Status: 1.0α**

## Requirements
- A POSIX environment
- libcurl (Compiled with --enable-websockets)
- libjson-c

## Example
```c
#include <stdio.h>
#include <makima/makima.h>

static bool
on_message(struct makima *m,
           uint64_t author, uint64_t channel, uint64_t server,
           const char *content)
{
    printf("%ld:%ld:%ld: %s\n", author, channel, server, content);
    return true;
}

int main(void)
{
    struct makima m = {.on_message = on_message};
    while (makima_next(&m));
    return 0;
}
```

```sh
make
sudo make install
gcc test.c -lmakima
makima TOKEN | ./a.out
```
