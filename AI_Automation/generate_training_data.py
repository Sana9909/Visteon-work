"""
SOME/IP VHAL Log Parser — Training Dataset Generator (Three-Pool Boundary Mapping)
===========================================================================
Reads target rx.log, tx.log, and extracted_non_target_noise.log.
Maps valid SOME/IP messages to their schemas (WITHOUT protocol/serviceID), 
and explicitly maps all non-target noise lines to {"status": "ignored"}.

Usage:
    python generate_training_data.py
"""

import json
import os
import random
import re

# Updated: Instructions stripped of protocol and serviceID constraints
SYSTEM_INSTRUCTION = (
    "You are an advanced, deterministic log parsing engine optimized exclusively for SOME/IP network protocol traffic. "
    "Your job is to process an input log string and return a strictly structured JSON object.\n\n"
    "Follow these core parsing rules:\n"
    "1. Always extract the standard 'timestamp' field (MM-DD HH:MM:SS.mmm) at the root level.\n"
    "2. Extract and map the core communication details directly to the root level: 'type' ('RX' or 'TX'), "
    " 'serviceID', 'methodID', and 'payload' (as an array of hex byte strings).\n\n"
    "Output Constraints: Return ONLY a valid, minified JSON object string. Do not include markdown formatting blocks "
    "(like ```json), conversational text explanations, or trailing punctuation."
)

def parse_rx_line(log_line):
    """Parses an RX line matching the standard target schema pattern."""
    ts_match = re.match(r"^(\d{2}-\d{2}\s\d{2}:\d{2}:\d{2}\.\d{3})", log_line)
    if not ts_match:
        return None
    timestamp = ts_match.group(1)

    rx_pattern = r"SomeipCOmm\s*::\s*RX\s*Message\s*(?:\[[^\]]+\])?\s*:\s*service\s*:\s*([a-fA-F0-9]+)\s*,\s*Method\s*:\s*([a-fA-F0-9]+)\s*,\s*payload\s*:\s*([a-fA-F0-9 ]+)"
    match = re.search(rx_pattern, log_line)
    if not match:
        return None

    service_id, method_id, payload = match.groups()
    return {
        "timestamp": timestamp,
        "type": "RX",
        "serviceID": service_id.strip(),
        "methodID": method_id.strip(),
        "payload": payload.strip().split()
    }

def parse_tx_line(log_line):
    """Parses a TX line matching the standard target schema pattern."""
    ts_match = re.match(r"^(\d{2}-\d{2}\s\d{2}:\d{2}:\d{2}\.\d{3})", log_line)
    if not ts_match:
        return None
    timestamp = ts_match.group(1)

    tx_pattern = r"TX\s*Message\s*SomeipComm::send\s*:\s*service\s*:\s*([a-fA-F0-9]+)\s*,\s*method\s*:\s*([a-fA-F0-9]+)\s*,\s*response\s*Payload\s*:\s*([a-fA-F0-9 ]+)"
    match = re.search(tx_pattern, log_line)
    if not match:
        return None

    service_id, method_id, payload = match.groups()
    return {
        "timestamp": timestamp,
        "type": "TX",
        "serviceID": service_id.strip(),
        "methodID": method_id.strip(),
        "payload": payload.strip().split()
    }

def generate_dataset(rx_path, tx_path, noise_path, output_json_path):
    dataset = []
    rx_count = 0
    tx_count = 0
    ignored_count = 0

    print("============================================================")
    print("        STARTING THREE-POOL DATASET GENERATION LOGIC        ")
    print("============================================================")

    # Pool 1: Process Valid RX Log Files
    if os.path.exists(rx_path):
        print(f"Processing Pool [1/3] - RX Logs: '{rx_path}'")
        with open(rx_path, "r", encoding="utf-8") as f:
            for line in f:
                line_str = line.strip()
                if not line_str or line_str.startswith("---"):
                    continue
                parsed = parse_rx_line(line_str)
                if parsed:
                    rx_count += 1
                    dataset.append({
                        "instruction": SYSTEM_INSTRUCTION,
                        "input": line_str,
                        "output": json.dumps(parsed, ensure_ascii=False, separators=(',', ':'))
                    })

    # Pool 2: Process Valid TX Log Files
    if os.path.exists(tx_path):
        print(f"Processing Pool [2/3] - TX Logs: '{tx_path}'")
        with open(tx_path, "r", encoding="utf-8") as f:
            for line in f:
                line_str = line.strip()
                if not line_str or line_str.startswith("---"):
                    continue
                parsed = parse_tx_line(line_str)
                if parsed:
                    tx_count += 1
                    dataset.append({
                        "instruction": SYSTEM_INSTRUCTION,
                        "input": line_str,
                        "output": json.dumps(parsed, ensure_ascii=False, separators=(',', ':'))
                    })

    # Pool 3: Process Extracted Isolated Non-Target Noise Logs
    if os.path.exists(noise_path):
        print(f"Processing Pool [3/3] - Non-Target Noise Logs: '{noise_path}'")
        with open(noise_path, "r", encoding="utf-8") as f:
            for line in f:
                line_str = line.strip()
                if not line_str or line_str.startswith("---"):
                    continue
                
                ignored_count += 1
                ignored_schema = {"status": "ignored"}
                dataset.append({
                    "instruction": SYSTEM_INSTRUCTION,
                    "input": line_str,
                    "output": json.dumps(ignored_schema, ensure_ascii=False, separators=(',', ':'))
                })

    total_extracted = len(dataset)
    if total_extracted == 0:
        print("\n❌ Error: Dataset generation failed. No logs were successfully parsed.")
        return False

    print(f"\n🎲 Shuffling {total_extracted} records to balance training weights...")
    random.seed(42)
    random.shuffle(dataset)

    os.makedirs(os.path.dirname(output_json_path), exist_ok=True)
    with open(output_json_path, "w", encoding="utf-8") as out_f:
        json.dump(dataset, out_f, indent=4, ensure_ascii=False)

    print("\n============================================================")
    print("                 TRAINING DATASET METRICS SUMMARY            ")
    print("============================================================")
    print(f" Valid RX Target Items     : {rx_count}")
    print(f" Valid TX Target Items     : {tx_count}")
    print(f" Ground-Truth Ignored Rows : {ignored_count}")
    print(f" Consolidated Dataset Size : {total_extracted} samples")
    print("============================================================")
    print(f"🎉 Success! Comprehensive dataset ready for fine-tuning:\n -> '{output_json_path}'\n")
    return True

if __name__ == "__main__":
    RX_LOG_FILE = "C:\\Sneha\\AI_Automation\\rx_old.log"
    TX_LOG_FILE = "C:\\Sneha\\AI_Automation\\tx_old.log"
    NOISE_LOG_FILE = "C:\\Sneha\\AI_Automation\\non_target_noise_old.log"
    FINAL_DATASET_FILE = "C:\\Sneha\\AI_Automation\\llm_fine_tuning_dataset.json"

    generate_dataset(
        rx_path=RX_LOG_FILE, 
        tx_path=TX_LOG_FILE, 
        noise_path=NOISE_LOG_FILE, 
        output_json_path=FINAL_DATASET_FILE
    )