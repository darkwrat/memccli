# memccli
Simple commandline tool for interacting with secure memcached instances over binary protocol.
```
Supported options:
	--host <val>, -h <val>
	--port <val>, -P <val>
	--snd-timeout-us <val>, -r <val>
	--rcv-timeout-us <val>, -w <val>
	--key <val>, -k <val>
	--value <val>, -v <val>
	--del, -d
	--user <val>, -u <val>
	--pass <val>, -p <val>
	--flags <val>, -f <val>
	--expire <val>, -e <val>
```

Process exit code is `0` on success.

When `-k` is provided and `-v` is not provided - executes GET. Returned value is written to stdout.

When both `-k` and `-v` are provided - executes SET.

When `-k` and `-d` are provided - executes DELETE.
