from re import I
import subprocess
import time
import sys
import os
import signal
from tkinter import NO

TEAM = 10
PLAYER_PER_TEAM = 20
GAME_EXE = "./lemipc"
TOOL_EXE = "./testSystem"

running_process = []

def clean_ipc_resources():
	subprocess.run([TOOL_EXE, "test_sem_clear_ipcs"])

def launch_processes():
	"""플레이어 프로세스들을 생성하고 실행"""
	print(f"[*] Launching {TEAM} teams x {PLAYER_PER_TEAM} players...")

	for team_id in range(0, TEAM):
		for _ in range(PLAYER_PER_TEAM):
			proc = subprocess.Popen([GAME_EXE, str(team_id)])
			running_process.append(proc)

	print(f"[*] Total {len(running_process)} players launched.")

def monitor_loop():
  """loop 를 돌면서 보드 상태를 출력"""
  print("init start")
  time.sleep(0.2)
  print("init clear")
  while True:
    subprocess.run([TOOL_EXE, "view"])
    alive = sum(1 for p in running_process if p.poll() is None)
    print(f"\n[Alive Processes : {alive}/{len(running_process)}]")
    time.sleep(0.5)

def cleanup_and_exit(sig=None, frame=None):
	print("\n\n[!] Stopping simulation...")
	for p in running_process:
		if p.poll() is None:
			p.terminate()
	
	time.sleep(0.2)
	for p in running_process:
		if p.poll() is None:
			p.kill()
	
	clean_ipc_resources()
	sys.exit(0)

def main():
	signal.signal(signal.SIGINT, cleanup_and_exit)
	clean_ipc_resources()
	launch_processes()
	try:
		monitor_loop()
	except Exception as e:
		print(f"[!] Error: {e}")
		cleanup_and_exit()

if __name__ == "__main__":
  main()



