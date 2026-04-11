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
on_message(makima *m, uint64_t author, uint64_t channel, uint64_t server,
           const char *content)
{
    bool ret = false;

    char name[64]    = {0};
    char avatar[128] = {0};
    ret = makima_author(m, author, server, name, avatar);
    if (ret)
        printf("%s (%s): %s\n", name, avatar, content);

    return ret;
}

int main(void)
{
    struct makima_cb cb = {.on_message = on_message};
    makima *m = makima_new(NULL, cb);
    while (makima_next(m));
    makima_del(m);
    return 0;
}
```

```sh
make
sudo make install
gcc test.c -lmakima
makima TOKEN | ./a.out
```
