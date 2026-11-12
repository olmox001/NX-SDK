# NX-SDK public contract versioning

`contracts/version.json` declares independent revisions for the SDK wire-contract surface and the public API surface.

- `abi` covers every header under `include/contract/`.
- `api` covers every header under `include/api/`.
- each scope snapshot must enumerate every header in that scope exactly once;
- every recorded SHA-256 must match the file content;
- changing a public header requires increasing that scope revision;
- `make check-contracts` enforces the inventory, digests and revision rule.

The NexsOS1 integration repository additionally compares every NX-CORE wire ABI header with its NX-SDK counterpart. A local snapshot update alone cannot make incompatible kernel/SDK headers pass that integration gate.
