# PlayOS examples

Compile-only validation (headers only, no link):

```sh
make check
```

Link examples against a real `libplayos` build:

```sh
gcc -I../include 01_hello_system.c -o hello_system -lplayos
```

| File | Demonstrates |
|---|---|
| `01_hello_system.c` | system info + logging |
| `02_input_and_lifecycle.c` | lifecycle poll + controller state |
| `03_audio_and_display.c` | audio state/volume + display info/brightness |
| `04_storage.c` | per-game paths + atomic save |
| `05_power.c` | power/thermal info + performance profile |
| `minimal_game.c` | the getting-started minimal game loop |
