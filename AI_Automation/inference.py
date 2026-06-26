"""
SOME/IP VHAL Log Parser — Inference Script (Scrubbed Field Edition)
===================================================================
Loads the fine-tuned model and parses SOME/IP log lines into a focused JSON schema.
"""

import os
import sys
import json
import argparse
import torch
from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig
from peft import PeftModel

if sys.stdout.encoding != 'utf-8':
    sys.stdout.reconfigure(encoding='utf-8')

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# LORA_DIR = os.path.join(os.path.dirname(__file__), "output", "llama-3.2-3b-instruct-bnb-4bit", "vhal_parser_model")
# BASE_MODEL = "C:/Sneha/models/Llama-3.2-3B-Instruct-bnb-4bit"
LORA_DIR = os.path.join(os.path.dirname(__file__), "output", "llama3.2-1B", "vhal_parser_model")
BASE_MODEL = "C:/Sneha/models/llama3.2-1B"
MAX_SEQ_LENGTH = 1024

# Matches new training instructions perfectly
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


def load_model():
    """Load the fine-tuned model for inference."""
    if not os.path.exists(LORA_DIR):
        print("ERROR: No fine-tuned model found!")
        print(f"  Expected at: {LORA_DIR}")
        print(f"\nPlease run 'python train.py' first to fine-tune the model.")
        sys.exit(1)

    print(f"Loading base model and LoRA adapter from: {LORA_DIR}")

    bnb_config = BitsAndBytesConfig(
        load_in_4bit=True,
        bnb_4bit_use_double_quant=True,
        bnb_4bit_quant_type="nf4",
        bnb_4bit_compute_dtype=torch.float16
    )

    tokenizer = AutoTokenizer.from_pretrained(BASE_MODEL)
    base_model = AutoModelForCausalLM.from_pretrained(
        BASE_MODEL,
        quantization_config=bnb_config,
        device_map="auto"
    )
    
    model = PeftModel.from_pretrained(base_model, LORA_DIR)

    print(f"Model loaded successfully on {model.device}\n")
    return model, tokenizer


# def normalize_to_target_schema(model_dict: dict) -> dict:
#     """Restructures model outputs into the flat target focused JSON schema."""
#     if not model_dict or not isinstance(model_dict, dict):
#         return {"status": "ignored"}
        
#     if model_dict.get("status") == "ignored":
#         return {"status": "ignored"}

#     def safe_int(value):
#         if value is None:
#             return 0
#         try:
#             val_str = str(value).strip()
#             if not val_str:
#                 return 0
#             if any(c in val_str.lower() for c in 'abcdef') or val_str.lower().startswith('0x'):
#                 return int(val_str, 16)
#             return int(val_str)
#         except ValueError:
#             return str(value)

#     # Focused parameter remapping
#     time_val = model_dict.get("timestamp") or model_dict.get("time") or ""
#     type_val = model_dict.get("type") or ""
#     mid_val = model_dict.get("methodID") if model_dict.get("methodID") is not None else model_dict.get("methodId")
#     payload_val = model_dict.get("payload") or model_dict.get("response Payload") or ""

#     # Check validity without using serviceID
#     if not mid_val and not payload_val:
#         return {"status": "ignored"}

#     return {
#         "time": str(time_val).strip(),
#         "type": str(type_val).lower().strip(),
#         "methodId": safe_int(mid_val),
#         "payload": str(payload_val).strip()
#     }

def normalize_to_target_schema(model_dict: dict) -> dict:
    """Restructures model outputs into the flat target focused JSON schema."""
    if not model_dict or not isinstance(model_dict, dict):
        return {"status": "ignored"}
        
    if model_dict.get("status") == "ignored":
        return {"status": "ignored"}

    def safe_int(value):
        """Converts hex values to base-10 integers."""
        if value is None:
            return 0
        try:
            val_str = str(value).strip()
            return int(val_str, 16)
        except ValueError:
            return str(value)

    # Focused parameter remapping
    time_val = model_dict.get("timestamp") or model_dict.get("time") or ""
    type_val = str(model_dict.get("type") or "").strip().upper()
    sid_val = model_dict.get("serviceID") or model_dict.get("serviceId")
    mid_val = model_dict.get("methodID") if model_dict.get("methodID") is not None else model_dict.get("methodId")
    payload_val = model_dict.get("payload") or model_dict.get("response Payload") or ""
    if isinstance(payload_val, list):
        payload_str = " ".join(str(p) for p in payload_val)
    else:
        payload_str = str(payload_val).strip()

    # Check validity without using serviceID
    if not mid_val and not payload_str:
        return {"status": "ignored"}

    # CRITICAL UPDATE: Convert to integer ONLY if the log direction type is 'RX'
    if type_val == "RX":
        final_method_id = safe_int(mid_val)
        final_service_id = safe_int(sid_val)
    else:
        # Leave TX or other types as clean raw strings (e.g., "8007" or "0x1a")
        final_method_id = str(mid_val).strip() if mid_val is not None else ""
        final_service_id = str(sid_val).strip() if sid_val is not None else ""

    return {
        "time": str(time_val).strip(),
        "type": type_val.lower(),
        "serviceId": final_service_id,
        "methodId": final_method_id,
        "payload": payload_str
    }


def parse_log_line(model, tokenizer, log_line: str) -> dict:
    """Parse a single SOME/IP log line and return the structured JSON."""
    prompt = (
        f"### Instruction:\n{SYSTEM_INSTRUCTION}\n\n"
        f"### Input:\n{log_line.strip()}\n\n"
        f"### Response:\n"
    )

    inputs = tokenizer(prompt, return_tensors="pt").to(model.device)

    with torch.no_grad():
        outputs = model.generate(
            **inputs,
            max_new_tokens=1024,
            temperature=0.1,
            top_p=0.9,
            do_sample=True,
            use_cache=True,
            pad_token_id=tokenizer.eos_token_id,
        )

    prompt_length = inputs["input_ids"].shape[1]
    generated_tokens = outputs[0][prompt_length:]
    response_text = tokenizer.decode(generated_tokens, skip_special_tokens=True).strip()

    try:
        json_start = response_text.find("{")
        json_end = response_text.rfind("}") + 1
        if json_start != -1 and json_end > json_start:
            json_str = response_text[json_start:json_end]
            raw_parsed = json.loads(json_str)
            return normalize_to_target_schema(raw_parsed)
        else:
            return {"error": "No JSON found in response", "raw": response_text}
    except json.JSONDecodeError as e:
        return {"error": f"Invalid JSON generated: {e}", "raw": response_text}


def process_file(model, tokenizer, input_file: str, output_file: str = None):
    """Process a file of log lines, one per line."""
    results = []

    with open(input_file, "r", encoding="utf-8") as f:
        lines = [line.strip() for line in f if line.strip()]

    print(f"Processing {len(lines)} log lines from {input_file}...\n")

    for i, line in enumerate(lines):
        if line.startswith("---"):
            continue
            
        print(f"  [{i+1}/{len(lines)}] Parsing...")
        result = parse_log_line(model, tokenizer, line)
        
        if result.get("status") == "ignored":
            continue
            
        results.append(result)
        print(f"           → methodId={result.get('methodId', '?')}")

    if output_file:
        with open(output_file, "w", encoding="utf-8") as f:
            json.dump(results, f, indent=2)
        print(f"\nResults saved to: {output_file}")
    else:
        print("\nResults:")
        print(json.dumps(results, indent=2))

    return results


def interactive_mode(model, tokenizer):
    """Interactive REPL mode."""
    print("=" * 60)
    print("  SOME/IP Focused Log Parser — Interactive Mode")
    print("=" * 60)
    print()

    while True:
        try:
            log_line = input("📥 Paste log line: ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nGoodbye!")
            break

        if log_line.lower() in ("quit", "exit", "q"):
            print("Goodbye!")
            break

        if not log_line:
            continue

        print("⏳ Parsing...\n")
        result = parse_log_line(model, tokenizer, log_line)
        print("📤 Parsed output:")
        print(json.dumps(result, indent=2))
        print()


def main():
    parser = argparse.ArgumentParser(description="SOME/IP Focused VHAL Log Parser")
    parser.add_argument("--input", "-i", type=str, help="A single log line to parse")
    parser.add_argument("--file", "-f", type=str, help="Path to a log file")
    parser.add_argument("--output", "-o", type=str, help="Path to save JSON output")

    args = parser.parse_args()
    model, tokenizer = load_model()

    if args.input:
        result = parse_log_line(model, tokenizer, args.input)
        print(json.dumps(result, indent=2))
    elif args.file:
        process_file(model, tokenizer, args.file, args.output)
    else:
        interactive_mode(model, tokenizer)


if __name__ == "__main__":
    main()