# """
# SOME/IP VHAL Log Parser — Model Validation Script
# ==================================================
# Tests the fine-tuned model against known examples to verify accuracy.

# Usage:
#     python test_model.py
# """

# import json
# import sys
# import os

# # Add project root to path
# sys.path.insert(0, os.path.dirname(__file__))

# from inference import load_model, parse_log_line


# # ---------------------------------------------------------------------------
# # Test cases loaded from unique_logs.json
# # ---------------------------------------------------------------------------

# def load_test_cases():
#     test_cases_path = os.path.join(os.path.dirname(__file__), "unique_logs.json")
#     with open(test_cases_path, "r", encoding="utf-8") as f:
#         data = json.load(f)
#     test_cases = []
#     for entry in data:
#         test_cases.append({
#             "input": entry["input"],
#             "expected": entry["output"]
#         })
#     return test_cases


# # ---------------------------------------------------------------------------
# # Validation
# # ---------------------------------------------------------------------------

# def validate_result(result: dict, expected: dict, path="") -> list:
#     """Compare parsed result against expected values (recursive). Returns list of errors."""
#     errors = []

#     for key in expected:
#         current_path = f"{path}.{key}" if path else key
#         if key not in result:
#             errors.append(f"Missing field: '{current_path}'")
#             continue

#         if isinstance(expected[key], dict):
#             if not isinstance(result[key], dict):
#                 errors.append(f"Field '{current_path}' should be a dict")
#             else:
#                 errors.extend(validate_result(result[key], expected[key], current_path))
#         else:
#             if result[key] != expected[key]:
#                 errors.append(
#                     f"Field '{current_path}': expected {expected[key]!r}, got {result[key]!r}"
#                 )

#     # Check for unexpected fields (except 'error' and 'raw' at root)
#     for key in result:
#         current_path = f"{path}.{key}" if path else key
#         if key not in expected:
#             if path == "" and key in ("error", "raw"):
#                 continue
#             errors.append(f"Unexpected field: '{current_path}' = {result[key]!r}")

#     return errors


# # def run_tests():
# #     """Run all test cases and report results."""
# #     print("=" * 60)
# #     print("  SOME/IP VHAL Log Parser — Model Validation")
# #     print("=" * 60)
# #     print()

# #     # Load model
# #     model, tokenizer = load_model()

# #     test_cases = load_test_cases()
# #     passed = 0
# #     failed = 0
# #     total = len(test_cases)

# #     for i, test in enumerate(test_cases):
# #         print(f"Test {i+1}/{total}:")
# #         print(f"  Input: {test['input'][:80]}...")

# #         result = parse_log_line(model, tokenizer, test["input"])
# #         errors = validate_result(result, test["expected"])

# #         if errors:
# #             failed += 1
# #             print(f"  ❌ FAILED")
# #             for err in errors:
# #                 print(f"     - {err}")
# #             print(f"  Got:      {json.dumps(result, indent=2)}")
# #             print(f"  Expected: {json.dumps(test['expected'], indent=2)}")
# #         else:
# #             passed += 1
# #             print(f"  ✅ PASSED")
# #             print(f"  Result matched expected structure.")

# #         print()

# #     # Summary
# #     print("=" * 60)
# #     print(f"  Results: {passed}/{total} passed, {failed}/{total} failed")
# #     if failed == 0:
# #         print("  🎉 All tests passed!")
# #     else:
# #         print("  ⚠️  Some tests failed — consider more training data or epochs.")
# #     print("=" * 60)

# #     return failed == 0

# def run_tests():
#     # 1. Load model
#     model, tokenizer = load_model()

#     # 2. Load your test cases
#     test_cases = load_test_cases()
    
#     # 3. Initialize a list to hold all collected model outputs
#     all_model_outputs = []
    
#     print(f"Starting validation on {len(test_cases)} test cases...")
    
#     for idx, test_case in enumerate(test_cases):
#         input_log = test_case['input']
        
#         # Run inference using the helper from inference.py
#         parsed_json = parse_log_line(model, tokenizer, input_log)
            
#         # Keep track of what case this belongs to by storing it alongside the original input metadata
#         record = {
#             "test_case_index": idx,
#             "input_log": input_log,
#             "expected_output": test_case.get("expected", {}),
#             "model_parsed_output": parsed_json
#         }
        
#         # Append the record to our collection list
#         all_model_outputs.append(record)
        
#         print(f"Processed test case {idx + 1}/{len(test_cases)}")

#     # 4. After the loop completes, save the entire collection list to a new separate file
#     output_filepath = "C:\\Sneha\\AI_Automation\\validation_results.json"
    
#     with open(output_filepath, "w", encoding="utf-8") as out_f:
#         json.dump(all_model_outputs, out_f, indent=4, ensure_ascii=False)
        
#     print(f"\n============================================================")
#     print(f"Success! All model test outputs saved to: {output_filepath}")
#     print(f"============================================================")
    
#     return True


# if __name__ == "__main__":
#     success = run_tests()
#     sys.exit(0 if success else 1)


"""
SOME/IP VHAL Log Parser — Model Validation Script
==================================================
Tests the fine-tuned model against known examples to verify accuracy, 
logs granular nested parameter mismatch tracking, and calculates metrics.

Usage:
    python test_model.py
"""

import json
import sys
import os

# Add project root to path
sys.path.insert(0, os.path.dirname(__file__))

from inference import load_model, parse_log_line


# ---------------------------------------------------------------------------
# Test cases loaded from unique_logs.json / llm_fine_tuning_dataset.json
# ---------------------------------------------------------------------------

def load_test_cases():
    """
    Loads validation examples securely. Automatically tracks whether inputs 
    are nested under standard format shapes.
    """
    # test_cases_path = os.path.join(os.path.dirname(__file__), "unique_logs.json")
    
    # If using your instruction-fine-tuning format dataset, swap filename:
    test_cases_path = os.path.join(os.path.dirname(__file__), "llm_fine_tuning_dataset.json")
    
    with open(test_cases_path, "r", encoding="utf-8") as f:
        data = json.load(f)
        
    test_cases = []
    for entry in data:
        # Gracefully handle formats containing either 'output' or 'expected' structures
        expected_output = entry.get("output", entry.get("expected", {}))
        
        # If stringified minified JSON is extracted from training rows, unpack it
        if isinstance(expected_output, str):
            try:
                expected_output = json.loads(expected_output)
            except json.JSONDecodeError:
                pass
                
        test_cases.append({
            "input": entry["input"],
            "expected": expected_output
        })
    return test_cases


# ---------------------------------------------------------------------------
# Deep Structural Comparison Validation
# ---------------------------------------------------------------------------

def validate_result_deep(expected, predicted, path="") -> list:
    """
    Recursively compares ground-truth expected logs with model-predicted outputs.
    Ensures safe casting adjustments so data types don't trigger false failures.
    Returns a list of explicit error messages.
    """
    errors = []

    # 1. Handle mismatched data types fundamentally
    if type(expected) != type(predicted):
        # Allow pass if it's a numeric type conversion discrepancy (e.g., 193 vs "193")
        if str(expected).strip() == str(predicted).strip():
            return errors
        errors.append(f"Type mismatch at '{path if path else 'root'}': Expected {type(expected).__name__}, got {type(predicted).__name__}")
        return errors

    # 2. Recursive Dictionary Processing
    if isinstance(expected, dict):
        for key in expected:
            current_path = f"{path}.{key}" if path else key
            if key not in predicted:
                errors.append(f"Missing field: '{current_path}'")
                continue
            errors.extend(validate_result_deep(expected[key], predicted[key], current_path))
            
        # Check for unexpected fields generated by hallucination
        for key in predicted:
            current_path = f"{path}.{key}" if path else key
            if key not in expected:
                if path == "" and key in ("error", "raw"):
                    continue
                errors.append(f"Unexpected field: '{current_path}' = {predicted[key]!r}")

    # 3. Recursive List/Array Processing
    elif isinstance(expected, list):
        if len(expected) != len(predicted):
            errors.append(f"Length mismatch at '{path}': Expected length {len(expected)}, got {len(predicted)}")
        else:
            for i in range(len(expected)):
                errors.extend(validate_result_deep(expected[i], predicted[i], f"{path}[{i}]"))

    # 4. Primitive Element Checking (Strings, Integers, Booleans)
    else:
        if str(expected).strip() != str(predicted).strip():
            errors.append(f"Value mismatch at '{path}': expected {expected!r}, got {predicted!r}")

    return errors


# ---------------------------------------------------------------------------
# Validation Runner Engine
# ---------------------------------------------------------------------------

def run_tests():
    # 1. Load fine-tuned model parameters
    model, tokenizer = load_model()

    # 2. Load the test case collections
    test_cases = load_test_cases()
    
    all_model_outputs = []
    passed_count = 0
    total_count = len(test_cases)
    
    print("\n============================================================")
    print(f" Starting Validation Engine on {total_count} Test Cases...")
    print("============================================================\n")
    
    for idx, test_case in enumerate(test_cases):
        input_log = test_case['input']
        expected_json = test_case['expected']
        
        # Run live model inference using the helper pipeline from inference.py
        parsed_json = parse_log_line(model, tokenizer, input_log)
        
        # If the inference helper returns a raw unparsed string string, cast it safely
        if isinstance(parsed_json, str):
            try:
                parsed_json = json.loads(parsed_json.strip())
            except json.JSONDecodeError:
                pass

        # Execute deep validation match tracking
        if isinstance(parsed_json, dict):
            errors = validate_result_deep(expected_json, parsed_json)
        else:
            errors = ["Model output is not valid/malformed JSON syntax structure."]
            
        is_pass = len(errors) == 0
        
        # Terminal Logging Configuration using robust status reporting
        print(f"[{idx + 1}/{total_count}] Processing Case...")
        print(f"  Input Log: {input_log[:110]}...")
        
        if is_pass:
            passed_count += 1
            print(f"  Status:    \033[92mPASS\033[0m")  # Green pass marker
        else:
            print(f"  Status:    \033[91mFAIL\033[0m")  # Red fail marker
            for err in errors:
                print(f"    - Mismatch Details: {err}")
                
        print("-" * 70)
            
        # Append complete diagnostic data array records
        record = {
            "test_case_index": idx,
            "input_log": input_log,
            "status": "PASS" if is_pass else "FAIL",
            "mismatch_errors": errors,
            "expected_output": expected_json,
            "model_parsed_output": parsed_json
        }
        all_model_outputs.append(record)

    # Calculate precise accuracy metric calculations
    failed_count = total_count - passed_count
    accuracy = (passed_count / total_count) * 100 if total_count > 0 else 0

    # 3. Print out Final Summary Metrics Dashboard Console View
    print("\n============================================================")
    print("                 VALIDATION METRICS SUMMARY                 ")
    print("============================================================")
    print(f" Total Executed Test Cases : {total_count}")
    print(f" Total Successfully Passed : \033[92m{passed_count}\033[0m")
    print(f" Total Failed Test Cases   : \033[91m{failed_count}\033[0m")
    print(f" Overall Parsing Accuracy  : {accuracy:.2f}%")
    print("============================================================")

    # 4. Save the diagnostic results dataset tracking output
    output_filepath = "C:\\Sneha\\AI_Automation\\validation_results.json"
    with open(output_filepath, "w", encoding="utf-8") as out_f:
        json.dump(all_model_outputs, out_f, indent=4, ensure_ascii=False)
        
    print(f"Success! Comprehensive execution file saved to:\n -> {output_filepath}\n")
    
    return failed_count == 0


if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)