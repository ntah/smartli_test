#!/usr/bin/env python3
"""Parse SmartLi binary frames captured by ESPHome's uart_debug logger."""

from __future__ import annotations

import argparse
import ast
import json
import re
import sys
from pathlib import Path
from typing import Any


UART_LOG_PATTERN = re.compile(r"\[uart_debug:[^\]]+\]:\s+<<<\s+(\".*\")\s*$")

# Number of bytes used by each value in the observed command 0x01 response.
FIELD_WIDTHS = {
    0x01: 2,
    0x02: 2,
    0x03: 2,
    0x04: 2,
    0x05: 2,
    0x06: 2,
    0x07: 2,
    0x08: 2,
    0x09: 2,
    0x0A: 2,
    0x0B: 4,
    0x0C: 4,
    0x0D: 4,
    0x0E: 4,
    0x0F: 4,
    0x10: 4,
    0x11: 2,
    0x12: 2,
}


def decode_uart_segments(text: str) -> list[bytes]:
    """Decode the quoted C-style byte strings emitted by uart_debug."""
    segments: list[bytes] = []

    for line in text.splitlines():
        match = UART_LOG_PATTERN.search(line)
        if not match:
            continue

        try:
            decoded = ast.literal_eval(match.group(1))
        except (SyntaxError, ValueError) as exc:
            raise ValueError(f"Unable to decode uart_debug line: {line}") from exc

        segments.append(decoded.encode("latin-1"))

    return segments


def extract_frames(segments: list[bytes]) -> tuple[list[bytes], bytes]:
    """
    Reassemble frames split by uart_debug.

    Observed frame:
      0x7E | address | command | payload length | payload | check | 0x0D
    """
    buffer = b"".join(segments)
    frames: list[bytes] = []

    while buffer:
        start = buffer.find(b"\x7e")
        if start < 0:
            return frames, b""

        buffer = buffer[start:]
        if len(buffer) < 4:
            break

        # Some tools on the same bus use an ASCII frame beginning with "~1".
        # Unlike the binary SmartLi frame, it is terminated by CR and its
        # fourth byte is an ASCII character rather than a binary length.
        is_ascii = all(
            byte in b"0123456789ABCDEFabcdef" for byte in buffer[1:4]
        )
        if is_ascii:
            terminator = buffer.find(b"\x0d", 4)
            if terminator < 0:
                break
            frame_length = terminator + 1
        else:
            frame_length = 6 + buffer[3]

        if len(buffer) < frame_length:
            break

        frame = buffer[:frame_length]
        buffer = buffer[frame_length:]

        if frame[-1] != 0x0D:
            raise ValueError(
                f"Invalid frame terminator 0x{frame[-1]:02X}: {frame.hex(' ')}"
            )

        frames.append(frame)

    return frames, buffer


def read_unsigned(data: bytes) -> int:
    return int.from_bytes(data, byteorder="big", signed=False)


def parse_tlv_payload(payload: bytes) -> dict[str, Any]:
    fields: dict[str, Any] = {}
    offset = 0

    while offset < len(payload):
        if offset + 2 > len(payload):
            raise ValueError(f"Incomplete TLV header at payload offset {offset}")

        field_id = payload[offset]
        count = payload[offset + 1]
        offset += 2

        width = FIELD_WIDTHS.get(field_id)
        if width is None:
            raise ValueError(
                f"Unknown field 0x{field_id:02X} at payload offset {offset - 2}"
            )

        data_length = count * width
        if offset + data_length > len(payload):
            raise ValueError(
                f"Field 0x{field_id:02X} exceeds payload: "
                f"need {data_length} bytes, have {len(payload) - offset}"
            )

        values = [
            read_unsigned(payload[position : position + width])
            for position in range(offset, offset + data_length, width)
        ]
        offset += data_length

        fields[f"0x{field_id:02X}"] = values

    return fields


def make_read_request(address: int, command: int = 0x01) -> bytes:
    """Build the observed empty-payload read request."""
    check = (-(address + command)) & 0xFF
    return bytes((0x7E, address, command, 0x00, check, 0x0D))


def parse_frame(frame: bytes) -> dict[str, Any]:
    if all(byte in b"0123456789ABCDEFabcdef" for byte in frame[1:4]):
        return {
            "kind": "ascii",
            "text": frame[1:-1].decode("ascii", errors="replace"),
            "raw_hex": frame.hex(" "),
        }

    address = frame[1]
    command = frame[2]
    payload_length = frame[3]
    payload = frame[4 : 4 + payload_length]
    check = frame[-2]

    result: dict[str, Any] = {
        "address": address,
        "command": command,
        "payload_length": payload_length,
        "check": check,
        "raw_hex": frame.hex(" "),
    }

    if command == 0xDC and payload == b"\x06\x00\x00":
        result["check_valid"] = None
        result["kind"] = "request"
        result["payload_hex"] = payload.hex(" ")
        return result

    if payload_length == 0:
        expected = (-(address + command)) & 0xFF
        result["check_valid"] = check == expected
        result["kind"] = "request"
        return result

    result["kind"] = "response"

    if command != 0x01:
        result["payload_hex"] = payload.hex(" ")
        result["payload_ascii"] = "".join(
            chr(byte) if 32 <= byte <= 126 else "."
            for byte in payload
        )
        if command == 0x45 and payload_length == 6:
            result["decoded"] = {
                "device_datetime": (
                    f"20{payload[0]:02d}-{payload[1]:02d}-{payload[2]:02d} "
                    f"{payload[3]:02d}:{payload[4]:02d}:{payload[5]:02d}"
                )
            }
        elif command == 0x42:
            result["decoded"] = {
                "device_identifier": payload.decode("ascii", errors="replace").strip()
            }
        elif command == 0x33:
            result["decoded"] = {
                "model": payload.decode("ascii", errors="replace").strip()
            }
        elif command == 0xDC:
            printable = re.findall(rb"[\x20-\x7e]{4,}", payload)
            result["decoded"] = {
                "identity_strings": [
                    item.decode("ascii", errors="replace").strip(" ^")
                    for item in printable
                    if item.strip(b" ^")
                ]
            }
        result["check_valid"] = None
        return result

    result["fields"] = parse_tlv_payload(payload)

    cells = result["fields"].get("0x01", [])
    current_raw = result["fields"].get("0x02", [None])[0]
    remaining_raw = result["fields"].get("0x03", [None])[0]
    full_raw = result["fields"].get("0x04", [None])[0]
    pack_voltage_raw = result["fields"].get("0x08", [None])[0]
    soh_raw = result["fields"].get("0x09", [None])[0]
    total_charged_raw = result["fields"].get("0x0B", [None])[0]
    total_discharged_raw = result["fields"].get("0x0C", [None])[0]
    bus_voltage_raw = result["fields"].get("0x11", [None])[0]
    decoded: dict[str, Any] = {}

    if cells:
        decoded.update(
            {
                "cell_voltage_v": [round(value / 1000, 3) for value in cells],
                "cell_min_v": round(min(cells) / 1000, 3),
                "cell_max_v": round(max(cells) / 1000, 3),
                "cell_delta_v": round((max(cells) - min(cells)) / 1000, 3),
            }
        )
    if current_raw is not None:
        decoded["current_a"] = round((current_raw - 30000) / 100, 2)
    if remaining_raw is not None:
        decoded["remaining_capacity_ah"] = round(remaining_raw / 100, 2)
    if full_raw is not None:
        decoded["full_capacity_ah"] = round(full_raw / 100, 2)
    if remaining_raw is not None and full_raw:
        decoded["state_of_charge_percent"] = round(remaining_raw / full_raw * 100, 2)
    if pack_voltage_raw is not None:
        decoded["pack_voltage_v"] = round(pack_voltage_raw / 100, 2)
    if soh_raw is not None:
        decoded["state_of_health_percent"] = round(soh_raw / 100, 2)
    if total_charged_raw is not None:
        decoded["total_charged_ah"] = total_charged_raw
    if total_discharged_raw is not None:
        decoded["total_discharged_ah"] = total_discharged_raw
    if bus_voltage_raw is not None:
        decoded["bus_voltage_v"] = round(bus_voltage_raw / 100, 2)

    result["decoded"] = decoded

    # The response check algorithm still needs additional protocol evidence.
    result["check_valid"] = None
    return result


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Parse SmartLi frames from an ESPHome uart_debug log."
    )
    parser.add_argument("log", type=Path, help="Path to the ESPHome text log")
    parser.add_argument(
        "--responses-only",
        action="store_true",
        help="Do not print the repeated read requests",
    )
    args = parser.parse_args()

    try:
        text = args.log.read_text(encoding="utf-8", errors="replace")
        segments = decode_uart_segments(text)
        frames, remainder = extract_frames(segments)
        parsed = [parse_frame(frame) for frame in frames]
    except (OSError, ValueError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    if args.responses_only:
        parsed = [item for item in parsed if item["kind"] == "response"]

    output = {
        "uart_segments": len(segments),
        "frames": len(parsed),
        "unparsed_bytes": remainder.hex(" "),
        "data": parsed,
    }
    print(json.dumps(output, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
