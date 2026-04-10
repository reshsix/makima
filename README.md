# Makima
***Status: 1.0α***

## Requirements
- A POSIX environment
- libcurl (Compiled with --enable-websockets)
- libjson-c

## Example
```c
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <makima/makima.h>

static bool
on_message(const char *name, const char *content)
{
    printf("%s: %s\n", name, content);
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
