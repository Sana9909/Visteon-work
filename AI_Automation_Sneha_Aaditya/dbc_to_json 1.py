import cantools
import json
import sys
import os


def serialize_choices(choices):
    if choices is None:
        return {}
    return {int(k): str(v).strip() for k, v in choices.items()}

def create_sig_dict(msg_id_hex, sig):
    choices = serialize_choices(sig.choices)
    sig_dict = {
        "msgid": msg_id_hex,
        "start_byte": sig.start // 8,
        "start_bit": sig.start % 8,
        "length": sig.length,
        "choices": choices
    }
    if not choices:
        sig_dict["minimum"] = sig.minimum if getattr(sig, 'minimum', None) is not None else 0.0
        sig_dict["maximum"] = sig.maximum if getattr(sig, 'maximum', None) is not None else 0.0
        sig_dict["offset"] = getattr(sig, 'offset', 0.0)
        sig_dict["factor"] = getattr(sig, 'scale', 1.0)
    return sig_dict

def dbc_to_nested_json(dbc_path, json_path):
    db = cantools.database.load_file(dbc_path)
    result = {"rx": {}, "tx": {}}

    # Collect all node names
    all_nodes = set()
    for node in db.nodes:
        all_nodes.add(node.name)

    for msg in db.messages:
        msg_id_hex = f"{msg.frame_id:04X}"
        msg_name = msg.name
        # If no transmitter, treat as rx for all
        if msg.senders:
            for sender in msg.senders:
                if sender not in result["tx"]:
                    result["tx"][sender] = {}
                result["tx"][sender][msg_name] = {}
                for sig in msg.signals:
                    result["tx"][sender][msg_name][sig.name] = create_sig_dict(msg_id_hex, sig)
        # Receivers
        for receiver in msg.receivers:
            if receiver not in result["rx"]:
                result["rx"][receiver] = {}
            result["rx"][receiver][msg_name] = {}
            for sig in msg.signals:
                result["rx"][receiver][msg_name][sig.name] = create_sig_dict(msg_id_hex, sig)

    with open(json_path, "w") as f:
        json.dump(result, f, indent=4)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python dbc_to_json.py <input.dbc> <output.json>")
        sys.exit(1)
    dbc_to_nested_json(sys.argv[1], sys.argv[2])

