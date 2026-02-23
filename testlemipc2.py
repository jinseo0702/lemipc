import subprocess
import time
import sys
import signal
import threading

TEAM = 2
PLAYER_PER_TEAM = 2
GAME_EXE = "./lemipc"
TOOL_EXE = "./testSystem"

running_process = []  # list of (proc, label)

def clean_ipc_resources():
	subprocess.run([TOOL_EXE, "test_sem_clear_ipcs"])

def stream_stderr(proc, label):
	for line in proc.stderr:
		print(f"[ERR {label} PID={proc.pid}] {line.decode().rstrip()}", flush=True)

def launch_processes():
	print(f"[*] Launching {TEAM} teams x {PLAYER_PER_TEAM} players...")
	for team_id in range(TEAM):
		for player_id in range(PLAYER_PER_TEAM):
			proc = subprocess.Popen([GAME_EXE, str(team_id)], stderr=subprocess.PIPE)
			label = f"team{team_id}-p{player_id}"
			t = threading.Thread(target=stream_stderr, args=(proc, label), daemon=True)
			t.start()
			running_process.append((proc, label))
			time.sleep(0.01)
	print(f"[*] Total {len(running_process)} players launched.")

def monitor_loop():
    print("\033[2J", end="")
    print("[*] Monitoring started (Ctrl+C to stop)")
    while True:
        alive = sum(1 for p, _ in running_process if p.poll() is None)
        print("\033[H", end="")
        print(f"[Alive: {alive}/{len(running_process)}]")
        for p, label in running_process:
            ret = p.poll()
            if ret is not None and ret != 0:
                print(f"[DEAD] {label} PID={p.pid} exited with code {ret}")
        time.sleep(0.01)
        if alive == 0:
            break
        time.sleep(0.1)

def cleanup_and_exit(sig=None, frame=None):
	print("\n\n[!] Stopping simulation...")
	for p, _ in running_process:
		if p.poll() is None:
			p.terminate()
	time.sleep(0.3)
	for p, _ in running_process:
		if p.poll() is None:
			p.kill()
	for p, _ in running_process:
		p.wait()
	print("[*] All processes cleaned up.")
	clean_ipc_resources()
	sys.exit(0)

def main():
	signal.signal(signal.SIGINT, cleanup_and_exit)
	clean_ipc_resources()
	launch_processes()
	try:
		monitor_loop()
	except Exception as e:
		print(f"\n[!] Error: {e}")
		cleanup_and_exit()

if __name__ == "__main__":
	main()
