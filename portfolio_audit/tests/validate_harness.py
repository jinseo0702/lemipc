#!/usr/bin/env python3

import copy
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
AUDIT = ROOT / "portfolio_audit"


def counts_consistent(snapshot):
    return snapshot["players"] == sum(snapshot["teams"])


def board_consistent(snapshot):
    return (
        snapshot["cells"]["A"] == snapshot["teams"][0]
        and snapshot["cells"]["B"] == snapshot["teams"][1]
        and sum(snapshot["cells"].values()) == 100
    )


def cleanup_complete(snapshot):
    return not snapshot["shm"] and not snapshot["sem"] and not snapshot["msg"]


def semaphore_recovered(snapshot):
    return snapshot["sem"] and snapshot["semval"] == 1


def within_deadline(observation):
    return observation["completed"] and observation["elapsed"] <= observation["deadline"]


def main():
    stable = {
        "players": 4,
        "teams": [2, 2],
        "cells": {"A": 2, "B": 2, "empty": 96, "other": 0},
        "shm": True,
        "sem": True,
        "msg": True,
        "semval": 1,
    }
    absent = {"shm": False, "sem": False, "msg": False}
    deadline = {"completed": True, "elapsed": 0.8, "deadline": 1.0}

    mutations = []

    bad = copy.deepcopy(stable)
    bad["players"] = 5
    mutations.append(("HV01", "wrong global count", counts_consistent, stable, bad))

    bad = copy.deepcopy(stable)
    bad["cells"]["B"] = 1
    bad["cells"]["empty"] = 97
    mutations.append(("HV02", "missing board occupant", board_consistent, stable, bad))

    bad = copy.deepcopy(absent)
    bad["shm"] = True
    mutations.append(("HV03", "leftover shared memory", cleanup_complete, absent, bad))

    bad = copy.deepcopy(stable)
    bad["semval"] = 0
    mutations.append(("HV04", "semaphore remains locked", semaphore_recovered, stable, bad))

    bad = copy.deepcopy(deadline)
    bad["completed"] = False
    bad["elapsed"] = 1.5
    mutations.append(("HV05", "deadline exceeded", within_deadline, deadline, bad))

    results = []
    for case_id, name, checker, valid, invalid in mutations:
        accepts_valid = bool(checker(valid))
        rejects_invalid = not bool(checker(invalid))
        result = {
            "id": case_id,
            "name": name,
            "accepts_valid": accepts_valid,
            "rejects_invalid": rejects_invalid,
            "classification": "PASS" if accepts_valid and rejects_invalid else "FAIL",
            "valid": valid,
            "synthetic_wrong": invalid,
        }
        results.append(result)
        print(case_id, result["classification"], name)

    payload = {
        "method": "predicate-level mutation validation",
        "results": results,
        "pass": sum(result["classification"] == "PASS" for result in results),
        "fail": sum(result["classification"] == "FAIL" for result in results),
    }
    (AUDIT / "harness_validation.json").write_text(
        json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    lines = [
        "# Harness Validation",
        "",
        "Method: predicate-level mutation validation. Each checker must accept a valid synthetic observation and reject one deliberately corrupted observation.",
        "",
        "| ID | Mutation | Valid accepted? | Wrong rejected? | Result |",
        "|---|---|---:|---:|---|",
    ]
    for result in results:
        lines.append(
            f"| {result['id']} | {result['name']} | {result['accepts_valid']} | {result['rejects_invalid']} | {result['classification']} |"
        )
    lines.extend(
        [
            "",
            f"Result: {payload['pass']} PASS, {payload['fail']} FAIL.",
            "",
            "This validates representative count, board, cleanup, semaphore, and deadline predicates. It does not prove that every test setup is correct; setup failures remain separately classified.",
        ]
    )
    (AUDIT / "harness_validation.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0 if payload["fail"] == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
