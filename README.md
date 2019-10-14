### Infinitton iDisplay driver
This is a Linux driver/library for interacting with the [Infinitton iDisplay](https://www.infinitton.com/).

It includes functionality for writing to the displays inside the buttons, as well as reading input for determining the press state of each button.

### Building
This project includes:
    - **libinfinitton** The library for creating applications that drive the display and read buttons.
    - **infctl** a command line program for controlling various things, but mostly for running tests.
    - **examples** Example projects using libinfinitton

To build everything, use meson & ninja:
```
meson build
ninja -C build
```

### Examples
#### Pomodoro
A pomodoro timer. Pass in the number of minutes you want to start the timer as the first argument.

