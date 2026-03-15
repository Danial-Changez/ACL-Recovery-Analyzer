"""
BLE Live Plotter for ACL-Rehab device.

Connects to the "ACL-Rehab" BLE device, subscribes to the live data
characteristic, decodes RehabPacket, and plots all Euler angles in
real time.

Requirements:
    pip install bleak matplotlib

Usage:
    python tools/ble_plot.py
"""

import asyncio
import struct
import sys
import threading
from collections import deque

import matplotlib.pyplot as plt
import matplotlib.animation as animation
from bleak import BleakClient, BleakScanner

DEVICE_NAME = "ACL-Rehab"
SERVICE_UUID = "13953057-0959-4d32-ab62-362a7ffb6aab"
LIVE_DATA_UUID = "13953057-0959-4d32-ab62-362a7ffb6aac"

# kneeAngle, thighPitch, thighRoll, thighHeading,
# shankPitch, shankRoll, shankHeading, linearAccelZ,
# stepCount, repCount, exerciseType, calibStatus, batteryPercent, flags
PACKET_FMT = "<hhhhhhhhHBBBBB"
PACKET_SIZE = struct.calcsize(PACKET_FMT)

EXERCISE_NAMES = [
    "Seated Knee Ext",
    "Hip Abduction",
    "Single-Leg Squat",
    "Leg Bridging",
    "Reverse Lunge",
]

MAX_POINTS = 500

knee_data = deque(maxlen=MAX_POINTS)
thigh_pitch_data = deque(maxlen=MAX_POINTS)
thigh_roll_data = deque(maxlen=MAX_POINTS)
thigh_heading_data = deque(maxlen=MAX_POINTS)
shank_pitch_data = deque(maxlen=MAX_POINTS)
shank_roll_data = deque(maxlen=MAX_POINTS)
shank_heading_data = deque(maxlen=MAX_POINTS)
accel_data = deque(maxlen=MAX_POINTS)
rep_data = deque(maxlen=MAX_POINTS)
time_data = deque(maxlen=MAX_POINTS)

sample_idx = 0
connected = False
stop_event = threading.Event()
current_exercise = ""
current_reps = 0


def on_notify(_handle, data: bytearray):
    global sample_idx, current_exercise, current_reps

    if len(data) < PACKET_SIZE:
        return

    (knee, t_pitch, t_roll, t_heading,
     s_pitch, s_roll, s_heading, accel_z,
     steps, reps, ex_type, calib, batt, flags) = struct.unpack(
        PACKET_FMT, data[:PACKET_SIZE]
    )

    knee_angle = knee / 100.0
    tp = t_pitch / 100.0
    tr = t_roll / 100.0
    th = t_heading / 100.0
    sp = s_pitch / 100.0
    sr = s_roll / 100.0
    sh = s_heading / 100.0
    lin_accel_z = accel_z / 100.0

    ex_name = EXERCISE_NAMES[ex_type] if ex_type < len(EXERCISE_NAMES) else f"Unknown({ex_type})"
    current_exercise = ex_name
    current_reps = reps

    time_data.append(sample_idx)
    knee_data.append(knee_angle)
    thigh_pitch_data.append(tp)
    thigh_roll_data.append(tr)
    thigh_heading_data.append(th)
    shank_pitch_data.append(sp)
    shank_roll_data.append(sr)
    shank_heading_data.append(sh)
    accel_data.append(lin_accel_z)
    rep_data.append(reps)
    sample_idx += 1

    sys.stdout.write(
        f"\r[{ex_name}] knee={knee_angle:7.2f}  "
        f"thigh=({tp:6.1f}, {tr:6.1f}, {th:6.1f})  "
        f"shank=({sp:6.1f}, {sr:6.1f}, {sh:6.1f})  "
        f"reps={reps}  batt={batt}%  #{sample_idx}"
    )
    sys.stdout.flush()


async def ble_loop():
    global connected

    print(f"Scanning for '{DEVICE_NAME}'...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10)

    if device is None:
        print("Name scan failed, trying service UUID scan...")
        device = await BleakScanner.find_device_by_filter(
            lambda d, adv: SERVICE_UUID in (adv.service_uuids or []),
            timeout=10,
        )

    if device is None:
        print("Could not find device. Make sure the ESP32 is powered on.")
        sys.exit(1)

    print(f"Found {device.name} [{device.address}], connecting...")

    async with BleakClient(device) as client:
        print(f"Connected! MTU={client.mtu_size}")
        connected = True

        await client.start_notify(LIVE_DATA_UUID, on_notify)
        print("Receiving data... Close the plot window to stop.\n")

        while not stop_event.is_set():
            await asyncio.sleep(0.1)

        await client.stop_notify(LIVE_DATA_UUID)

    connected = False
    print("\nBLE disconnected.")


def run_ble():
    asyncio.run(ble_loop())


def main():
    ble_thread = threading.Thread(target=run_ble, daemon=True)
    ble_thread.start()

    fig, axes = plt.subplots(4, 1, figsize=(12, 9), sharex=True)
    title_text = fig.suptitle("ACL-Rehab BLE Live Plot")
    ax_knee, ax_thigh, ax_shank, ax_accel = axes

    # Knee angle
    (line_knee,) = ax_knee.plot([], [], "b-", linewidth=1.5, label="Knee Angle")
    ax_knee.set_ylabel("Degrees")
    ax_knee.legend(loc="upper right")
    ax_knee.grid(True, alpha=0.3)

    # Thigh Euler angles
    (line_tp,) = ax_thigh.plot([], [], "r-", linewidth=1.2, label="Pitch")
    (line_tr,) = ax_thigh.plot([], [], "g-", linewidth=1, alpha=0.7, label="Roll")
    (line_th,) = ax_thigh.plot([], [], "b-", linewidth=1, alpha=0.5, label="Heading")
    ax_thigh.set_ylabel("Thigh (\u00b0)")
    ax_thigh.legend(loc="upper right")
    ax_thigh.grid(True, alpha=0.3)

    # Shank Euler angles
    (line_sp,) = ax_shank.plot([], [], "r-", linewidth=1.2, label="Pitch")
    (line_sr,) = ax_shank.plot([], [], "g-", linewidth=1, alpha=0.7, label="Roll")
    (line_sh,) = ax_shank.plot([], [], "b-", linewidth=1, alpha=0.5, label="Heading")
    ax_shank.set_ylabel("Shank (\u00b0)")
    ax_shank.legend(loc="upper right")
    ax_shank.grid(True, alpha=0.3)

    # Accel + reps
    (line_accel,) = ax_accel.plot([], [], "g-", linewidth=1, label="Linear Accel Z")
    ax_accel_twin = ax_accel.twinx()
    (line_reps,) = ax_accel_twin.plot([], [], "m-", linewidth=2, alpha=0.6, label="Reps")
    ax_accel.set_ylabel("m/s\u00b2")
    ax_accel_twin.set_ylabel("Reps")
    ax_accel.set_xlabel("Sample")
    ax_accel.legend(loc="upper left")
    ax_accel_twin.legend(loc="upper right")
    ax_accel.grid(True, alpha=0.3)

    plt.tight_layout()

    all_lines = [line_knee, line_tp, line_tr, line_th,
                 line_sp, line_sr, line_sh, line_accel, line_reps]

    def update(_frame):
        if len(time_data) < 2:
            return all_lines

        t = list(time_data)

        title_text.set_text(
            f"ACL-Rehab  |  {current_exercise}  |  Reps: {current_reps}"
        )

        line_knee.set_data(t, list(knee_data))
        line_tp.set_data(t, list(thigh_pitch_data))
        line_tr.set_data(t, list(thigh_roll_data))
        line_th.set_data(t, list(thigh_heading_data))
        line_sp.set_data(t, list(shank_pitch_data))
        line_sr.set_data(t, list(shank_roll_data))
        line_sh.set_data(t, list(shank_heading_data))
        line_accel.set_data(t, list(accel_data))
        line_reps.set_data(t, list(rep_data))

        for ax in axes:
            ax.set_xlim(t[0], t[-1])
            ax.relim()
            ax.autoscale_view(scalex=False)
        ax_accel_twin.relim()
        ax_accel_twin.autoscale_view(scalex=False)

        return all_lines

    ani = animation.FuncAnimation(
        fig, update, interval=200, blit=False, cache_frame_data=False
    )

    plt.show()
    stop_event.set()


if __name__ == "__main__":
    main()
