"""Summarize actual navigation captures; reversals are evidence, not automatic failures."""
import argparse
import json
import re
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('log', type=Path)
parser.add_argument('--start', type=float, default=60)
parser.add_argument('--end', type=float, default=240)
args = parser.parse_args()
samples, contacts, captures = [], [], []
for line in args.log.read_text(encoding='utf-8-sig', errors='replace').splitlines():
    if 'LogDC: Navigation:' in line:
        sample = {k: float(v) for k, v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)', line)}
        if args.start <= sample['t'] <= args.end:
            samples.append(sample)
    if 'LogDC: Hull struck stone' in line:
        match = re.search(r'strength ([\d.]+).*ride time ([\d.]+)', line)
        if match and args.start <= float(match[2]) <= args.end:
            contacts.append({'time': float(match[2]), 'strength': float(match[1])})
    if 'LogDC: Capture state:' in line:
        captures.append(line.split('LogDC: ')[1])
reversals, target_changes = [], []
last_direction = 0
previous = None
for sample in samples:
    direction = 1 if sample['helm'] > .65 else -1 if sample['helm'] < -.65 else 0
    if direction and last_direction and direction != last_direction:
        reversals.append(sample['t'])
    if direction:
        last_direction = direction
    if previous and abs(sample['target'] - previous['target']) >= 1000:
        target_changes.append(sample['t'])
    previous = sample
double_reversals = [(a, b) for a, b in zip(reversals, reversals[1:]) if b - a <= 4]
print(json.dumps({
    'window': [args.start, args.end], 'samples': len(samples),
    'stone_contacts': contacts,
    'strong_helm_reversals': reversals,
    'successive_reversals_within_4s': double_reversals,
    'large_target_changes': target_changes,
    'capture_states': captures,
    'interpretation': 'Different random obstacle layouts are not a controlled A/B. Inspect reversals and captures; legitimate avoidance also reverses helm.'
}, indent=2))
