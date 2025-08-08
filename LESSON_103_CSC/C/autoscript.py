import subprocess
import re

# --- Compile the C program ---
print("Compiling the C program...")
compile_command = "gcc CSC_main.c CSC_RGB_to_YCC_01.c CSC_YCC_to_RGB_01.c -o CSC"
try:
    subprocess.run(compile_command, shell=True, check=True)
    print("Compilation successful.")
except subprocess.CalledProcessError as e:
    print(f"Compilation failed with error: {e}")
    exit(1)

# --- Initialize data structures ---
num_runs = 1000
# A dictionary to hold the total runtime for each image size
total_runtimes = {}
# A dictionary to hold the count of runs for each image size (should be 1000 for each in this case)
run_counts = {}

# --- Run the program and process output ---
print(f"Running ./CSC {num_runs} times and calculating total runtimes...")
for i in range(num_runs):
    try:
        # Run the compiled program and capture its output
        result = subprocess.run("./CSC", capture_output=True, text=True, check=True)
        output = result.stdout
        
        # Use a regular expression to find all lines with 'Elapsed time'
        # The pattern captures the image size (e.g., 64x48) and the time (e.g., 0.0076)
        matches = re.findall(r"Elapsed time for image size (\d+x\d+): (\d+\.\d+) seconds", output)
        
        for image_size, runtime_str in matches:
            runtime = float(runtime_str)
            
            # Update the total runtime and run count for this specific image size
            total_runtimes[image_size] = total_runtimes.get(image_size, 0.0) + runtime
            run_counts[image_size] = run_counts.get(image_size, 0) + 1
            
    except subprocess.CalledProcessError as e:
        print(f"Execution failed on run {i+1} with error: {e}")
        break # Exit the loop if a run fails

# --- Calculate and print averages ---
print("\n--- Average Runtimes by Image Size ---")
if total_runtimes:
    for image_size, total_time in total_runtimes.items():
        count = run_counts[image_size]
        if count > 0:
            average_time = total_time / count
            print(f"Image Size {image_size}: Average Runtime = {average_time:.6f} seconds ({count} runs)")
else:
    print("No runtime data was collected.")