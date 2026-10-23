"""Run with Python; requires protoc from PATH or PROTOC environment variable."""
import os
import json
import re
import tempfile
from google.protobuf import descriptor_pb2
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


class UnifiedProtocolTest(unittest.TestCase):
    def test_all_projects_share_schema_and_retired_fields(self):
        schemas = []
        for repo in ("Mary", "Wind", "HQMarket"):
            folder = ROOT / repo / repo / "request"
            with tempfile.TemporaryDirectory() as temporary:
                output = Path(temporary) / "schema.pb"
                subprocess.run([PROTOC, "-I", str(folder), "--include_imports", "--descriptor_set_out=" + str(output), str(folder / "request.proto")], check=True, capture_output=True)
                descriptor = descriptor_pb2.FileDescriptorSet.FromString(output.read_bytes())
            schemas.append(descriptor.SerializeToString())
            wire_types = next(enum for file in descriptor.file for enum in file.enum_type if enum.name == "RequestType")
            header = (folder / "request.h").read_text(encoding="utf-8")
            cpp_enum = re.search(r"enum class types\s*\{(.*?)\};", header, re.S).group(1)
            cpp_values = {name: int(number) for name, number in re.findall(r"(\w+)\s*=\s*(\d+)", cpp_enum)}
            self.assertEqual(cpp_values, {value.name: value.number for value in wire_types.value})
            self.assertEqual(41, len(cpp_values))
            envelope = next(message for file in descriptor.file for message in file.message_type if message.name == "RequestData")
            fields = {field.name for field in envelope.field}
            self.assertFalse(fields.intersection({"cmd", "ret", "auth_request", "subscribe_request", "unsubscribe_request", "query_request", "account_snapshot"}))
            self.assertIn("account_info_list", fields)
            self.assertIn("strategy_runtime_list", fields)
            reserved = {number for span in envelope.reserved_range for number in range(span.start, span.end)}
            self.assertTrue({2, 4, 10, 11, 12, 20, 32}.issubset(reserved))
        self.assertEqual(schemas[0], schemas[1])
        self.assertEqual(schemas[0], schemas[2])

    def test_auth_uses_extra_and_top_level_status(self):
        text = transcode("Wind", "Mary", 'type: AUTH_LOGIN id: 10 extra { key: "token" value: "test-token" } extra { key: "user_id" value: "7" }')
        self.assertIn('key: "token"', text)
        self.assertNotIn("ret {", text)
        self.assertNotIn("auth_response", text)
        error = transcode("HQMarket", "Wind", 'type: AUTH_LOGIN id: 11 error_code: 1001 error_message: "invalid credentials"')
        self.assertIn("error_code: 1001", error)

    def test_account_info_batch_and_row_failure(self):
        for ids in ([1001], [1001, 1002], []):
            encoded_json = json.dumps(json.dumps(ids))
            text = transcode("Mary", "Wind", 'type: TRADE_ACCOUNT_INFO_QUERY id: 42 extra { key: "account_ids" value: ' + encoded_json + ' }')
            self.assertIn('key: "account_ids"', text)
            self.assertNotIn("payload", text)
        text = transcode("Wind", "Mary", 'type: TRADE_ACCOUNT_INFO_QUERY id: 42 account_info_list { accounts { account_id: 1001 balance: 500 available_snapshot: true } accounts { account_id: 1002 error_code: 1202 error_message: "missing" } }')
        self.assertIn("account_id: 1001", text)
        self.assertIn("account_id: 1002", text)
        self.assertIn("error_code: 1202", text)
        self.assertIn("account_info_list {", transcode("Wind", "Mary", 'type: TRADE_ACCOUNT_INFO_QUERY id: 42 account_info_list {}'))

    def test_subscription_parameters_are_json_and_ack_preserves_type(self):
        rows = [{"security": "600000.SSE", "channel": "quote"}, {"security": "000001.SZSE", "channel": "depth"}]
        for type_name in ("MARKET_SUBSCRIBE", "MARKET_UNSUBSCRIBE"):
            text = transcode("Mary", "HQMarket", 'type: ' + type_name + ' id: 44 extra { key: "subscriptions" value: ' + json.dumps(json.dumps(rows)) + ' }')
            self.assertIn(type_name, text)
            reply = transcode("HQMarket", "Mary", 'type: ' + type_name + ' id: 44 subscription_ack { results { security { symbol: "600000" exchange: SSE } channel: CHANNEL_QUOTE accepted: true } }')
            self.assertIn(type_name, reply)
            self.assertIn("subscription_ack {", reply)

    def test_strategy_runtime_uses_list_payload(self):
        text = transcode("Wind", "Mary", 'type: STRATEGY_RUNTIME_QUERY id: 5 strategy_runtime_list { runtimes { strategy_id: 100 state: 2 market_available: true queue_length: 5 active_orders: 1 } }')
        self.assertIn("strategy_runtime_list {", text)
        self.assertIn("strategy_id: 100", text)
        self.assertNotIn("runtime_count", text)


class TradingCounterWireTest(unittest.TestCase):
    def test_order_request_is_environment_neutral_metadata(self):
        request = 'type: TRADE_ORDER_SUBMIT id: 90 extra { key: "account_id" value: "1" } extra { key: "client_order_id" value: "900" } extra { key: "security" value: "600000.SSE" } extra { key: "order_type" value: "limit" }'
        for target in ("Wind", "HQMarket"):
            text = transcode("Mary", target, request)
            self.assertIn("TRADE_ORDER_SUBMIT", text)
            self.assertNotIn("simulated", text)
            self.assertNotIn("order_request", text)

    def test_snapshot_and_event_watermark_round_trip(self):
        text = transcode("Wind", "Mary", 'type: TRADE_SYNC_QUERY id: 91 trade_sync { watermark: 12 ready: true accounts { account_id: 1 balance: 100 } orders { account_id: 1 order_id: 2 client_order_id: 900 price: 1000 price_scale: 2 quantity: 100 filled_quantity: 50 } trades { account_id: 1 trade_id: 3 order_id: 2 quantity: 50 } positions { account_id: 1 security: "600000.SSE" total_quantity: 50 today_quantity: 50 } events { sequence: 12 request_id: 90 account_id: 1 kind: 2 trade { trade_id: 3 } } }')
        self.assertIn("watermark: 12", text)
        self.assertIn("today_quantity: 50", text)
        self.assertIn("client_order_id: 900", text)
        push = transcode("Wind", "HQMarket", 'type: TRADE_ORDER_PUSH extra { key: "sequence" value: "12" } trade_orders { orders { order_id: 2 } }')
        self.assertIn("TRADE_ORDER_PUSH", push)
        self.assertNotIn("id:", push.split("trade_orders")[0])


if __name__ == "__main__":
    unittest.main()
