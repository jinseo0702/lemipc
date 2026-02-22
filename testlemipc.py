import subprocess
import time
import sys
import signal

TEAM = 3
PLAYER_PER_TEAM = 5
GAME_EXE = "./lemipc"
TOOL_EXE = "./testSystem"

running_process = []

def clean_ipc_resources():
	subprocess.run([TOOL_EXE, "test_sem_clear_ipcs"])

def launch_processes():
	print(f"[*] Launching {TEAM} teams x {PLAYER_PER_TEAM} players...")
	for team_id in range(TEAM):
		for _ in range(PLAYER_PER_TEAM):
			proc = subprocess.Popen([GAME_EXE, str(team_id)])
			running_process.append(proc)
			time.sleep(0.01)
	print(f"[*] Total {len(running_process)} players launched.")

def monitor_loop():
    print("\033[2J", end="")      # 시작 시 1회만 전체 clear
    print("[*] Monitoring started (Ctrl+C to stop)")
    while True:
        alive = sum(1 for p in running_process if p.poll() is None)
        print("\033[H", end="")    # 매 프레임 커서만 맨 위로
        subprocess.run([TOOL_EXE, "view"])
        print(f"[Alive: {alive}/{len(running_process)}]")
        time.sleep(0.01) 
        if alive == 0:
            break;
        time.sleep(0.1)

def cleanup_and_exit(sig=None, frame=None):
	print("\n\n[!] Stopping simulation...")
	for p in running_process:
		if p.poll() is None:
			p.terminate()
	time.sleep(0.3)
	for p in running_process:
		if p.poll() is None:
			p.kill()
	for p in running_process:
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
