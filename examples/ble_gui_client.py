#!/usr/bin/env python3
"""
Simple GUI BLE position client for the Saturn Sports strap.

Requires:
    pip install bleak

The GUI shows the latest position received from the device and provides
Connect / Disconnect controls.
"""

import asyncio
import struct
import threading
import tkinter as tk
from tkinter import ttk

from bleak import BleakClient, BleakScanner

NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

POSITION_STRUCT = struct.Struct("<fi")
DEVICE_NAME = "SaturnStrap"


class BLEPositionGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Saturn Strap")
        self.root.geometry("360x230")
        self.root.resizable(False, False)

        self.loop = asyncio.new_event_loop()
        self.thread = threading.Thread(target=self._run_loop, daemon=True)
        self.thread.start()

        self.client = None
        self.device = None
        self.connected = False

        self.position_var = tk.StringVar(value="-- mm")
        self.counts_var = tk.StringVar(value="-- counts")
        self.status_var = tk.StringVar(value="Disconnected")

        frame = ttk.Frame(root, padding=20)
        frame.pack(fill="both", expand=True)

        ttk.Label(frame, text="Current Position").pack()

        ttk.Label(
            frame,
            textvariable=self.position_var,
            font=("TkDefaultFont", 28, "bold"),
        ).pack(pady=(8, 0))

        ttk.Label(
            frame,
            textvariable=self.counts_var,
            font=("TkDefaultFont", 11),
        ).pack(pady=(0, 12))

        ttk.Label(frame, textvariable=self.status_var).pack(pady=(0, 12))

        buttons = ttk.Frame(frame)
        buttons.pack()

        self.connect_button = ttk.Button(
            buttons, text="Connect", command=self.connect
        )
        self.connect_button.pack(side="left", padx=5)

        self.disconnect_button = ttk.Button(
            buttons, text="Disconnect", command=self.disconnect, state="disabled"
        )
        self.disconnect_button.pack(side="left", padx=5)

        self.root.protocol("WM_DELETE_WINDOW", self.close)

    def _run_loop(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def _submit(self, coroutine):
        return asyncio.run_coroutine_threadsafe(coroutine, self.loop)

    async def _find_device(self):
        self._set_status("Scanning...")
        return await BleakScanner.find_device_by_filter(
            lambda d, adv: (
                d.name == DEVICE_NAME
                or NUS_SERVICE_UUID in (adv.service_uuids or [])
            ),
            timeout=10.0,
        )

    async def _connect(self):
        try:
            device = await self._find_device()

            if device is None:
                self._set_status("Device not found")
                self._set_buttons(False)
                return

            self.device = device
            self._set_status("Connecting...")

            self.client = BleakClient(device)
            await self.client.connect()

            await self.client.start_notify(
                NUS_TX_CHAR_UUID,
                self._on_position_notify,
            )

            self.connected = True
            self._set_status("Connected")
            self._set_buttons(True)

            while self.client.is_connected:
                await asyncio.sleep(0.25)

        except Exception as exc:
            self._set_status(f"Connection failed: {exc}")
        finally:
            self.connected = False
            self._set_buttons(False)
            if self.client is not None:
                try:
                    if self.client.is_connected:
                        await self.client.stop_notify(NUS_TX_CHAR_UUID)
                        await self.client.disconnect()
                except Exception:
                    pass
            self.client = None

            if self.status_var.get() not in ("Device not found",):
                self._set_status("Disconnected")

    def _on_position_notify(self, _, data):
        if len(data) != POSITION_STRUCT.size:
            return

        distance_mm, counts = POSITION_STRUCT.unpack(data)

        self.root.after(
            0,
            lambda: self._update_position(distance_mm, counts),
        )

    def _update_position(self, distance_mm, counts):
        self.position_var.set(f"{distance_mm:.2f} mm")
        self.counts_var.set(f"{counts} counts")

    def connect(self):
        if not self.connected:
            self.connect_button.config(state="disabled")
            self._submit(self._connect())

    def disconnect(self):
        if self.client is not None:
            self._submit(self._disconnect())

    async def _disconnect(self):
        try:
            if self.client and self.client.is_connected:
                await self.client.stop_notify(NUS_TX_CHAR_UUID)
                await self.client.disconnect()
        except Exception:
            pass

    def _set_status(self, text):
        self.root.after(0, lambda: self.status_var.set(text))

    def _set_buttons(self, connected):
        def update():
            if connected:
                self.connect_button.config(state="disabled")
                self.disconnect_button.config(state="normal")
            else:
                self.connect_button.config(state="normal")
                self.disconnect_button.config(state="disabled")

        self.root.after(0, update)

    def close(self):
        if self.client is not None:
            try:
                self._submit(self._disconnect()).result(timeout=2)
            except Exception:
                pass

        self.loop.call_soon_threadsafe(self.loop.stop)
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = BLEPositionGUI(root)
    root.mainloop()

