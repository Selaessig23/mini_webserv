#!/usr/bin/env python3

import argparse
import os
import select
import socket
import subprocess
import sys
import tempfile
import time
import shlex
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional


DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 19090
DEFAULT_CLIENTS = 20
DEFAULT_JOIN_TEMPLATE = "A new member has entered the room, Id: {id}\n"
DEFAULT_WELCOME_TEMPLATE = "Welcome new member, you got the id: {id}\n"
DEFAULT_LEAVE_TEMPLATE = "A member has left the room, Id: {id}\n"
VALGRIND_ARGS = shlex.split(
    os.environ.get(
        "VALGRIND_CMD",
        "valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes",
    )
)


class TestFailure(RuntimeError):
    pass


@dataclass
class CheckResult:
    passed: bool
    label: str
    detail: str = ""


class Reporter:
    def __init__(self) -> None:
        self.results: List[CheckResult] = []

    def pass_check(self, label: str, detail: str = "") -> None:
        self.results.append(CheckResult(True, label, detail))
        print(f"[PASS] {label}")

    def fail_check(self, label: str, detail: str) -> None:
        self.results.append(CheckResult(False, label, detail))
        print(f"[FAIL] {label}")
        if detail:
            print(f"       {detail}")

    def summary(self) -> int:
        passed = sum(1 for result in self.results if result.passed)
        failed = len(self.results) - passed

        print("\nSummary")
        print(f"  Passed: {passed}")
        print(f"  Failed: {failed}")
        return 0 if failed == 0 else 1


class ServerHandle:
    def __init__(self, binary: Path, port: int, use_valgrind: bool) -> None:
        self.binary = binary
        self.port = port
        self.use_valgrind = use_valgrind
        self.temp_dir = tempfile.TemporaryDirectory(prefix="mini_webserv_test_")
        self.log_path = Path(self.temp_dir.name) / "server.log"
        self.log_file = None
        self.process = None

    def start(self) -> None:
        command = [str(self.binary), str(self.port)]
        if self.use_valgrind:
            command = VALGRIND_ARGS + command
        self.log_file = self.log_path.open("w+", encoding="utf-8")
        self.process = subprocess.Popen(
            command,
            stdout=self.log_file,
            stderr=subprocess.STDOUT,
            cwd=str(self.binary.parent),
        )
        time.sleep(0.1)
        self.assert_alive("during startup")

    def assert_alive(self, context: str) -> None:
        if self.process is None:
            raise TestFailure("Server process was not started")
        returncode = self.process.poll()
        if returncode is not None:
            raise TestFailure(
                f"Server exited {context} with code {returncode}.\n{self.tail_log()}"
            )

    def tail_log(self, lines: int = 20) -> str:
        if not self.log_path.exists():
            return "No server log was captured."
        content = self.log_path.read_text(encoding="utf-8", errors="replace").splitlines()
        if not content:
            return "Server log is empty."
        tail = "\n".join(content[-lines:])
        return f"Server log tail:\n{tail}"

    def stop(self) -> None:
        if self.process is not None and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=1.0)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=1.0)
        if self.log_file is not None:
            self.log_file.close()

    def cleanup(self) -> None:
        self.temp_dir.cleanup()

    def full_log(self) -> str:
        if not self.log_path.exists():
            return "No server log was captured."
        return self.log_path.read_text(encoding="utf-8", errors="replace")


class Client:
    def __init__(self, sock: socket.socket, client_id: int) -> None:
        self.sock = sock
        self.client_id = client_id
        self.buffer = ""

    def send_text(self, text: str) -> None:
        self.sock.sendall(text.encode("utf-8"))

    def _pop_line(self) -> Optional[str]:
        if "\n" not in self.buffer:
            return None
        line, _, rest = self.buffer.partition("\n")
        self.buffer = rest
        return line + "\n"

    def read_next_line(self, timeout: float) -> str:
        deadline = time.monotonic() + timeout
        while True:
            line = self._pop_line()
            if line is not None:
                return line
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TestFailure(
                    f"Timed out while waiting for data from client {self.client_id}"
                )
            readable, _, _ = select.select([self.sock], [], [], remaining)
            if not readable:
                continue
            data = self.sock.recv(4096)
            if not data:
                raise TestFailure(
                    f"Client {self.client_id} connection closed while waiting for data"
                )
            self.buffer += data.decode("utf-8", errors="replace")

    def expect_line(self, expected: str, timeout: float) -> None:
        actual = self.read_next_line(timeout)
        if actual != expected:
            raise TestFailure(
                f"Client {self.client_id} expected {expected!r} but received {actual!r}"
            )

    def expect_silence(self, timeout: float) -> None:
        if self.buffer:
            raise TestFailure(
                f"Client {self.client_id} had unexpected buffered data: {self.buffer!r}"
            )
        readable, _, _ = select.select([self.sock], [], [], timeout)
        if not readable:
            return
        data = self.sock.recv(4096)
        if not data:
            raise TestFailure(
                f"Client {self.client_id} connection closed unexpectedly while checking silence"
            )
        self.buffer += data.decode("utf-8", errors="replace")
        raise TestFailure(
            f"Client {self.client_id} received unexpected data: {self.buffer!r}"
        )

    def close(self) -> None:
        try:
            self.sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        self.sock.close()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Integration test for the mini_webserv server."
    )
    parser.add_argument("--binary", default="./miniserv", help="Path to the server binary")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Host to connect to")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Port to connect to")
    parser.add_argument(
        "--clients",
        type=int,
        default=DEFAULT_CLIENTS,
        help="How many clients to connect before broadcast tests",
    )
    parser.add_argument(
        "--connect-timeout",
        type=float,
        default=2.0,
        help="Timeout in seconds for new connections",
    )
    parser.add_argument(
        "--read-timeout",
        type=float,
        default=1.0,
        help="Timeout in seconds for expected messages",
    )
    parser.add_argument(
        "--quiet-timeout",
        type=float,
        default=0.35,
        help="How long to wait when asserting no data is received",
    )
    parser.add_argument(
        "--join-template",
        default=DEFAULT_JOIN_TEMPLATE,
        help="Join message template. Use {id} for the client id.",
    )
    parser.add_argument(
        "--welcome-template",
        default=DEFAULT_WELCOME_TEMPLATE,
        help="Welcome message template. Use {id} for the client id.",
    )
    parser.add_argument(
        "--leave-template",
        default=DEFAULT_LEAVE_TEMPLATE,
        help="Leave message template. Use {id} for the client id.",
    )
    parser.add_argument(
        "--valgrind",
        action="store_true",
        help="Run the server process under valgrind.",
    )
    return parser.parse_args()


def connect_client(
    host: str,
    port: int,
    expected_id: int,
    timeout: float,
    server: ServerHandle,
) -> Client:
    deadline = time.monotonic() + timeout
    last_error = None
    while time.monotonic() < deadline:
        server.assert_alive("while accepting a client")
        try:
            sock = socket.create_connection((host, port), timeout=0.5)
            return Client(sock, expected_id)
        except OSError as exc:
            last_error = exc
            time.sleep(0.05)
    raise TestFailure(
        f"Could not connect client {expected_id} to {host}:{port}: {last_error}"
    )


def check_connection_sequence(
    args: argparse.Namespace,
    server: ServerHandle,
    reporter: Reporter,
) -> List[Client]:
    clients: List[Client] = []
    for expected_id in range(1, args.clients + 1):
        label = f"client {expected_id} connects"
        try:
            client = connect_client(
                args.host,
                args.port,
                expected_id,
                args.connect_timeout,
                server,
            )
            welcome_message = args.welcome_template.format(id=expected_id)
            client.expect_line(welcome_message, args.read_timeout)
            reporter.pass_check(
                label,
                f"received welcome message {welcome_message!r}",
            )
            for other in clients:
                join_message = args.join_template.format(id=expected_id)
                other.expect_line(join_message, args.read_timeout)
            if clients:
                reporter.pass_check(
                    f"existing clients saw client {expected_id} join",
                    f"broadcasted {args.join_template.format(id=expected_id)!r}",
                )
            clients.append(client)
        except TestFailure as exc:
            reporter.fail_check(label, str(exc))
            raise
    return clients


def check_broadcast(
    clients: List[Client],
    args: argparse.Namespace,
    reporter: Reporter,
) -> None:
    if len(clients) < 2:
        reporter.fail_check(
            "broadcast between clients",
            "Need at least 2 connected clients for the broadcast check.",
        )
        raise TestFailure("Not enough connected clients to test broadcasting")

    sender_index = len(clients) // 2
    sender = clients[sender_index]
    payload = f"client-{sender.client_id} says hello to everybody\n"

    try:
        sender.send_text(payload)
        for client in clients:
            if client is sender:
                continue
            client.expect_line(payload, args.read_timeout)
        sender.expect_silence(args.quiet_timeout)
        reporter.pass_check(
            "client broadcast reaches all other clients",
            f"payload {payload!r} reached every client except client {sender.client_id}",
        )
    except TestFailure as exc:
        reporter.fail_check("client broadcast reaches all other clients", str(exc))
        raise


def check_disconnect_and_reconnect(
    clients: List[Client],
    args: argparse.Namespace,
    server: ServerHandle,
    reporter: Reporter,
) -> None:
    if len(clients) < 3:
        reporter.fail_check(
            "disconnect handling",
            "Need at least 3 connected clients for the disconnect test.",
        )
        raise TestFailure("Not enough connected clients to test disconnect handling")

    leaver_index = len(clients) // 3
    leaver = clients.pop(leaver_index)
    leaver_id = leaver.client_id
    leave_message = args.leave_template.format(id=leaver_id)

    try:
        leaver.close()
        server.assert_alive("after a client disconnect")
        for client in clients:
            client.expect_line(leave_message, args.read_timeout)
        reporter.pass_check(
            "disconnect broadcasts a leave message",
            f"all remaining clients received {leave_message!r}",
        )
    except TestFailure as exc:
        reporter.fail_check("disconnect broadcasts a leave message", str(exc))
        raise

    next_id = args.clients + 1
    reconnect_label = f"replacement client gets id {next_id}"
    try:
        replacement = connect_client(
            args.host,
            args.port,
            next_id,
            args.connect_timeout,
            server,
        )
        replacement.expect_line(args.welcome_template.format(id=next_id), args.read_timeout)
        for client in clients:
            client.expect_line(args.join_template.format(id=next_id), args.read_timeout)
        clients.append(replacement)
        reporter.pass_check(
            reconnect_label,
            f"welcome and join messages use id {next_id}",
        )
    except TestFailure as exc:
        reporter.fail_check(reconnect_label, str(exc))
        raise


def close_clients(clients: List[Client]) -> None:
    for client in clients:
        try:
            client.close()
        except OSError:
            pass


def main() -> int:
    args = parse_args()
    binary = Path(args.binary).resolve()
    reporter = Reporter()

    if not binary.exists():
        print(f"Server binary not found: {binary}", file=sys.stderr)
        return 1

    server = ServerHandle(binary, args.port, args.valgrind)
    clients: List[Client] = []

    try:
        try:
            server.start()
        except TestFailure as exc:
            reporter.fail_check("server starts", str(exc))
            return reporter.summary()
        reporter.pass_check(
            "server starts",
            f"launched {binary.name} on port {args.port}",
        )
        clients = check_connection_sequence(args, server, reporter)
        check_broadcast(clients, args, reporter)
        check_disconnect_and_reconnect(clients, args, server, reporter)
    except TestFailure:
        pass
    finally:
        close_clients(clients)
        server.stop()
        print()
        print(server.full_log())
        server.cleanup()

    return reporter.summary()


if __name__ == "__main__":
    sys.exit(main())
