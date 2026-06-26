"""
SOME/IP VHAL Log Parser — Bulk File Parsing Script (Focused Architecture)
========================================================================
Reads a raw log text file, ignores non-target rows silently, and streams 
valid elements using the focused fields layout directly to disk.
"""

import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from inference import load_model, parse_log_line

def process_log_file_realtime(input_txt_path, output_json_path):
    if not os.path.exists(input_txt_path):
        print(f"❌ Error: The input file was not found at: '{input_txt_path}'")
        return False

    print("Loading fine-tuned model and pipeline configurations...")
    model, tokenizer = load_model()
    print("Model successfully loaded.")

    print(f"Reading target file contents: '{input_txt_path}'...")
    with open(input_txt_path, "r", encoding="utf-8") as f:
        raw_lines = f.readlines()

    clean_lines = []
    for line in raw_lines:
        line_str = line.strip()
        if line_str and not line_str.startswith("---"):
            clean_lines.append(line_str)

    total_lines = len(clean_lines)
    print(f"Found {total_lines} valid target log entries to process.\n")

    os.makedirs(os.path.dirname(output_json_path), exist_ok=True)

    print("============================================================")
    print("        STARTING REAL-TIME BULK LOG PARSING (JSON ARRAY)     ")
    print("============================================================")
    
    with open(output_json_path, "w", encoding="utf-8") as out_f:
        out_f.write("[\n")
        out_f.flush()
        
        is_first_line = True
        
        for idx, log_line in enumerate(clean_lines):
            try:
                model_output = parse_log_line(model, tokenizer, log_line)
                
                if isinstance(model_output, str):
                    try:
                        model_output = json.loads(model_output.strip())
                    except json.JSONDecodeError:
                        model_output = {}
            except Exception as e:
                model_output = {}

            # Drop ignored lines cleanly without displaying them
            if not model_output or model_output.get("status") == "ignored":
                continue

            # Display parsing status tracking serviceId and methodId
            print(f"[{idx + 1}/{total_lines}] Parsed Target -> serviceId={model_output.get('serviceId')} methodId={model_output.get('methodId')}")

            json_item_string = json.dumps(model_output, indent=4, ensure_ascii=False)
            indented_item = "\n".join(f"  {line}" for line in json_item_string.splitlines())
            
            if not is_first_line:
                out_f.write(",\n")
                
            out_f.write(indented_item)
            is_first_line = False
            out_f.flush()
            
        out_f.write("\n]\n")
        out_f.flush()

    print("============================================================")
    print("                 BULK PARSING SEQUENCE COMPLETE             ")
    print("============================================================\n")
    print(f"🎉 Success! Output file built successfully:\n -> '{output_json_path}'\n")
    return True


if __name__ == "__main__":
    INPUT_LOG_FILE = "C:\\Sneha\\AI_Automation\\data\\ucl_test.log"
    OUTPUT_JSON_FILE = "C:\\Sneha\\AI_Automation\\data\\output\\parsed_ucl_test_logs.json"

    process_log_file_realtime(input_txt_path=INPUT_LOG_FILE, output_json_path=OUTPUT_JSON_FILE)