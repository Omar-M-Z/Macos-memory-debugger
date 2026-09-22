# RuntimeAnalyst

RuntimeAnalyst is a macOS command-line program for finding values in another
process's memory. Scans can run on the CPU or with a Metal compute kernel.

## Requirements

- macOS with Clang and the Metal frameworks
- `metal-cpp` headers available to Clang
- Permission to inspect the target process

## Build

From the project directory, run:

```sh
make
```

This creates and signs the `analyst` executable.

## Start the program

Find the PID of the process you want to inspect, then run:

```sh
./analyst <pid>
```

For example:

```sh
./analyst 12345
```

If macOS denies access to the process, try running the command with `sudo`:

```sh
sudo ./analyst 12345
```

Run the program from the project directory. The Metal backend loads
`src/scan_memory.metal` using that directory.

## Scan for a value

The basic scan syntax is:

```text
scan <store-name> -v <value> -t <type> [-gpu] [-time] [-bytes]
```

Supported types are:

- `int`
- `float`
- `double`
- `rawbyte`: exactly eight binary digits, such as `00101010`
- `hexbyte`: one or two hexadecimal digits, such as `2a`

The store name identifies the results so they can be listed or refined later.

CPU scan example:

```text
scan first -v 1929340 -t int
```

GPU scan example:

```text
scan first -v 1929340 -t int -gpu
```

Options may appear in any order after the store name. `-gpu` selects the Metal
backend. CPU scanning is used when `-gpu` is omitted.

Use `-time` to print the scan time in milliseconds:

```text
scan timed -v 42 -t int -time
```

Use `-bytes` to print the total number of bytes searched:

```text
scan measured -v 42 -t int -gpu -time -bytes
```

Each store name must be unique during a session.

## View scan results

List the saved stores:

```text
stores
```

List addresses in a store:

```text
list first
```

The `list` command prints at most 100 addresses.

## Refine a scan

Refining reads the addresses in an existing store again and keeps values that
match the selected condition:

```text
refine <store-name> <same|changed|increased|decreased|new_value> [value]
```

Examples:

```text
refine first same
refine first changed
refine first increased
refine first decreased
refine first new_value 25
```

Only `new_value` takes an additional value. Refinement uses the type selected
when the store was created.

## Other commands

```text
help
quit
```

`help` prints the available commands. `quit` exits RuntimeAnalyst.

## Simple test target

The repository includes `target.cpp`, which keeps a known integer alive in a
small process. Build and run it in one terminal:

```sh
clang++ -std=c++17 target.cpp -o target
./target
```

It prints its PID and the value `1929340`. In another terminal, attach to that
PID and scan for the value:

```sh
sudo ./analyst <printed-pid>
```

Then enter:

```text
scan result -v 1929340 -t int -time -bytes
list result
```

To test the Metal backend, use a different store name:

```text
scan gpu-result -v 1929340 -t int -gpu -time -bytes
list gpu-result
```

