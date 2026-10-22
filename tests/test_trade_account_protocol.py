"""Run with Python; requires protoc from PATH or PROTOC environment variable."""
import os
from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
PROTOC = os.environ.get("PROTOC", "protoc")


def transcode(source, target, text):
    def command(repo, action):
        folder = ROOT / repo / repo / "request"
        return [PROTOC, "-I", str(folder), "--" + action + "=request.RequestData", str(folder / "request.proto")]
    encoded = subprocess.run(command(source, "encode"), input=text.encode(), check=True, capture_output=True).stdout
    return subprocess.run(command(target, "decode"), input=encoded, check=True, capture_output=True).stdout.decode()


class TradeAccountProtocolTest(unittest.TestCase):
    def test_query_has_no_scope_cmd_or_ret(self):
        text = transcode("Mary", "Wind", "type: TRADE_ACCOUNT_QUERY id: 42")
        self.assertIn("TRADE_ACCOUNT_QUERY", text)
        for field in ("scope", "extra", "cmd", "ret"):
            self.assertNotIn(field + ":", text)

    def test_response_records_and_explicit_empty_list(self):
        for payload in ('account_list {}', 'account_list { accounts { account_id: 1 name: "Simulation" number: "SIM-1" } }'):
            text = transcode("Wind", "Mary", "type: TRADE_ACCOUNT_QUERY id: 42 error_code: 0 " + payload)
            self.assertIn("account_list {", text)
            self.assertNotIn("ret {", text)

    def test_error_uses_top_level_fields(self):
        text = transcode("Wind", "Mary", 'type: TRADE_ACCOUNT_QUERY id: 42 error_code: 1201 error_message: "database unavailable"')
        self.assertIn("error_code: 1201", text)
        self.assertIn('error_message: "database unavailable"', text)
        self.assertNotIn("ret {", text)


if __name__ == "__main__":
    unittest.main()
