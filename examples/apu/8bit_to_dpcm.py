import sys
import argparse
from pathlib import Path

def convert_pcm_to_dpcm(input_path, output_path, step_size=4):
    """
    Converts an unsigned 8-bit raw PCM file into a 1-bit DPCM file.
    
    Parameters:
    - input_path: Path to the raw .raw or .bin 8-bit file.
    - output_path: Path to save the compiled .dmc or .dpcm file.
    - step_size: How aggressively the waveform level scales up/down per bit.
    """
    try:
        with open(input_path, "rb") as f:
            pcm_samples = f.read()
    except FileNotFoundError:
        print(f"Error: The file '{input_path}' was not found.")
        return
    smin = smax = 128
    dpcm_bytes = bytearray()
    current_byte = 0
    bit_count = 0
    
    # Initialize the internal tracker to mid-range (128 for unsigned 8-bit)
    predictor = 128 
    
    for sample in pcm_samples:
        if sample > smax: smax = sample
        if sample < smin: smin = sample
        # Determine if the wave is moving up or down
        if sample >= predictor:
            bit = 1
            predictor = min(255, predictor + step_size)
        else:
            bit = 0
            predictor = max(0, predictor - step_size)
            
        # Pack 8 individual 1-bit samples into a single output byte (LSB first)
        current_byte |= (bit << bit_count)
        bit_count += 1
        
        # When the byte is full (8 bits), save it and clear the buffer
        if bit_count == 8:
            dpcm_bytes.append(current_byte)
            current_byte = 0
            bit_count = 0
            
    # Handle any remaining trailing bits
    if bit_count > 0:
        dpcm_bytes.append(current_byte)
        
    with open(output_path, "wb") as f:
        f.write(dpcm_bytes)
        
    print(f"Success! Converted {len(pcm_samples)} samples into {len(dpcm_bytes)} DPCM bytes. (min={smin}, max={smax})")


cli_parser = argparse.ArgumentParser(description='Convert raw uint8 samples to dpcm')
cli_parser.add_argument('src', metavar='<raw-uint8-source>',
                    type=Path,
                    help='the input file')
cli_parser.add_argument('-o', '--output', metavar='<dpcm-dest>',
                    dest='dst',
                    type=Path,
                    help='the output file; if omitted, any file extension is dropped and .dmc is added')
cli_parser.add_argument('-s', '--step-size', metavar='<int>',
                    dest='step_size',
                    default=4,
                    type=int,
                    help='the level width for determining up vs. down waveform movement (default: 4)')

cli_args = cli_parser.parse_args()
if not cli_args.dst:
    cli_args.dst = cli_args.src.with_suffix('.dmc')
# Run conversion (Step size 4 yields a solid baseline for NES-style audio)
convert_pcm_to_dpcm(cli_args.src, cli_args.dst, step_size=cli_args.step_size)
