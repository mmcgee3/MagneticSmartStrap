#!/usr/bin/env python3
"""
ble_position_client.py

Companion app for the Saturn Sports strap firmware. Scans for the device,
connects, and subscribes to position updates pushed over the Nordic UART
Service (NUS) "TX" characteristic.

Wire format per update (8 bytes, little-endian):
    offset 0: float32  distance_mm
    offset 4: int32    counts

Install:
    pip install bleak

Run:
    python ble_position_client.py
    python ble_position_client.py --name SaturnStrap --timeout 10
"""

import argparse
import asyncio
import struct
import sys
from datetime import datetime

from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic

# Nordic UART Service UUIDs - must match ble_controller.c on the firmware side.
NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # device -> us (notify)
NUS_RX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  # us -> device (write)

POSITION_STRUCT = struct.Struct("<fi")  # float distance_mm, int32 counts


def on_position_notify(_: BleakGATTCharacteristic, data: bytearray) -> None:
    if len(data) != POSITION_STRUCT.size:
        print(f"[warn] unexpected payload length {len(data)}, ignoring")
        return

    distance_mm, counts = POSITION_STRUCT.unpack(data)
    ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print(f"[{ts}] {distance_mm:8.2f} mm  ({counts} counts)")


async def find_device(name: str, scan_timeout: float):
    print(f"Scanning for '{name}' (timeout={scan_timeout}s)...")
    device = await BleakScanner.find_device_by_filter(
        lambda d, adv: (d.name == name) or (NUS_SERVICE_UUID in (adv.service_uuids or [])),
        timeout=scan_timeout,
    )
    return device


async def run(name: str, scan_timeout: float):
    device = await find_device(name, scan_timeout)
    if device is None:
        print(f"Could not find device '{name}'. Is the firmware running and advertising?")
        sys.exit(1)

    print(f"Found {device.name} ({device.address}). Connecting...")

    async with BleakClient(device) as client:
        print("Connected. Subscribing to position notifications...")
        await client.start_notify(NUS_TX_CHAR_UUID, on_position_notify)

        print("Listening for position updates - press Ctrl+C to stop.\n")
        try:
            while client.is_connected:
                await asyncio.sleep(0.5)
        except asyncio.CancelledError:
            pass
        finally:
            try:
                await client.stop_notify(NUS_TX_CHAR_UUID)
            except Exception:
                pass

    print("Disconnected.")


def main():
    parser = argparse.ArgumentParser(description="Saturn Sports BLE position client")
    parser.add_argument("--name", default="SaturnStrap",
                         help="BLE advertised device name to look for (default: SaturnStrap)")
    parser.add_argument("--timeout", type=float, default=10.0,
                         help="Scan timeout in seconds (default: 10)")
    args = parser.parse_args()

    try:
        asyncio.run(run(args.name, args.timeout))
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()