# makima
A discord library written in C for POSIX

## Requirements
- A POSIX environment
- libcurl (Compiled with --enable-websockets)
- libjson-c

## Example
```c
#include <unistd.h>
#include <makima/makima.h>

int main(void)
{
    // Token, intents, shard number, shards amount, input fd, output fd
    int ret = makima_gateway("MyToken", (1 << 0) + (1 << 9), 0, 1,
                             STDIN_FILENO, STDOUT_FILENO);
    return ret;
}
```
```sh
make
sudo make install
gcc test.c -L/usr/local/lib -lmakima -lcurl -ljson-c
```

## Progress
- [x] Gateway
- [ ] REST
- [ ] Voice
- [ ] Commands
- [ ] Docs
