"""
Data Augmentation Script for SOME/IP VHAL Log Parsing Dataset
=============================================================
Generates 500+ synthetic training examples following the same format
as the real training data. Produces both RX and TX log line variants.

Usage:
    python data/augment_data.py

Output:
    Appends generated examples to data/train_data.jsonl
"""

import json
import random
import os

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
NUM_EXAMPLES = 500          # Number of synthetic examples to generate
OUTPUT_FILE = os.path.join(os.path.dirname(__file__), "train_data.jsonl")

# Instruction text (same for every example)
INSTRUCTION = (
    "You are a senior Vehicle Hardware Abstraction Layer (VHAL) developer in "
    "Skylark project. Parse this SOME/IP log line from the vehicle service. "
    "Extract the timestamp, transmission direction (RX/TX), serviceId "
    "(converting hex to decimal for RX; keeping decimal for TX), methodId "
    "(converting hex to decimal for RX; keeping decimal for TX), and payload. "
    "Output the parsed fields strictly as a validated JSON object matching "
    "the VHAL signal schema."
)

# ---------------------------------------------------------------------------
# Random generators
# ---------------------------------------------------------------------------

def random_timestamp():
    """Generate a realistic Android logcat timestamp: MM-DD HH:MM:SS.mmm"""
    month = random.randint(1, 12)
    day = random.randint(1, 28)
    hour = random.randint(0, 23)
    minute = random.randint(0, 59)
    second = random.randint(0, 59)
    millis = random.randint(0, 999)
    return f"{month:02d}-{day:02d} {hour:02d}:{minute:02d}:{second:02d}.{millis:03d}"


def random_pid():
    """Generate a realistic PID (3-4 digits)."""
    return random.randint(100, 999)


def random_tid():
    """Generate a realistic TID (3-4 digits)."""
    return random.randint(400, 600)


def random_hex_id():
    """Generate a random hex service/method ID (1-4 hex digits)."""
    length = random.randint(1, 4)
    value = random.randint(1, 16**length - 1)
    return format(value, 'x')  # lowercase hex, no 0x prefix


def random_payload():
    """Generate a random hex payload (5-60 bytes)."""
    num_bytes = random.randint(5, 60)
    return " ".join(f"{random.randint(0, 255):02x}" for _ in range(num_bytes))


def random_message_count():
    """Generate a random message count in hex (for the [0/XXXX] field)."""
    count = random.randint(1, 65535)
    return format(count, 'x')


# ---------------------------------------------------------------------------
# Example generator
# ---------------------------------------------------------------------------

def generate_example():
    """Generate a single synthetic training example."""
    timestamp = random_timestamp()
    pid = random_pid()
    tid = random_tid()
    msg_type = random.choice(["RX", "TX"])
    msg_count = random_message_count()
    service_hex = random_hex_id()
    method_hex = random_hex_id()
    payload = random_payload()

    # Build the log line and parse IDs based on message type
    if msg_type == "RX":
        # RX log line template (service and method are hex)
        log_line = (
            f"{timestamp}   {pid}   {tid} D "
            f"vendor.visteon.skylark.hardware.automotive.vehicle@V1-visteon-service: "
            f"SomeipCOmm :: {msg_type} Message [0/{msg_count}]:"
            f"service:{service_hex} ,Method:{method_hex},payload:{payload}"
        )
        service_id = int(service_hex, 16)
        method_id = int(method_hex, 16)
    else:
        # TX log line template (service and method are already decimal in the log line!)
        service_id = int(service_hex, 16)  # Using random hex generator but we format it as decimal
        method_id = int(method_hex, 16)
        
        log_line = (
            f"{timestamp}   {pid}   {tid} D "
            f"vendor.visteon.skylark.hardware.automotive.vehicle@V1-visteon-service: "
            f"TX Message SomeipComm::send  : service:{service_id},method:{method_id}, response Payload: {payload}"
        )

    response = {
        "time": timestamp,
        "type": msg_type.lower(),
        "serviceId": service_id,
        "methodId": method_id,
        "payload": payload
    }

    # Build the full training text
    text = (
        f"### Instruction:\n{INSTRUCTION}\n\n"
        f"### Input:\n{log_line}\n\n"
        f"### Response:\n{json.dumps(response, indent=2)}"
    )

    return {"text": text}


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    # Count existing examples
    existing_count = 0
    if os.path.exists(OUTPUT_FILE):
        with open(OUTPUT_FILE, "r", encoding="utf-8") as f:
            existing_count = sum(1 for line in f if line.strip())

    print(f"Existing examples: {existing_count}")
    print(f"Generating {NUM_EXAMPLES} synthetic examples...")

    # Append to the file
    with open(OUTPUT_FILE, "a", encoding="utf-8") as f:
        for i in range(NUM_EXAMPLES):
            example = generate_example()
            f.write(json.dumps(example) + "\n")

    total = existing_count + NUM_EXAMPLES
    print(f"Done! Total examples in dataset: {total}")
    print(f"Output: {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
