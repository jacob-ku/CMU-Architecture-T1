#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse

def parse_args():
    parser = argparse.ArgumentParser(
        description="Merge timestamp lines with CSV lines into single CSV records."
    )
    parser.add_argument(
        "input_file",
        help="Path to the input file containing alternating timestamp and CSV lines."
    )
    parser.add_argument(
        "output_file",
        help="Path to the output CSV file where merged results will be saved."
    )
    return parser.parse_args()

def merge_timestamp_and_csv(input_path, output_path):
    with open(input_path, "r", encoding="utf-8") as fin, \
         open(output_path, "w", encoding="utf-8") as fout:

        timestamp = None
        for raw_line in fin:
            line = raw_line.strip()
            if not line:
                continue  # Skip empty lines

            # If the line contains no commas and is all digits, treat it as a timestamp
            if "," not in line and line.isdigit():
                timestamp = line
                continue

            # Otherwise, treat it as a CSV line
            if timestamp is not None:
                # Remove any leading commas or spaces from the CSV part
                csv_part = line.lstrip(", ").rstrip()
                merged = f"{timestamp},{csv_part}"
                fout.write(merged + "\n")
                timestamp = None
            else:
                # No preceding timestamp; write the CSV line as-is
                fout.write(line + "\n")

def main():
    args = parse_args()
    merge_timestamp_and_csv(args.input_file, args.output_file)
    print(f"Conversion complete: '{args.output_file}' has been created.")

if __name__ == "__main__":
    main()
