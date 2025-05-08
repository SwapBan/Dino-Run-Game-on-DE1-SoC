import tkinter as tk
import subprocess
import threading
import numpy as np
import time
from heartrate_monitor import HeartRateMonitor

# ===== Step 1: Read from MAX30102 =====
def get_avg_vitals(duration=10):
    hrm = HeartRateMonitor(print_raw=False, print_result=False)
    hrm.start_sensor()
    heart_rates = []
    start = time.time()
    while time.time() - start < duration:
        hr = hrm.bpm
        if hr > 0:
            heart_rates.append(hr)
        time.sleep(0.5)
    hrm.stop_sensor()
    spo2s = [96.7] * len(heart_rates)
    avg_hr = np.mean(heart_rates) if heart_rates else 0
    avg_spo2 = np.mean(spo2s) if spo2s else 0
    return round(avg_hr, 1), round(avg_spo2, 1)

# ===== Build standardized medical prompt =====
def build_prompt(hr, spo2, temperature=36.7):
    return f"Heart Rate: {hr:.1f} bpm, Oxygen Level: {spo2:.1f}%, Temperature: {temperature:.1f}\u00B0C. Please diagnose me and give me advice."

# ===== Query Ollama model =====
def query_llm(prompt):
    try:
        result = subprocess.run(
            ["ollama", "run", "med-diagnosis"],
            input=prompt.encode('utf-8'),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=300
        )
        if result.returncode != 0:
            err = result.stderr.decode('utf-8').strip() or 'Unknown error'
            return f"Error: {err}"
        out = result.stdout.decode('utf-8').strip()
        return out if out else "Error: empty response from model."
    except subprocess.TimeoutExpired:
        return "Error: Model request timed out after 30s."
    except Exception as e:
        return f"Error: {str(e)}"

# ===== Chat GUI & Interaction =====
root = tk.Tk()
root.title("Local Medical Assistant Chat")
root.geometry("600x700")

# Chat display
top_frame = tk.Frame(root)
top_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
chat_display = tk.Text(top_frame, state="disabled", wrap="word")
chat_display.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
scrollbar = tk.Scrollbar(top_frame, command=chat_display.yview)
scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
chat_display.config(yscrollcommand=scrollbar.set)

# User input & controls
bottom_frame = tk.Frame(root)
bottom_frame.pack(fill=tk.X, padx=10, pady=(0,10))
user_input = tk.Entry(bottom_frame, font=("Helvetica", 12))
user_input.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0,10))
user_input.bind("<Return>", lambda event: send_prompt(user_input.get().strip()))

send_button = tk.Button(bottom_frame, text="Send", font=("Helvetica", 12),
                        command=lambda: send_prompt(user_input.get().strip()))
send_button.pack(side=tk.LEFT)

vitals_button = tk.Button(bottom_frame, text="Read Vitals", font=("Helvetica", 12),
                          command=lambda: send_prompt(build_prompt(*get_avg_vitals())))
vitals_button.pack(side=tk.LEFT, padx=(10,0))

# ===== Functions =====

def disable_input():
    user_input.config(state="disabled")
    send_button.config(state="disabled")
    vitals_button.config(state="disabled")

def enable_input():
    user_input.config(state="normal")
    send_button.config(state="normal")
    vitals_button.config(state="normal")

# ===== Send prompt & lock input until response =====
def send_prompt(prompt):
    if not prompt:
        return
    user_input.delete(0, tk.END)
    chat_display.config(state="normal")
    chat_display.insert(tk.END, f"You: {prompt}\n")
    chat_display.insert(tk.END, "Assistant: Thinking...\n")
    chat_display.config(state="disabled")
    chat_display.see(tk.END)
    disable_input()

    def worker(p):
        response = query_llm(p)
        chat_display.config(state="normal")
        chat_display.delete("end-2l linestart", "end-2l lineend")
        chat_display.insert(tk.END, f"Assistant: {response}\n\n")
        chat_display.config(state="disabled")
        chat_display.see(tk.END)
        enable_input()

    threading.Thread(target=worker, args=(prompt,), daemon=True).start()

# ===== Initial greeting =====
chat_display.config(state="normal")
chat_display.insert(tk.END, "Assistant: Hello! Type a question or click 'Read Vitals' to send data.\n\n")
chat_display.config(state="disabled")

root.mainloop()
