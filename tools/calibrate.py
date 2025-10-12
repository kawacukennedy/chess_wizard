#!/usr/bin/env python3

import json
import numpy as np
from scipy.optimize import minimize
import argparse

def sigmoid(cp, k, offset):
    return 1 / (1 + np.exp(-k * (cp + offset) / 100.0))

def loss(params, cps, outcomes):
    k, offset = params
    preds = sigmoid(cps, k, offset)
    return -np.mean(outcomes * np.log(preds + 1e-9) + (1 - outcomes) * np.log(1 - preds + 1e-9))

def calibrate(cps, outcomes):
    # Initial guess
    initial_params = [0.0045, 0.0]
    result = minimize(loss, initial_params, args=(cps, outcomes), method='L-BFGS-B')
    return result.x

def main():
    parser = argparse.ArgumentParser(description='Calibrate win probability sigmoid parameters')
    parser.add_argument('--input', required=True, help='JSON file with list of {cp: int, outcome: 0/1}')
    parser.add_argument('--output', default='calib.json', help='Output JSON file')

    args = parser.parse_args()

    with open(args.input, 'r') as f:
        data = json.load(f)

    cps = np.array([d['cp'] for d in data])
    outcomes = np.array([d['outcome'] for d in data])

    k, offset = calibrate(cps, outcomes)

    calib = {'k': k, 'offset': offset}
    with open(args.output, 'w') as f:
        json.dump(calib, f, indent=2)

    print(f"Calibrated k={k}, offset={offset}")

if __name__ == '__main__':
    main()