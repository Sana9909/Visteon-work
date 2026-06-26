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
# BASE_MODEL = "C:/Sneha/models/Llama-3.2-3B-Instruct-bnb-4bit" 
# OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "output", "llama-3.2-3b-instruct-bnb-4bit", "vhal_parser_model")
# DATASET_PATH = "C:/Sneha/AI_Automation/llm_fine_tuning_dataset.json"
BASE_MODEL = "C:/Sneha/models/llama3.2-1B" 
OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "output", "llama3.2-1B", "vhal_parser_model")
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
        per_device_train_batch_size=2,        # Prevents Out of Memory (OOM) faults on 8B scales
        gradient_accumulation_steps=4,       # Reconstitutes training footprint to batch size 4
        learning_rate=2e-4,
        logging_steps=5,
        num_train_epochs=5,
        
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