#!/usr/bin/env python3
"""Live 2D plot of the node1 ADC channels streamed over UART.

The firmware prints one line per conversion holding all four channels:

    ADC: 128 130  12 200

Any line containing exactly NUM_CHANNELS integers is accepted, so a plain
"128,130,12,200" works too.

Channel-to-axis assignment is set with --joystick / --touchpad and can also be
changed while the plot is running (see KEY_HELP).
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
import threading
import time
from collections import deque

import matplotlib.pyplot as plt
import serial
import serial.tools.list_ports
from matplotlib.animation import FuncAnimation

NUM_CHANNELS = 4
ADC_MAX = 255

INTEGERS = re.compile(r"\d+")

# Unparsable lines are expected right after a reset (half-printed line, noise),
# so report a few of them and then only the final count.
MAX_REPORTED_BAD_LINES = 20

KEY_HELP = (
    "keys:  j/J = cycle joystick X/Y channel   "
    "t/T = cycle touchpad X/Y channel   c = clear trails"
)


def parse_line(line: str) -> list[int] | None:
    """Return the channel values in `line`, or None if it is not a sample."""
    values = [int(token) for token in INTEGERS.findall(line)]
    if len(values) != NUM_CHANNELS:
        return None
    if any(value > ADC_MAX for value in values):
        return None
    return values


class SerialReader(threading.Thread):
    """Reads samples in the background so the GUI never blocks on the port."""

    def __init__(self, port: serial.Serial, sample_writer: csv.writer | None):
        super().__init__(daemon=True)
        self._port = port
        self._writer = sample_writer
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._latest: list[int] | None = None
        self._started_at = time.monotonic()
        self.bad_lines = 0

    def run(self) -> None:
        while not self._stop.is_set():
            raw = self._port.readline()
            if not raw:
                continue
            line = raw.decode("ascii", errors="replace").strip()
            if not line:
                continue
            sample = parse_line(line)
            if sample is None:
                self.bad_lines += 1
                if self.bad_lines <= MAX_REPORTED_BAD_LINES:
                    print(f"ignored line: {line!r}", file=sys.stderr)
                continue
            with self._lock:
                self._latest = sample
            if self._writer is not None:
                elapsed = time.monotonic() - self._started_at
                self._writer.writerow([f"{elapsed:.4f}", *sample])

    def latest(self) -> list[int] | None:
        with self._lock:
            return self._latest

    def stop(self) -> None:
        self._stop.set()


class Panel:
    """One square 0-255 plot fed by two ADC channels."""

    def __init__(self, ax, name: str, x_channel: int, y_channel: int, trail_length: int):
        self.ax = ax
        self.name = name
        self.x_channel = x_channel
        self.y_channel = y_channel
        self.trail: deque[tuple[int, int]] = deque(maxlen=trail_length)

        ax.set_xlim(0, ADC_MAX)
        ax.set_ylim(0, ADC_MAX)
        ax.set_aspect("equal")
        ax.set_xticks([0, 64, 128, 192, 255])
        ax.set_yticks([0, 64, 128, 192, 255])
        ax.grid(alpha=0.3)
        # Centre lines make the joystick's rest position easy to judge.
        ax.axhline(ADC_MAX / 2, color="0.6", lw=0.8)
        ax.axvline(ADC_MAX / 2, color="0.6", lw=0.8)

        (self.trail_line,) = ax.plot([], [], "-", lw=1, alpha=0.5)
        (self.point,) = ax.plot([], [], "o", ms=12, color="tab:red")
        self.readout = ax.text(
            0.5, -0.14, "", transform=ax.transAxes, ha="center", va="top", fontsize=10
        )
        self._update_title()

    def _update_title(self) -> None:
        self.ax.set_title(f"{self.name}   X=ch{self.x_channel}  Y=ch{self.y_channel}")

    def cycle_channel(self, axis: str) -> None:
        if axis == "x":
            self.x_channel = (self.x_channel + 1) % NUM_CHANNELS
        else:
            self.y_channel = (self.y_channel + 1) % NUM_CHANNELS
        self.trail.clear()
        self._update_title()

    def clear(self) -> None:
        self.trail.clear()

    def draw(self, sample: list[int]) -> None:
        x = sample[self.x_channel]
        y = sample[self.y_channel]
        self.trail.append((x, y))
        self.trail_line.set_data(*zip(*self.trail))
        self.point.set_data([x], [y])
        self.readout.set_text(f"X={x:3d}  Y={y:3d}")

    def artists(self) -> tuple:
        return (self.trail_line, self.point, self.readout)


def channel_pair(text: str) -> tuple[int, int]:
    parts = text.split(",")
    if len(parts) != 2:
        raise argparse.ArgumentTypeError(f"expected 'x,y' channel pair, got {text!r}")
    pair = tuple(int(part) for part in parts)
    for channel in pair:
        if not 0 <= channel < NUM_CHANNELS:
            raise argparse.ArgumentTypeError(
                f"channel {channel} out of range 0-{NUM_CHANNELS - 1}"
            )
    return pair


def autodetect_port() -> str:
    candidates = [
        info.device
        for info in serial.tools.list_ports.comports()
        if any(tag in info.device for tag in ("usbmodem", "usbserial", "ttyUSB", "ttyACM"))
    ]
    if not candidates:
        raise SystemExit("no USB serial port found, pass --port")
    if len(candidates) > 1:
        raise SystemExit(f"several USB serial ports found, pass --port: {candidates}")
    return candidates[0]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", help="serial port (autodetected when omitted)")
    parser.add_argument("--baud", type=int, default=9600, help="must match uart_init()")
    parser.add_argument("--joystick", type=channel_pair, default=(0, 1),
                        metavar="X,Y", help="ADC channels driving the joystick panel")
    parser.add_argument("--touchpad", type=channel_pair, default=(2, 3),
                        metavar="X,Y", help="ADC channels driving the touchpad panel")
    parser.add_argument("--trail", type=int, default=60,
                        help="number of past samples drawn as a trail")
    parser.add_argument("--interval", type=int, default=33,
                        help="redraw period in milliseconds")
    parser.add_argument("--csv", help="also log every sample to this file")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    port_name = args.port or autodetect_port()

    csv_file = open(args.csv, "w", newline="") if args.csv else None
    writer = None
    if csv_file is not None:
        writer = csv.writer(csv_file)
        writer.writerow(["seconds", *(f"ch{i}" for i in range(NUM_CHANNELS))])

    # A timeout keeps readline() returning so the reader can be stopped.
    port = serial.Serial(port_name, args.baud, timeout=1)
    print(f"reading {port_name} at {args.baud} baud")
    print(KEY_HELP)

    reader = SerialReader(port, writer)
    reader.start()

    fig, axes = plt.subplots(1, 2, figsize=(11, 5.5))
    fig.canvas.manager.set_window_title("node1 ADC")
    panels = {
        "joystick": Panel(axes[0], "joystick", *args.joystick, args.trail),
        "touchpad": Panel(axes[1], "touchpad", *args.touchpad, args.trail),
    }
    fig.subplots_adjust(bottom=0.2)
    fig.text(0.5, 0.04, KEY_HELP, ha="center", fontsize=8, color="0.4")

    key_actions = {
        "j": ("joystick", "x"),
        "J": ("joystick", "y"),
        "t": ("touchpad", "x"),
        "T": ("touchpad", "y"),
    }

    def on_key(event):
        if event.key == "c":
            for panel in panels.values():
                panel.clear()
            return
        action = key_actions.get(event.key)
        if action is not None:
            name, axis = action
            panels[name].cycle_channel(axis)

    fig.canvas.mpl_connect("key_press_event", on_key)

    def update(_frame):
        sample = reader.latest()
        if sample is not None:
            for panel in panels.values():
                panel.draw(sample)
        return [artist for panel in panels.values() for artist in panel.artists()]

    # Kept in a name so the animation is not garbage collected while running.
    animation = FuncAnimation(fig, update, interval=args.interval, blit=False,
                              cache_frame_data=False)
    try:
        plt.show()
    finally:
        reader.stop()
        reader.join(timeout=2)
        port.close()
        if csv_file is not None:
            csv_file.close()
            print(f"wrote {args.csv}")
        if reader.bad_lines:
            print(f"{reader.bad_lines} unparsable line(s) ignored", file=sys.stderr)
        del animation


if __name__ == "__main__":
    main()
