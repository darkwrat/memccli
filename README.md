# memccli
Simple commandline tool for interacting with secure memcached instances over binary protocol.
```
# ./memccli --help
./memccli: unrecognized option '--help'
Supported options:
	--host <val>, -h <val>
	--port <val>, -P <val>
	--snd-timeout-us <val>, -r <val>
	--rcv-timeout-us <val>, -w <val>
	--key <val>, -k <val>
	--value <val>, -v <val>
	--add, -a
	--del, -d
	--user <val>, -u <val>
	--pass <val>, -p <val>
	--flags <val>, -f <val>
	--expire <val>, -e <val>
```

Process exit code is `0` on success.

Process exit code less than `EX__BASE` is a libmemcached return code for the requested operation, substracted from `EX__BASE`.

Process exit code above `EX__BASE` means an internal error and guarantees that memcached state is untouched.

When `-k` is provided and `-v` is not provided - executes GET. Returned value is written to stdout.

When both `-k` and `-v` are provided - executes SET.

When `-k` and `-d` are provided - executes DELETE.

When `-a` is provided -- executes ADD. Iff `-k` and `-v` are also provided.
