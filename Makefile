CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
PROGS = file_demo recursive_ls typing process_signal pipe_ipc pipe_chat mq_chat mmap_copy minishell
all: $(PROGS)
file_demo: src/file_demo.c
	$(CC) $(CFLAGS) $< -o $@
recursive_ls: src/recursive_ls.c
	$(CC) $(CFLAGS) $< -o $@
typing: src/typing.c
	$(CC) $(CFLAGS) $< -o $@
process_signal: src/process_signal.c
	$(CC) $(CFLAGS) $< -o $@
pipe_ipc: src/pipe_ipc.c
	$(CC) $(CFLAGS) $< -o $@
pipe_chat: src/pipe_chat.c
	$(CC) $(CFLAGS) $< -o $@
mq_chat: src/mq_chat.c
	$(CC) $(CFLAGS) $< -o $@ -lrt
mmap_copy: src/mmap_copy.c
	$(CC) $(CFLAGS) $< -o $@
minishell: src/minishell.c src/builtin.c src/builtin.h
	$(CC) $(CFLAGS) src/minishell.c src/builtin.c -o $@
rust:
	cd matrix && cargo build
rust-run:
	cd matrix && cargo run
dbus:
	$(CC) $(CFLAGS) src/dbus_server.c -o dbus_server $$(pkg-config --cflags --libs gio-2.0)
	$(CC) $(CFLAGS) src/dbus_client.c -o dbus_client $$(pkg-config --cflags --libs gio-2.0)
clean:
	rm -f $(PROGS) dbus_server dbus_client
	cd matrix && cargo clean
.PHONY: all rust rust-run dbus clean
