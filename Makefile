# This file is part of makima.

# Makima is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 3.

# Makima is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with makima. If not, see <https://www.gnu.org/licenses/>.

CFLAGS += -Iinclude -Og -ggdb3 -Wall -Wextra
LDFLAGS += -L/usr/local/lib -lcurl -ljson-c

.PHONY: all clean

all: build/libmakima.a
clean:
	rm -rf build

install: include/makima build/libmakima.a
	cp -r include/makima "$(DESTDIR)/usr/include/"
	cp build/libmakima.a "$(DESTDIR)/usr/local/lib/"
uninstall:
	rm -rf "$(DESTDIR)/usr/include/makima/"
	rm -rf "$(DESTDIR)/usr/local/lib/libmakima.a"

build/libmakima.a: build/gateway.o | build
	ar ruv $@ $^
	ranlib $@

build/gateway.o: src/gateway.c | build
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

build:
	mkdir -p build
