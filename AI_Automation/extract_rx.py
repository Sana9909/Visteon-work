import os

def extract_rx_messages(input_file_path, output_file_path):
    # Check if the input file exists
    if not os.path.exists(input_file_path):
        print(f"Error: The file '{input_file_path}' was not found.")
        return

    count = 0
    try:
        # Open input log file for reading and output file for writing
        # using utf-8 encoding to prevent formatting/character distortion
        with open(input_file_path, 'r', encoding='utf-8', errors='ignore') as infile, \
             open(output_file_path, 'w', encoding='utf-8') as outfile:
            
            for line in infile:
                # Use 'in' comparison to match the exact string RX Message'
                # if you want case-insensitive matching, change to: if "rx message" in line.lower():
                if "RX Message" in line:
                    outfile.write(line)
                    count += 1
                    
        print(f"Extraction successful! Found {count} lines matching 'RX Message'.")
        print(f"Results saved to: {output_file_path}")

    except Exception as e:
        print(f"An error occurred while processing the file: {e}")

if __name__ == "__main__":
    # Configure your file paths here
    INPUT_LOG = ".\\data\\ucl_90 1.log"
    OUTPUT_LOG = "extracted_rx_messages.log"
    
    extract_rx_messages(INPUT_LOG, OUTPUT_LOG)