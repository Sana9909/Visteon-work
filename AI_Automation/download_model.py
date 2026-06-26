# from huggingface_hub import snapshot_download

# # Download the model repo and save locally
# # snapshot_download(
# #     repo_id="unsloth/Llama-3.2-1B-Instruct-bnb-4bit",
# #     local_dir="C:/Sneha/models/llama3.2-1B",
# #     local_files_only=False
# # )


# snapshot_download(
#     repo_id="Llama-3.1-8B-Instruct",
#     local_dir="C:/Sneha/models/Llama-3.1-8B-Instruct",
#     local_files_only=False
# )


"""
Script to download Llama-3.2-3B-Instruct-bnb-4bit explicitly for offline use.
"""
import os
from huggingface_hub import snapshot_download

# Specify where you want to permanently store the model weights offline
OFFLINE_MODEL_DIR = "C:/Sneha/models/Llama-3.2-3B-Instruct-bnb-4bit"

print("====================================================")
print("     STARTING OFFLINE LLAMA 3.2 3B MODEL DOWNLOAD   ")
print("====================================================")
print(f"Target Destination Folder: {OFFLINE_MODEL_DIR}")
print("Please ensure a stable network connection...\n")

try:
    os.makedirs(OFFLINE_MODEL_DIR, exist_ok=True)
    
    # Secure download directly into the specified folder
    # Using unsloth's 4-bit optimized repo
    snapshot_download(
        repo_id="unsloth/Llama-3.2-3B-Instruct-bnb-4bit",
        local_dir=OFFLINE_MODEL_DIR,
        local_dir_use_symlinks=False,  # Copies actual files directly (ideal for Windows/offline backup)
        ignore_patterns=["*.msgpack", "*.h5", "*.ot"], # Ignore non-PyTorch/safetensors files to save space
    )
    
    print("\n====================================================")
    print("🎉 SUCCESS! Llama-3.2-3B-Instruct-bnb-4bit downloaded cleanly!")
    print(f"You can now run your pipeline entirely offline using path:\n-> '{OFFLINE_MODEL_DIR}'")
    print("====================================================")

except Exception as e:
    print(f"\n❌ Error during download: {e}")
    print("Ensure you ran 'huggingface-cli login' with a token approved for Meta-Llama access.")