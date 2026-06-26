# """
# SOME/IP VHAL Log Parser — Fine-Tuning Script (Hugging Face + QLoRA)
# ===============================================================
# Fine-tunes a small LLM to parse SOME/IP log lines into structured JSON.

# Prerequisites:
#     - NVIDIA GPU with >= 8 GB VRAM
#     - Python packages: torch, transformers, peft, trl, datasets, bitsandbytes

# Usage:
#     python train.py

# The fine-tuned model is saved to: ./output/vhal_parser_model/
# """

# # ============================================================
# # Python 3.14 Compatibility Fix (MUST be before other imports)
# # ============================================================
# import sys
# import os
# from transformers import AutoModelForCausalLM, AutoTokenizer

# if sys.stdout.encoding != 'utf-8':
#     sys.stdout.reconfigure(encoding='utf-8')

# if sys.version_info >= (3, 14):
#     import pickle
#     import dill._dill
#     dill._dill.Pickler._batch_setitems = pickle._Pickler._batch_setitems
#     try:
#         import datasets.utils._dill
#         datasets.utils._dill.Pickler._batch_setitems = pickle._Pickler._batch_setitems
#     except ImportError:
#         pass
# # ============================================================

# import json
# import torch
# from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig
# from peft import get_peft_model, LoraConfig, prepare_model_for_kbit_training
# from trl import SFTTrainer, SFTConfig

# # ---------------------------------------------------------------------------
# # Configuration
# # ---------------------------------------------------------------------------

# # Base model
# # BASE_MODEL = "C:/Sneha/models/llama3.2-1B"
# BASE_MODEL = "C:/Sneha/models/Llama-3-8B-Instruct"

# # Paths
# DATASET_PATH = os.path.join(os.path.dirname(__file__), "llm_fine_tuning_dataset.json")
# OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "output", "vhal_parser_model")

# # LoRA Configuration
# LORA_RANK = 16              # Higher = more capacity, more VRAM
# LORA_ALPHA = 32             # Scaling factor (typically 2x rank)
# LORA_DROPOUT = 0.05         # Small dropout for regularization

# # Training Hyperparameters
# NUM_EPOCHS = 5              # Number of training epochs
# LEARNING_RATE = 2e-4        # Learning rate
# BATCH_SIZE = 2              # Per-device batch size
# GRAD_ACCUM_STEPS = 4        # Effective batch size = BATCH_SIZE * GRAD_ACCUM_STEPS = 8
# MAX_SEQ_LENGTH = 1024        # Max tokens per example
# WARMUP_STEPS = 10           # Warmup steps for learning rate scheduler
# LOGGING_STEPS = 10          # Log training loss every N steps
# SAVE_STEPS = 50             # Save checkpoint every N steps

# # ---------------------------------------------------------------------------
# # Step 1: Load the base model with 4-bit quantization
# # ---------------------------------------------------------------------------

# def load_model():
#     """Load the base model using transformers and bitsandbytes."""
#     print("=" * 60)
#     print("STEP 1: Loading base model...")
#     print(f"  Model: {BASE_MODEL}")
#     print(f"  Max sequence length: {MAX_SEQ_LENGTH}")
#     print("=" * 60)

#     # 4-bit quantization config
#     bnb_config = BitsAndBytesConfig(
#         load_in_4bit=True,
#         bnb_4bit_use_double_quant=True,
#         bnb_4bit_quant_type="nf4",
#         bnb_4bit_compute_dtype=torch.float16
#     )

#     model = AutoModelForCausalLM.from_pretrained(
#         BASE_MODEL,
#         local_files_only=True
#     )
    
#     tokenizer = AutoTokenizer.from_pretrained(
#         BASE_MODEL,
#         local_files_only=True
#     )

#     # Prepare model for kbit training
#     model = prepare_model_for_kbit_training(model)

#     print(f"  Model loaded successfully on {model.device}")
#     return model, tokenizer


# # ---------------------------------------------------------------------------
# # Step 2: Apply LoRA adapters
# # ---------------------------------------------------------------------------

# def apply_lora(model):
#     """Apply LoRA adapters to the model for parameter-efficient fine-tuning."""
#     print("\n" + "=" * 60)
#     print("STEP 2: Applying LoRA adapters...")
#     print(f"  Rank: {LORA_RANK}")
#     print(f"  Alpha: {LORA_ALPHA}")
#     print(f"  Dropout: {LORA_DROPOUT}")
#     print("=" * 60)

#     config = LoraConfig(
#         r=LORA_RANK,
#         lora_alpha=LORA_ALPHA,
#         target_modules=["q_proj", "k_proj", "v_proj", "o_proj", "gate_proj", "up_proj", "down_proj"],
#         lora_dropout=LORA_DROPOUT,
#         bias="none",
#         task_type="CAUSAL_LM"
#     )

#     model = get_peft_model(model, config)

#     trainable, total = model.get_nb_trainable_parameters()
#     print(f"  Trainable parameters: {trainable:,} / {total:,} ({100 * trainable / total:.2f}%)")
#     return model


# # ---------------------------------------------------------------------------
# # Step 3: Load and format the dataset
# # ---------------------------------------------------------------------------

# def load_training_data(tokenizer):
#     """Load the JSONL dataset and format for training."""
#     print("\n" + "=" * 60)
#     print("STEP 3: Loading training data...")
#     print(f"  Dataset: {DATASET_PATH}")
#     print("=" * 60)

#     if not os.path.exists(DATASET_PATH):
#         raise FileNotFoundError(
#             f"Dataset not found: {DATASET_PATH}\n"
#             f"Please run 'python data/augment_data.py' first to generate training data."
#         )

#     # Load JSON manually (avoids datasets.load_dataset pickle bug on Python 3.14)
#     EOS_TOKEN = tokenizer.eos_token
#     texts = []
#     with open(DATASET_PATH, "r", encoding="utf-8") as f:
#         data = json.load(f)
#         for entry in data:
#             instruction = entry.get("instruction", "")
#             input_text = entry.get("input", "")
#             output_text = entry.get("output", "")
            
#             # Format text in Llama 3 style or Alpaca style
#             text = f"### Instruction:\n{instruction}\n\n### Input:\n{input_text}\n\n### Response:\n{output_text}"
#             texts.append(text + EOS_TOKEN)

#     # Build a HuggingFace Dataset from the list
#     from datasets import Dataset
#     dataset = Dataset.from_dict({"text": texts})
#     print(f"  Total examples: {len(dataset)}")

#     return dataset


# # ---------------------------------------------------------------------------
# # Step 4: Train the model
# # ---------------------------------------------------------------------------

# def train(model, tokenizer, dataset):
#     """Run the fine-tuning loop."""
#     print("\n" + "=" * 60)
#     print("STEP 4: Starting fine-tuning...")
#     print(f"  Epochs: {NUM_EPOCHS}")
#     print(f"  Batch size: {BATCH_SIZE}")
#     print(f"  Output: {OUTPUT_DIR}")
#     print("=" * 60)

#     # trainer = SFTTrainer(
#     #     model=model,
#     #     processing_class=tokenizer,
#     #     train_dataset=dataset,
#     #     args=SFTConfig(
#     #         output_dir=OUTPUT_DIR,
#     #         num_train_epochs=NUM_EPOCHS,
#     #         per_device_train_batch_size=BATCH_SIZE,
#     #         gradient_accumulation_steps=GRAD_ACCUM_STEPS,
#     #         learning_rate=LEARNING_RATE,
#     #         lr_scheduler_type="cosine",
#     #         warmup_steps=WARMUP_STEPS,
#     #         logging_steps=LOGGING_STEPS,
#     #         save_steps=SAVE_STEPS,
#     #         save_total_limit=2,             # Keep only 2 latest checkpoints
#     #         fp16=not torch.cuda.is_bf16_supported(),
#     #         bf16=torch.cuda.is_bf16_supported(),
#     #         optim="adamw_8bit",             # Memory-efficient optimizer
#     #         seed=42,
#     #         max_length=MAX_SEQ_LENGTH,
#     #         dataset_text_field="text",
#     #         packing=True,                   # Pack short examples together for efficiency
#     #         report_to="none",               # Disable wandb/tensorboard
#     #     ),
#     # )

#     args = SFTConfig(
#         output_dir=OUTPUT_DIR,
#         max_length=MAX_SEQ_LENGTH,                     # ✅ replaces deprecated max_seq_length
#         per_device_train_batch_size=BATCH_SIZE,
#         gradient_accumulation_steps=GRAD_ACCUM_STEPS,
#         num_train_epochs=NUM_EPOCHS,                # 🔧 extended epochs
#         learning_rate=LEARNING_RATE,
#         logging_steps=LOGGING_STEPS,
#         save_steps=SAVE_STEPS,
#         report_to="none",
#         fp16=True,
#         bf16=False,
#         packing=False,                      # 🔧 disables packing
#         # attn_implementation="xformers"      # 🔧 explicitly set attention backend
#     )

#     trainer = SFTTrainer(
#         model=model,
#         processing_class=tokenizer,
#         train_dataset=dataset,
#         args=args
#     )

#     print("\n  Training started...\n")
#     trainer_stats = trainer.train()

#     print("\n" + "=" * 60)
#     print("TRAINING COMPLETE!")
#     print(f"  Total training time: {trainer_stats.metrics['train_runtime']:.0f} seconds")
#     print(f"  Final loss: {trainer_stats.metrics['train_loss']:.4f}")
#     print("=" * 60)

#     return trainer


# # ---------------------------------------------------------------------------
# # Step 5: Save the fine-tuned model
# # ---------------------------------------------------------------------------

# def save_model(model, tokenizer):
#     """Save the fine-tuned model and tokenizer."""
#     print("\n" + "=" * 60)
#     print("STEP 5: Saving fine-tuned model...")
#     print(f"  Output: {OUTPUT_DIR}")
#     print("=" * 60)

#     model.save_pretrained(OUTPUT_DIR)
#     tokenizer.save_pretrained(OUTPUT_DIR)

#     print("  Model saved successfully!")


# # ---------------------------------------------------------------------------
# # Main
# # ---------------------------------------------------------------------------

# def main():
#     print("\n" + "=" * 60)
#     print("  SOME/IP VHAL Log Parser — Fine-Tuning Pipeline")
#     print("  Using Hugging Face PEFT + QLoRA")
#     print("=" * 60 + "\n")

#     # Step 1: Load model
#     model, tokenizer = load_model()

#     # Step 2: Apply LoRA
#     model = apply_lora(model)

#     # Step 3: Load data
#     dataset = load_training_data(tokenizer)

#     # Step 4: Train
#     train(model, tokenizer, dataset)

#     # Step 5: Save
#     save_model(model, tokenizer)

#     print("\n" + "=" * 60)
#     print("  ALL DONE!")
#     print(f"  Run inference with: python inference.py")
#     print("=" * 60 + "\n")


# if __name__ == "__main__":
#     main()


"""
SOME/IP VHAL Log Parser — Fine-Tuning Script (Hugging Face + QLoRA)
===============================================================
Fine-tunes a Llama 3B model to parse SOME/IP log lines into structured JSON.
Checkpoints are disabled during training to save disk space/time.
"""

# ============================================================
# Python 3.14 Compatibility Fix (MUST be before other imports)
# ============================================================
import sys
import os

if sys.stdout.encoding != 'utf-8':
    sys.stdout.reconfigure(encoding='utf-8')

if sys.version_info >= (3, 14):
    import pickle
    import dill._dill
    dill._dill.Pickler._batch_setitems = pickle._Pickler._batch_setitems
    try:
        import datasets.utils._dill
        datasets.utils._dill.Pickler._batch_setitems = pickle._Pickler._batch_setitems
    except ImportError:
        pass
# ============================================================

import json
import torch
from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig
from peft import get_peft_model, LoraConfig, prepare_model_for_kbit_training
from trl import SFTTrainer, SFTConfig

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Point directly to your downloaded offline model directory or the HF model name
BASE_MODEL = "C:/Sneha/models/Llama-3.2-3B-Instruct-bnb-4bit" 
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "output", "llama-3.2-3b-instruct-bnb-4bit", "vhal_parser_model")
DATASET_PATH = "C:/Sneha/AI_Automation/llm_fine_tuning_dataset.json"

MAX_SEQ_LENGTH = 1024


# ---------------------------------------------------------------------------
# Step 1: Load quantized base model and tokenizer
# ---------------------------------------------------------------------------

def load_model():
    """Load model in 4-bit precision for memory conservation on consumer GPUs."""
    print("=" * 60)
    print(f"STEP 1: Loading base model from: {BASE_MODEL}")
    print("=" * 60)

    # Configured specifically for 8B models to minimize VRAM down to ~7GB
    bnb_config = BitsAndBytesConfig(
        load_in_4bit=True,
        bnb_4bit_use_double_quant=True,
        bnb_4bit_quant_type="nf4",
        bnb_4bit_compute_dtype=torch.float16,
    )

    tokenizer = AutoTokenizer.from_pretrained(BASE_MODEL, trust_remote_code=True)
    
    # Crucial adjustment for Llama models which lack an explicit padding token
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token

    model = AutoModelForCausalLM.from_pretrained(
        BASE_MODEL,
        quantization_config=bnb_config,
        device_map="auto", 
        trust_remote_code=True
    )

    # Configure caching rules for compatibility with LoRA gradient checkpointing
    model.config.use_cache = False
    model.config.pretraining_tp = 1

    print(f"Base model mapped to device: {model.device}")
    return model, tokenizer


# ---------------------------------------------------------------------------
# Step 2: Configure LoRA Adapters
# ---------------------------------------------------------------------------

def apply_lora(model):
    """Wrap base layers with low-rank adaptation matrix overrides."""
    print("\n" + "=" * 60)
    print("STEP 2: Preparing model for QLoRA adaptation...")
    print("=" * 60)

    model = prepare_model_for_kbit_training(model)

    # Targeted modules matching the full attention query-value structures of Llama architecture
    peft_config = LoraConfig(
        r=16,
        lora_alpha=32,
        target_modules=["q_proj", "k_proj", "v_proj", "o_proj", "gate_proj", "up_proj", "down_proj"],
        lora_dropout=0.05,
        bias="none",
        task_type="CAUSAL_LM"
    )

    model = get_peft_model(model, peft_config)
    model.print_trainable_parameters()
    return model


# ---------------------------------------------------------------------------
# Step 3: Parse and package fine-tuning instructions dataset
# ---------------------------------------------------------------------------

def load_training_data(tokenizer):
    """Load and map training instances securely into text formats."""
    print("\n" + "=" * 60)
    print(f"STEP 3: Loading fine-tuning data from: {DATASET_PATH}")
    print("=" * 60)

    from datasets import load_dataset
    dataset = load_dataset("json", data_files=DATASET_PATH, split="train")

    def format_prompts(batch):
        formatted_texts = []
        for i in range(len(batch["input"])):
            text = (
                f"### Instruction:\n{batch['instruction'][i]}\n\n"
                f"### Input:\n{batch['input'][i]}\n\n"
                f"### Response:\n{batch['output'][i]}"
            )
            formatted_texts.append(text)
        return {"text": formatted_texts}

    dataset = dataset.map(format_prompts, batched=True)
    print(f"Successfully configured {len(dataset)} balanced training elements.")
    return dataset


# ---------------------------------------------------------------------------
# Step 4: Core Training Process Block
# ---------------------------------------------------------------------------

def train(model, tokenizer, dataset):
    """Execute training with strict optimizations against GPU memory faults."""
    print("\n" + "=" * 60)
    print("STEP 4: Initializing SFT Training Engine...")
    print("=" * 60)

    # Optimization config for Llama 3B/8B
    training_args = SFTConfig(
        output_dir=OUTPUT_DIR,
        per_device_train_batch_size=1,        # Prevents Out of Memory (OOM) faults on 8B scales
        gradient_accumulation_steps=4,       # Reconstitutes training footprint to batch size 4
        learning_rate=2e-4,
        logging_steps=5,
        num_train_epochs=3,
        
        save_strategy="no",                   # Do not save checkpoints
        
        fp16=True,                            # Mixed precision acceleration
        optim="paged_adamw_8bit",             # Shifts old tracking contexts out to CPU if VRAM caps overflow
        report_to="none",
        dataset_text_field="text",
        max_length=MAX_SEQ_LENGTH,
        packing=False                         # Keeps tokenized sequence vectors flat
    )

    trainer = SFTTrainer(
        model=model,
        train_dataset=dataset,
        processing_class=tokenizer,
        args=training_args,
    )

    print("\nBeginning execution loop...")
    trainer_stats = trainer.train()

    print("\n" + "=" * 60)
    print("                 TRAINING EXECUTION METRICS                  ")
    print("============================================================")
    print(f"  Total steps:   {trainer_stats.global_step}")
    print(f"  Runtime:       {trainer_stats.metrics['train_runtime']:.0f} seconds")
    print(f"  Final loss:    {trainer_stats.metrics['train_loss']:.4f}")
    print("=" * 60)

    return trainer


# ---------------------------------------------------------------------------
# Step 5: Save weights
# ---------------------------------------------------------------------------

def save_model(trainer, model, tokenizer):
    print("\n" + "=" * 60)
    print("STEP 5: Saving final fine-tuned model weights and state...")
    print(f"  Target Destination: {OUTPUT_DIR}")
    print("=" * 60)

    # Save the model via the trainer to ensure state is maintained
    trainer.save_model(OUTPUT_DIR)
    # Explicitly save the training state
    trainer.save_state()
    tokenizer.save_pretrained(OUTPUT_DIR)

    print("LoRA adapter checkpoints and trainer state written to disk successfully!")



def main():
    print("\n" + "=" * 60)
    print("  SOME/IP VHAL Log Parser — 8B Optimized Pipeline")
    print("  Using Hugging Face PEFT + QLoRA Memory Shifting")
    print("=" * 60 + "\n")

    model, tokenizer = load_model()
    model = apply_lora(model)
    dataset = load_training_data(tokenizer)
    trainer = train(model, tokenizer, dataset)
    save_model(trainer, model, tokenizer)

    print("\n" + "=" * 60)
    print("🎉 PIPELINE RUN COMPLETE!")
    print("============================================================\n")


if __name__ == "__main__":
    main()