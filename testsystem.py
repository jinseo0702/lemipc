import unittest
import subprocess

def run_case(name):
  result = subprocess.run(["./lemipc", name])
  return result.returncode

class TestIPC(unittest.TestCase):

  def test_init_and_clear(self):
    self.assertEqual(run_case("test_init_and_clear"), 0)

  def test_check_fist_player(self):
    self.assertEqual(run_case("test_check_fist_player"), 0)

  def test_shm_player(self):
    self.assertEqual(run_case("test_shm_player"), 0)

  def test_sem_multi_process(self):
      """
      여러 프로세스를 동시에 돌려서 세마포어 / 공유메모리 race condition 확인
      """
      print("------------------test_sem------------------")
      WORKER_COUNT = 5
      # 0. 혹시 남아있을 IPC 자원을 먼저 정리 (있으면)
      run_case("test_sem_clear_ipcs")
      # 1. 워커 여러 개 동시에 실행
      procs = []
      for i in range(WORKER_COUNT):
          p = subprocess.Popen(["./lemipc", "test_sem"])
          procs.append(p)
      # 2. 전부 끝날 때까지 대기 + 리턴코드 체크
      for p in procs:
          p.wait()
          print("worker exit:", p.pid, p.returncode)
          self.assertEqual(p.returncode, 0)
      # 3. 끝난 후 정리
      run_case("test_sem_clear_ipcs")

  def test_msq_player(self):
    self.assertEqual(run_case("test_msq_player"), 0)


if __name__ == "__main__":
  unittest.main()