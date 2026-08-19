#!/usr/bin/env python3

import json
import os
import select
import signal
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
AUDIT = ROOT / "portfolio_audit"
RAW = AUDIT / "raw"
LEMIPC = ROOT / "lemipc"
TESTSYSTEM = ROOT / "testSystem"
IPC_PROBE = AUDIT / "bin" / "ipc_probe"
ALGORITHM_PROBE = AUDIT / "bin" / "algorithm_probe"
RECRUITMENT = 12
START_GAME = 13


class Baseline:
    def __init__(self):
        self.results = []
        self.current_log = []

    def log(self, value):
        if isinstance(value, (dict, list)):
            value = json.dumps(value, ensure_ascii=False, sort_keys=True)
        self.current_log.append(str(value))

    def command(self, args, timeout=5, input_data=None):
        args = [str(arg) for arg in args]
        self.log("$ " + " ".join(repr(arg) for arg in args))
        completed = subprocess.run(
            args,
            cwd=ROOT,
            input=input_data,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout,
        )
        self.log(f"returncode={completed.returncode}")
        if completed.stdout:
            self.log("stdout:\n" + completed.stdout.rstrip())
        if completed.stderr:
            self.log("stderr:\n" + completed.stderr.rstrip())
        return completed

    def probe(self, *args, timeout=5):
        result = self.command([IPC_PROBE, *args], timeout=timeout)
        if result.returncode != 0:
            raise RuntimeError(f"ipc_probe failed: {args}")
        if result.stdout.strip().startswith("{"):
            return json.loads(result.stdout.strip().splitlines()[-1])
        return result.stdout.strip()

    def snapshot(self):
        return self.probe("snapshot")

    def snapshot_quiet(self):
        result = subprocess.run(
            [str(IPC_PROBE), "snapshot"],
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=2,
        )
        if result.returncode != 0:
            raise RuntimeError(f"quiet snapshot failed: {result.stderr.strip()}")
        return json.loads(result.stdout.strip().splitlines()[-1])

    def cleanup(self):
        self.probe("cleanup")
        time.sleep(0.03)

    def spawn(self, *args, stdout=subprocess.PIPE, stderr=subprocess.PIPE):
        command = [str(arg) for arg in args]
        self.log("SPAWN " + " ".join(repr(arg) for arg in command))
        process = subprocess.Popen(
            command,
            cwd=ROOT,
            stdout=stdout,
            stderr=stderr,
        )
        self.log(f"pid={process.pid}")
        return process

    def wait_snapshot(self, predicate, timeout=4.0, label="state"):
        deadline = time.monotonic() + timeout
        last = None
        while time.monotonic() < deadline:
            last = self.snapshot_quiet()
            if predicate(last):
                self.log(f"reached {label}")
                self.log(last)
                return last
            time.sleep(0.03)
        self.log(f"timeout waiting for {label}; last={last}")
        raise TimeoutError(label)

    def wait_absent(self, timeout=5.0):
        return self.wait_snapshot(
            lambda snap: not snap["shm"] and not snap["sem"] and not snap["msg"],
            timeout=timeout,
            label="all IPC absent",
        )

    def communicate(self, process, timeout=5.0):
        try:
            stdout, stderr = process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            process.kill()
            stdout, stderr = process.communicate(timeout=2)
            self.log(f"pid={process.pid} exceeded timeout and was killed")
            raise
        if isinstance(stdout, bytes):
            stdout = stdout.decode(errors="replace")
        if isinstance(stderr, bytes):
            stderr = stderr.decode(errors="replace")
        self.log(f"pid={process.pid} returncode={process.returncode}")
        if stdout:
            self.log(f"pid={process.pid} stdout:\n{stdout.rstrip()}")
        if stderr:
            self.log(f"pid={process.pid} stderr:\n{stderr.rstrip()}")
        return process.returncode, stdout or "", stderr or ""

    def signal_and_wait(self, processes, sig=signal.SIGINT, timeout=7.0, first_only=True):
        living = [process for process in processes if process.poll() is None]
        targets = living[:1] if first_only else living
        for process in targets:
            self.log(f"signal {sig.name} -> pid={process.pid}")
            process.send_signal(sig)
        deadline = time.monotonic() + timeout
        for process in processes:
            remaining = max(0.05, deadline - time.monotonic())
            try:
                self.communicate(process, timeout=remaining)
            except subprocess.TimeoutExpired:
                self.force_kill(process)
        return [process.returncode for process in processes]

    def force_kill(self, process):
        if process.poll() is None:
            self.log(f"force SIGKILL -> pid={process.pid}")
            process.kill()
        try:
            self.communicate(process, timeout=2)
        except subprocess.TimeoutExpired:
            pass

    def force_cleanup(self, processes):
        for process in processes:
            self.force_kill(process)
        try:
            self.cleanup()
        except Exception as error:
            self.log(f"cleanup error: {error}")

    def record(self, case_id, name, classification, expected, actual, evidence_type="VERIFIED FROM RUNTIME TEST"):
        path = RAW / f"{case_id}.log"
        path.write_text("\n".join(self.current_log) + "\n", encoding="utf-8")
        self.results.append(
            {
                "id": case_id,
                "name": name,
                "classification": classification,
                "expected": expected,
                "actual": actual,
                "evidence_type": evidence_type,
                "raw": str(path.relative_to(ROOT)),
            }
        )
        print(f"{case_id} {classification:7} {name}")
        self.current_log = []

    def guarded(self, case_id, name, body):
        self.current_log = []
        try:
            classification, expected, actual = body()
        except subprocess.TimeoutExpired as error:
            self.log(f"target timeout: {error}")
            classification = "CRASH"
            expected = "bounded completion"
            actual = "process exceeded the case deadline"
        except TimeoutError as error:
            self.log(f"target state timeout: {error}")
            classification = "CRASH"
            expected = f"reach {error} within deadline"
            actual = "state was not reached"
        except Exception as error:
            self.log(f"HARNESS_ERROR {type(error).__name__}: {error}")
            classification = "HARNESS_ERROR"
            expected = "valid test setup and observation"
            actual = f"{type(error).__name__}: {error}"
        self.record(case_id, name, classification, expected, actual)

    def quick_cli(self, args, expected_fragment, case_id, name):
        def body():
            result = self.command([LEMIPC, *args], timeout=2)
            output = result.stdout + result.stderr
            passed = result.returncode != 0 and expected_fragment in output
            return (
                "PASS" if passed else "FAIL",
                f"nonzero exit and diagnostic containing {expected_fragment!r}; no IPC created",
                f"exit={result.returncode}, output={output.strip()!r}, ipc={self.snapshot()}",
            )

        self.guarded(case_id, name, body)

    def run(self):
        RAW.mkdir(parents=True, exist_ok=True)
        self.cleanup()

        self.guarded("B01", "build artifacts are current", self.case_build)
        self.quick_cli([], "Usage:", "B02", "missing argument is rejected")
        self.quick_cli(["abc"], "Invalid team number", "B03", "nondigit team is rejected")
        self.quick_cli(["2"], "Invalid team number", "B04", "out-of-range team is rejected")
        self.guarded("B05", "empty team string", self.case_empty_argument)
        self.guarded("B06", "viewmode prefix variant", self.case_viewmode_prefix)
        self.guarded("B07", "single-player initialization", self.case_single_player)
        self.guarded("B08", "simultaneous first-player race", self.case_creation_race)
        self.guarded("B09", "per-team capacity", self.case_capacity)
        self.guarded("B10", "three-player recruitment barrier", self.case_three_barrier)
        self.guarded("B11", "four-player start barrier", self.case_four_start)
        self.guarded("B12", "post-start admission gate", self.case_late_join)
        self.guarded("B13", "viewer lifecycle", self.case_viewer)
        self.guarded("B14", "SIGINT end propagation", self.case_sigint_propagation)
        self.guarded("B15", "repeated graceful cleanup", self.case_repeated_cleanup)
        self.guarded("B16", "SEM_UNDO lock-owner recovery", self.case_sem_undo)
        self.guarded("B17", "active-player SIGKILL", self.case_sigkill)
        self.guarded("B18", "active-player SIGTERM", self.case_sigterm)
        self.guarded("B19", "restart after abnormal exit", self.case_stale_restart)
        self.guarded("B20", "algorithm deterministic probes", self.case_algorithm)
        self.guarded("B21", "team message kind semantics", self.case_message_kind)
        self.guarded("B22", "global semaphore stress", self.case_sem_stress)
        self.guarded("B23", "message queue wrapper round trip", self.case_message_roundtrip)

        self.cleanup()
        self.write_results()

    def case_build(self):
        result = self.command(["make", "-q"], timeout=5)
        files = [LEMIPC.exists(), TESTSYSTEM.exists(), IPC_PROBE.exists(), ALGORITHM_PROBE.exists()]
        passed = result.returncode == 0 and all(files)
        return (
            "PASS" if passed else "FAIL",
            "make reports production target current and all four required binaries exist",
            f"make -q exit={result.returncode}, binaries={files}",
        )

    def case_empty_argument(self):
        self.cleanup()
        process = self.spawn(LEMIPC, "")
        time.sleep(0.35)
        accepted = process.poll() is None
        snap = self.snapshot()
        self.signal_and_wait([process])
        self.wait_absent()
        return (
            "FAIL" if accepted else "PASS",
            "empty string is rejected as a non-team argument",
            f"accepted={accepted}, snapshot={snap}",
        )

    def case_viewmode_prefix(self):
        self.cleanup()
        process = self.spawn(LEMIPC, "viewmode-extra")
        time.sleep(0.25)
        accepted = process.poll() is None
        if process.poll() is None:
            process.send_signal(signal.SIGINT)
        self.communicate(process, timeout=2)
        return (
            "FAIL" if accepted else "PASS",
            "only the exact documented token 'viewmode' enters viewer mode",
            f"prefix_variant_entered_viewer={accepted}, exit={process.returncode}",
        )

    def case_single_player(self):
        self.cleanup()
        process = self.spawn(LEMIPC, "0")
        try:
            snap = self.wait_snapshot(
                lambda s: s.get("players") == 1 and s.get("teams") == [1, 0],
                label="one admitted player",
            )
            passed = (
                snap["state"] == RECRUITMENT
                and snap["cells"] == {"A": 1, "B": 0, "empty": 99, "other": 0}
                and snap["qnum"] == 3
                and snap["semval"] == 1
            )
            codes = self.signal_and_wait([process])
            self.wait_absent()
            return (
                "PASS" if passed and codes == [0] else "FAIL",
                "one A player produces coherent recruitment state, consumes one of four positions, and cleans up after SIGINT",
                f"snapshot={snap}, exits={codes}",
            )
        finally:
            self.force_cleanup([process])

    def case_creation_race(self):
        self.cleanup()
        processes = [self.spawn(LEMIPC, "0"), self.spawn(LEMIPC, "1")]
        try:
            snap = self.wait_snapshot(
                lambda s: (
                    s.get("players") == 2
                    and s.get("teams") == [1, 1]
                    and s.get("cells", {}).get("A") == 1
                    and s.get("cells", {}).get("B") == 1
                ),
                label="two concurrently admitted players",
            )
            passed = snap["cells"]["A"] == 1 and snap["cells"]["B"] == 1 and snap["semval"] == 1
            codes = self.signal_and_wait(processes)
            self.wait_absent()
            return (
                "PASS" if passed and codes == [0, 0] else "FAIL",
                "concurrent creators converge on one coherent IPC set and unique cells",
                f"snapshot={snap}, exits={codes}",
            )
        finally:
            self.force_cleanup(processes)

    def case_capacity(self):
        self.cleanup()
        first = self.spawn(LEMIPC, "0")
        second = self.spawn(LEMIPC, "0")
        processes = [first, second]
        try:
            self.wait_snapshot(lambda s: s.get("teams") == [2, 0], label="team A full")
            third = self.spawn(LEMIPC, "0")
            code, _, stderr = self.communicate(third, timeout=2)
            snap = self.snapshot()
            passed = code == 1 and "Team 0 is full" in stderr and snap["players"] == 2 and snap["cells"]["A"] == 2
            codes = self.signal_and_wait(processes)
            self.wait_absent()
            return (
                "PASS" if passed and codes == [0, 0] else "FAIL",
                "third A player is rejected without count/board mutation",
                f"third_exit={code}, third_stderr={stderr.strip()!r}, snapshot={snap}, survivors={codes}",
            )
        finally:
            self.force_cleanup(processes)

    def case_three_barrier(self):
        self.cleanup()
        processes = [self.spawn(LEMIPC, "0"), self.spawn(LEMIPC, "0"), self.spawn(LEMIPC, "1")]
        try:
            snap = self.wait_snapshot(
                lambda s: s.get("players") == 3 and s.get("teams") == [2, 1],
                label="three admitted players",
            )
            time.sleep(0.25)
            alive = [process.poll() is None for process in processes]
            snap2 = self.snapshot()
            passed = snap["state"] == RECRUITMENT and snap2["state"] == RECRUITMENT and all(alive)
            codes = self.signal_and_wait(processes)
            self.wait_absent()
            return (
                "PASS" if passed and codes == [0, 0, 0] else "FAIL",
                "three players remain blocked in recruitment with no early start",
                f"snapshot={snap2}, alive={alive}, exits={codes}",
            )
        finally:
            self.force_cleanup(processes)

    def launch_four(self):
        processes = []
        for team in ["0", "0", "1", "1"]:
            processes.append(self.spawn(LEMIPC, team))
            time.sleep(0.02)
        snap = self.wait_snapshot(
            lambda s: s.get("state") == START_GAME and s.get("players") == 4,
            timeout=4,
            label="four-player START_GAME",
        )
        return processes, snap

    def case_four_start(self):
        self.cleanup()
        processes = []
        try:
            processes, snap = self.launch_four()
            passed = snap["teams"] == [2, 2] and snap["cells"]["A"] == 2 and snap["cells"]["B"] == 2
            codes = self.signal_and_wait(processes)
            self.wait_absent()
            return (
                "PASS" if passed and codes == [0, 0, 0, 0] else "FAIL",
                "fourth valid player changes state to START_GAME with coherent counts/cells",
                f"snapshot={snap}, exits={codes}",
            )
        finally:
            self.force_cleanup(processes)

    def case_late_join(self):
        self.cleanup()
        self.probe("create", str(START_GAME), "1", "1", "0")
        process = self.spawn(LEMIPC, "1")
        try:
            code, _, stderr = self.communicate(process, timeout=2)
            snap = self.snapshot()
            passed = code == 1 and snap["players"] == 1 and snap["teams"] == [1, 0]
            return (
                "PASS" if passed else "FAIL",
                "admission during START_GAME is rejected without shared counter mutation",
                f"exit={code}, stderr={stderr.strip()!r}, snapshot={snap}",
            )
        finally:
            self.force_cleanup([process])

    def case_viewer(self):
        observations = []
        for iteration in range(1, 9):
            self.cleanup()
            viewer_out = RAW / f"B13.viewer.{iteration:02d}.stdout"
            viewer_err = RAW / f"B13.viewer.{iteration:02d}.stderr"
            viewer = None
            player = None
            snap = None
            codes = None
            try:
                with viewer_out.open("wb") as stdout_file, viewer_err.open("wb") as stderr_file:
                    viewer = self.spawn(LEMIPC, "viewmode", stdout=stdout_file, stderr=stderr_file)
                    time.sleep(0.12)
                    player = self.spawn(LEMIPC, "0")
                    snap = self.wait_snapshot(
                        lambda s: s.get("players") == 1,
                        label=f"viewer iteration {iteration} player",
                    )
                    time.sleep(0.12)
                    codes = self.signal_and_wait([player])
                    after_player_exit = self.snapshot()
                    stuck = after_player_exit["shm"]
                    if not stuck:
                        self.communicate(viewer, timeout=2)
                    output = viewer_out.read_text(encoding="utf-8", errors="replace")
                    rendered = "Game Start" in output and "=== Board State (Players: 1) ===" in output
                    observations.append(
                        {
                            "iteration": iteration,
                            "player_snapshot": snap,
                            "player_exits": codes,
                            "after_player_exit": after_player_exit,
                            "viewer_stuck": stuck,
                            "rendered": rendered,
                            "viewer_exit": viewer.returncode,
                        }
                    )
            finally:
                if player is not None:
                    self.force_kill(player)
                if viewer is not None:
                    self.force_kill(viewer)
                self.cleanup()
        stuck_count = sum(item["viewer_stuck"] for item in observations)
        rendered_all = all(item["rendered"] for item in observations)
        if stuck_count:
            return (
                "CRASH",
                "viewer exits after each last player removes SHM; no graceful run leaves IPC",
                f"8 iterations: viewer/IPC stuck after {stuck_count}; rendered_all={rendered_all}; observations={observations}",
            )
        passed = rendered_all and all(item["player_exits"] == [0] for item in observations)
        return (
            "PASS" if passed else "FAIL",
            "viewer renders and exits after SHM removal in all 8 iterations",
            f"observations={observations}",
        )

    def case_sigint_propagation(self):
        self.cleanup()
        processes = []
        try:
            processes, before = self.launch_four()
            codes = self.signal_and_wait(processes, sig=signal.SIGINT, first_only=True)
            after = self.wait_absent()
            passed = codes == [0, 0, 0, 0]
            return (
                "PASS" if passed else "FAIL",
                "SIGINT to one active player propagates END_GAME, all four exit 0, all IPC disappears",
                f"before={before}, exits={codes}, after={after}",
            )
        finally:
            self.force_cleanup(processes)

    def case_repeated_cleanup(self):
        observations = []
        processes = []
        try:
            for iteration in range(3):
                self.cleanup()
                process = self.spawn(LEMIPC, "0")
                processes = [process]
                snap = self.wait_snapshot(lambda s: s.get("players") == 1, label=f"iteration {iteration + 1} admission")
                codes = self.signal_and_wait([process])
                absent = self.wait_absent()
                observations.append({"iteration": iteration + 1, "snapshot": snap, "exits": codes, "after": absent})
                processes = []
            passed = all(item["exits"] == [0] and not item["after"]["shm"] for item in observations)
            return (
                "PASS" if passed else "FAIL",
                "three sequential graceful single-player runs start from one player and leave no IPC",
                observations,
            )
        finally:
            self.force_cleanup(processes)

    def case_sem_undo(self):
        self.cleanup()
        self.probe("create", str(RECRUITMENT), "0", "0", "0")
        holder = self.spawn(IPC_PROBE, "hold-lock", "5000")
        try:
            ready, _, _ = select.select([holder.stdout], [], [], 2)
            if not ready:
                raise TimeoutError("lock acquisition")
            line = holder.stdout.readline().decode(errors="replace").strip()
            self.log(f"holder line={line!r}")
            locked = self.snapshot()
            holder.kill()
            self.communicate(holder, timeout=2)
            recovered = self.wait_snapshot(lambda s: s.get("semval") == 1, timeout=2, label="SEM_UNDO restoration")
            passed = line == "LOCKED" and locked["semval"] == 0 and recovered["semval"] == 1
            return (
                "PASS" if passed else "FAIL",
                "killing the lock owner restores semaphore value from 0 to 1 via SEM_UNDO",
                f"locked={locked}, holder_exit={holder.returncode}, recovered={recovered}",
            )
        finally:
            self.force_cleanup([holder])

    def abnormal_exit(self, sig):
        self.cleanup()
        process = self.spawn(LEMIPC, "0")
        try:
            before = self.wait_snapshot(lambda s: s.get("players") == 1, label="active player before abnormal exit")
            process.send_signal(sig)
            self.communicate(process, timeout=2)
            time.sleep(0.15)
            after = self.snapshot()
            leftovers = after["shm"] and after["sem"] and after["msg"] and after.get("players") == 1
            return before, after, process.returncode, leftovers
        finally:
            self.force_kill(process)

    def case_sigkill(self):
        try:
            before, after, code, leftovers = self.abnormal_exit(signal.SIGKILL)
            return (
                "PARTIAL" if leftovers else "PASS",
                "measure whether SIGKILL preserves semaphore progress but removes/reconciles logical player state and IPC",
                f"before={before}, exit={code}, after={after}; IPC and ghost player remain={leftovers}",
            )
        finally:
            self.cleanup()

    def case_sigterm(self):
        try:
            before, after, code, leftovers = self.abnormal_exit(signal.SIGTERM)
            return (
                "PARTIAL" if leftovers else "PASS",
                "SIGTERM should not leave an unrecoverable logical player/IPC state",
                f"before={before}, exit={code}, after={after}; IPC and ghost player remain={leftovers}",
            )
        finally:
            self.cleanup()

    def case_stale_restart(self):
        self.cleanup()
        first = self.spawn(LEMIPC, "0")
        second = None
        try:
            before = self.wait_snapshot(lambda s: s.get("players") == 1, label="first player")
            first.kill()
            self.communicate(first, timeout=2)
            stale = self.snapshot()
            second = self.spawn(LEMIPC, "0")
            joined = self.wait_snapshot(lambda s: s.get("players") == 2, label="new process joined stale run")
            live = int(second.poll() is None)
            ghost_mismatch = joined["players"] == 2 and live == 1
            codes = self.signal_and_wait([second])
            self.wait_absent()
            return (
                "PARTIAL" if ghost_mismatch else "PASS",
                "a new run should detect or reconcile stale ownership after abnormal exit",
                f"before={before}, stale={stale}, joined={joined}, live_processes={live}, exits={codes}",
            )
        finally:
            self.force_cleanup([process for process in [first, second] if process is not None])

    def case_algorithm(self):
        cases = [
            "distance",
            "greedy_y_tie",
            "greedy_x",
            "collision",
            "random_step",
            "surrounded_horizontal",
            "surrounded_mixed",
            "death_update",
            "out_of_radius",
        ]
        exits = {}
        for case in cases:
            result = self.command([ALGORITHM_PROBE, case], timeout=2)
            exits[case] = result.returncode
        passed = all(code == 0 for code in exits.values())
        return (
            "PASS" if passed else "FAIL",
            "nine deterministic direct probes match distance, greedy, random-step, collision, surrounded, death, and fallback rules",
            exits,
        )

    def case_message_kind(self):
        result = self.command([ALGORITHM_PROBE, "message_kind"], timeout=2)
        data = json.loads(result.stdout.strip())
        ignored = data["kind"] == 14 and data["track"] == 1
        return (
            "PARTIAL" if ignored else "PASS",
            "DEATH and chase/BROADCAST messages have distinct receive-side semantics",
            f"probe={data}; DEATH was converted into a chase target={ignored}",
        )

    def case_sem_stress(self):
        self.cleanup()
        processes = [self.spawn(TESTSYSTEM, "test_sem") for _ in range(5)]
        try:
            codes = []
            for process in processes:
                code, _, _ = self.communicate(process, timeout=6)
                codes.append(code)
            snap = self.snapshot()
            passed = codes == [0, 0, 0, 0, 0] and snap.get("players") == 1000 and snap.get("semval") == 1
            return (
                "PASS" if passed else "FAIL",
                "five helper processes converge on exactly 1000 under the global semaphore and leave it unlocked",
                f"exits={codes}, snapshot={snap}",
            )
        finally:
            self.force_cleanup(processes)

    def case_message_roundtrip(self):
        self.cleanup()
        try:
            result = self.command([TESTSYSTEM, "test_msq_player"], timeout=5)
            passed = result.returncode == 0 and "mytype = 10 x = 0 y = 1" in result.stdout
            snap = self.snapshot()
            return (
                "PASS" if passed else "FAIL",
                "forked sender/receiver transfers the complete typed message payload",
                f"exit={result.returncode}, output={result.stdout.strip()!r}, snapshot={snap}",
            )
        finally:
            self.cleanup()

    def write_results(self):
        totals = {key: 0 for key in ["PASS", "PARTIAL", "FAIL", "CRASH", "HARNESS_ERROR"]}
        for result in self.results:
            totals[result["classification"]] += 1
        payload = {
            "commit": "108c4fc68c90e41723fe2ce35d6cb4b443524ed2",
            "profile": "PROFILE C (lemipc), resolving the supplied PROFILE B label conflict",
            "generated_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
            "totals": totals,
            "tests": self.results,
        }
        (AUDIT / "test_results.json").write_text(
            json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
        lines = [
            "# Baseline Test Results",
            "",
            f"Commit: `{payload['commit']}`",
            "",
            "## Totals",
            "",
            "| PASS | PARTIAL | FAIL | CRASH | HARNESS_ERROR | Total |",
            "|---:|---:|---:|---:|---:|---:|",
            f"| {totals['PASS']} | {totals['PARTIAL']} | {totals['FAIL']} | {totals['CRASH']} | {totals['HARNESS_ERROR']} | {len(self.results)} |",
            "",
            "## Cases",
            "",
            "| ID | Classification | Case | Expected | Actual | Raw |",
            "|---|---|---|---|---|---|",
        ]
        for result in self.results:
            expected = str(result["expected"]).replace("|", "\\|").replace("\n", " ")
            actual = str(result["actual"]).replace("|", "\\|").replace("\n", " ")
            lines.append(
                f"| {result['id']} | {result['classification']} | {result['name']} | {expected} | {actual} | `{result['raw']}` |"
            )
        lines.extend(
            [
                "",
                "All case outcomes are `VERIFIED FROM RUNTIME TEST` except interpretive scope labels described in `failures.md`.",
            ]
        )
        (AUDIT / "test_results.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    os.environ.setdefault("LC_ALL", "C")
    runner = Baseline()
    runner.run()
    if any(result["classification"] == "HARNESS_ERROR" for result in runner.results):
        sys.exit(2)
