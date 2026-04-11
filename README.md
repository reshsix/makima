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

    char  name[32  + 1] = {0};
    char name2[100 + 1] = {0};
    char name3[100 + 1] = {0};
    char avatar[128] = {0};
    char   icon[128] = {0};
    bool direct = false;

    ret = makima_author(m, author, server, name, avatar) &&
          makima_channel(m, channel, name2, &direct) &&
          makima_server(m, server, name3, icon);
    if (ret)
    {
        printf("%s (%s): %s\n", name, avatar, content);
        if (!direct)
            printf("On %s from %s (%s)\n", name2, name3, icon);
        else
            printf("On DMs\n", name2, name3, icon);
    }

    if (strcmp(content, "!test") == 0)
        ret = makima_say(m, channel, "aaaaa!") &&
              makima_react(m, channel, message, "annoyed:12345678912345678");

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
