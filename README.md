# Makima
**Status: 1.0β**

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
        printf("\n");
    }

    if (strcmp(content, "!test") == 0)
        ret = makima_say(m, channel, "aaaaa!") &&
              makima_react(m, channel, message, "annoyed:12345678912345678");

    return ret;
}

static bool
on_reaction(makima *m, bool added, uint64_t author, uint64_t message,
            uint64_t channel, uint64_t server, const char *emoji)
{
    bool ret = false;

    char  content[2001] = {0};
    char  name[32  + 1] = {0};
    char name2[100 + 1] = {0};
    char name3[100 + 1] = {0};
    char name4[100 + 1] = {0};
    char  avatar[128] = {0};
    char avatar2[128] = {0};
    char    icon[128] = {0};
    bool direct = false;
    uint64_t author2 = 0;

    ret = makima_author(m, author, server, name, avatar) &&
          makima_channel(m, channel, name2, &direct) &&
          makima_server(m, server, name3, icon) &&
          makima_message(m, channel, message, &author2, content) &&
          makima_author(m, author2, server, name4, avatar2);
    if (ret)
    {
        printf("%s (%s): reacted with '%s' at\n", name, avatar, emoji);
        printf("%s (%s): %s\n", name4, avatar, content);
        if (!direct)
            printf("On %s from %s (%s)\n", name2, name3, icon);
        else
            printf("On DMs\n", name2, name3, icon);
        printf("\n");
    }

    return ret;
}

int main(void)
{
    struct makima_cb cb = {.on_message  = on_message,
                           .on_reaction = on_reaction};
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
