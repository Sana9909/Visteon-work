# SOME/IP VHAL Log Parser — Fine-Tuned LLM

A fine-tuned language model that parses SOME/IP vehicle log lines into structured JSON,
running entirely offline after initial setup.

---

## 📋 Prerequisites

| Requirement | Your System | Status |
|-------------|------------|--------|
| NVIDIA GPU (≥8 GB VRAM) | RTX PRO 2000 (8 GB) | ✅ |
| Python ≥ 3.10 | 3.14.4 | ✅ |
| PyTorch | 2.10.0 | ✅ |
| Unsloth | 2026.5.8 | ✅ |
| Transformers | 5.5.0 | ✅ |
| PEFT | 0.19.1 | ✅ |
| TRL | 0.24.0 | ✅ |
| Datasets | 4.3.0 | ✅ |

All packages are already installed on your system. No additional installation needed.

---

## 🚀 Step-by-Step Guide

### Step 1: Generate Training Data (Offline)

Your 6 real examples are already in `data/train_data.jsonl`. Run the augmentation
script to add 500 synthetic examples:

```powershell
cd c:\Sneha\AI_Automation
python data/augment_data.py
```

**Expected output:**
```
Existing examples: 6
Generating 500 synthetic examples...
Done! Total examples in dataset: 506
```

> **Note:** You can edit `NUM_EXAMPLES` in `data/augment_data.py` to generate more
> or fewer examples. More examples = better accuracy (up to ~2000).

---

### Step 2: Fine-Tune the Model (⚠️ Requires Internet FIRST TIME ONLY)

```powershell
python train.py
```

**What happens:**
1. **First run (online):** Downloads the base model `Llama-3.2-1B-Instruct` (~1.5 GB)
   from HuggingFace. This is cached locally — never downloaded again.
2. **Applies QLoRA:** Freezes the base model, adds tiny trainable adapters (~2% of parameters)
3. **Trains:** Runs through your dataset for 5 epochs
4. **Saves:** Stores the fine-tuned model to `output/vhal_parser_model/`

**Expected output:**
```
STEP 1: Loading base model...
  Model: unsloth/Llama-3.2-1B-Instruct-bnb-4bit
  Model loaded successfully on cuda:0

STEP 2: Applying LoRA adapters...
  Trainable parameters: 6,815,744 / 1,242,813,440 (0.55%)

STEP 3: Loading training data...
  Total examples: 506

STEP 4: Starting fine-tuning...
  {'train_loss': 0.xxxx, ...}

TRAINING COMPLETE!
  Total training time: ~900 seconds
  Final loss: ~0.1xxx
```

**⏱ Estimated time:** 15–30 minutes on your RTX PRO 2000

> **After this step, everything is offline!** The model weights are saved locally.
> You never need internet again.

---

### Step 3: Run Inference (Fully Offline)

#### Option A: Interactive Mode
```powershell
python inference.py
```

Then paste a log line:
```
📥 Paste log line: 08-12 22:35:32.261   463   476 D vendor.visteon.skylark.hardware.automotive.vehicle@V1-visteon-service: SomeipCOmm :: RX Message [0/2330]:service:9e83 ,Method:840f,payload:ea eb 48 db 5d 38 d6 6b

⏳ Parsing...

📤 Parsed output:
{
  "time": "08-12 22:35:32.261",
  "type": "rx",
  "serviceId": 40579,
  "methodId": 33807,
  "payload": "ea eb 48 db 5d 38 d6 6b"
}
```

Type `quit` to exit.

#### Option B: Single Log Line
```powershell
python inference.py --input "08-12 22:35:32.261   463   476 D vendor.visteon.skylark.hardware.automotive.vehicle@V1-visteon-service: SomeipCOmm :: RX Message [0/2330]:service:9e83 ,Method:840f,payload:ea eb 48 db"
```

#### Option C: Batch Process a File
```powershell
python inference.py --file my_logs.txt --output parsed_results.json
```

Where `my_logs.txt` has one log line per line.

---

### Step 4: Validate Accuracy (Optional)

```powershell
python test_model.py
```

Runs 3 test cases with known correct answers and reports pass/fail.

---

## 📁 Project Structure

```
c:\Sneha\AI_Automation\
├── data/
│   ├── train_data.jsonl      ← Training dataset (6 real + 500 synthetic)
│   └── augment_data.py       ← Data generation script
├── train.py                  ← Fine-tuning script (run once)
├── inference.py              ← Parse log lines (run anytime, offline)
├── test_model.py             ← Accuracy validation
├── output/                   ← Created after training
│   ├── vhal_parser_model/    ← LoRA adapter weights
│   └── vhal_parser_merged/   ← Full merged model (for inference)
└── README.md                 ← This file
```

---

## ⚙️ Tuning Tips

| If you see... | Try... |
|---------------|--------|
| Low accuracy | Increase `NUM_EXAMPLES` to 1000+ in `augment_data.py`, re-run augmentation and training |
| High loss (>0.5) | Increase `NUM_EPOCHS` to 10 in `train.py` |
| Out of memory (OOM) | Reduce `BATCH_SIZE` to 1 in `train.py` |
| Slow training | Reduce `NUM_EXAMPLES` to 200 |
| Model outputs garbage | Check that training loss decreased; ensure dataset is correct |

---

## 🔒 Offline Guarantee

After Step 2 (first run), **all model weights are cached locally** at:
```
C:\Users\<username>\.cache\huggingface\hub\
```

Steps 1, 3, and 4 **never require internet**. You can disconnect after the first
training run and everything continues to work.

---

## 📊 What the Model Learns

| Input Field | Extraction | Example |
|-------------|-----------|---------|
| Timestamp | First 21 chars | `08-12 22:35:32.261` |
| Direction | `RX` or `TX` after `::` | `rx` |
| Service ID | Hex after `service:` → decimal (RX) | `9e83` → `40579` |
| Method ID | Hex after `Method:` → decimal (RX) | `840f` → `33807` |
| Payload | Hex bytes after `payload:` | `ea eb 48 db ...` |
