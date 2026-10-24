CREATE TABLE IF NOT EXISTS account
(
	account TEXT NOT NULL PRIMARY KEY,
	password BLOB NOT NULL,
	create_time INTEGER NOT NULL,
	update_time INTEGER NOT NULL,
	last_success_time INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_account_last_success_time
ON account(last_success_time DESC);
