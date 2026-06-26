"""
SOME/IP VHAL Log Parser — Non-Target Log Extraction Script
=========================================================
Scans raw log files and isolates rows that do NOT match SOME/IP RX or TX 
protocol patterns, saving them to a separate file for engineering review.

Usage:
    python extract_non_target_logs.py
"""

import os
import re

# Exact regex patterns used in your dataset generation to identify valid targets
RX_PATTERN = re.compile(r"SomeipCOmm\s*::\s*RX\s*Message\s*(?:\[([^\]]+)\])?\s*:\s*service\s*:\s*[a-fA-F0-9]+\s*,\s*Method\s*:\s*[a-fA-F0-9]+\s*,\s*payload\s*:\s*[a-fA-F0-9 ]+")
TX_PATTERN = re.compile(r"TX\s*Message\s*SomeipComm::send\s*:\s*service\s*:\s*[a-fA-F0-9]+\s*,\s*method\s*:\s*[a-fA-F0-9]+\s*,\s*response\s*Payload\s*:\s*[a-fA-F0-9 ]+")

def extract_non_targets(input_file_path, output_file_path):
    if not os.path.exists(input_file_path):
        print(f"❌ Error: Input log file not found at: '{input_file_path}'")
        return False

    print(f"Scanning source logs: '{input_file_path}'")
    
    non_target_lines = []
    total_lines = 0
    target_count = 0

    with open(input_file_path, "r", encoding="utf-8") as f:
        for line in f:
            line_str = line.strip()
            if not line_str:
                continue
                
            total_lines += 1
            
            # Check if the line matches either valid RX or TX criteria
            is_rx = RX_PATTERN.search(line_str)
            is_tx = TX_PATTERN.search(line_str)
            
            if is_rx or is_tx or line_str.startswith("---"):
                target_count += 1
            else:
                # Line does not match target protocol; isolate it
                non_target_lines.append(line_str)

    # Save isolated noise rows to a separate file
    os.makedirs(os.path.dirname(output_file_path), exist_ok=True)
    with open(output_file_path, "w", encoding="utf-8") as out_f:
        for non_target in non_target_lines:
            out_f.write(non_target + "\n")

    non_target_count = len(non_target_lines)
    
    print("\n============================================================")
    print("                 EXTRACTION METRICS SUMMARY                 ")
    print("============================================================")
    print(f" Total Analyzed Lines     : {total_lines}")
    print(f" Valid SOME/IP Target Rows: {target_count}")
    print(f" Isolated Non-Target Rows : {non_target_count}")
    presents = (non_target_count / total_lines * 100) if total_lines > 0 else 0
    print(f" Log Noise Density        : {presents:.2f}%")
    print("============================================================")
    print(f"🎉 Success! Non-target rows isolated to:\n -> '{output_file_path}'\n")
    return True

if __name__ == "__main__":
    # Change this to point to any consolidated file or individual log file you want to inspect
    INPUT_LOG_FILE = "C:\\Sneha\\AI_Automation\\data\\ucl_90 1.log"
    OUTPUT_NOISE_FILE = "C:\\Sneha\\AI_Automation\\extracted_non_target_noise.log"

    extract_non_targets(input_file_path=INPUT_LOG_FILE, output_file_path=OUTPUT_NOISE_FILE)