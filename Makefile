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

all: build/libmakima.a build/makima
clean:
	rm -rf build

install: include/makima build/makima
	cp -r include/makima "$(DESTDIR)/usr/include/"
	cp build/libmakima.a "$(DESTDIR)/usr/local/lib/"
	cp build/makima      "$(DESTDIR)/usr/local/bin/"
uninstall:
	rm -rf "$(DESTDIR)/usr/include/makima/"
	rm -rf "$(DESTDIR)/usr/local/lib/libmakima.a"
	rm -rf "$(DESTDIR)/usr/local/bin/makima"

build/library.o: src/library.c | build
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

build/libmakima.a: build/library.o | build
	ar ruv $@ $^
	ranlib $@

build/makima: src/makima.c | build
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

build:
	mkdir -p build
