# Method and findings

## Problem

Runtime NVML requests above the former 1531 MHz ceiling could be accepted without producing a higher loaded clock. Direct edits to active VBIOS regions also caused GPU initialization failures. The useful distinction was between accepting a clock-policy request, reporting a higher limit, and actually operating at a higher frequency.

## Working approach

The physical VBIOS remains unchanged for clock policy and initialization. During driver initialization, the NVIDIA driver constructs a writable copy of the VBIOS in host RAM. The probe modifies that copy before the driver constructs its clock policy.

This avoids the physical firmware authentication path; it does not defeat or bypass Falcon signature verification. Original initialization firmware still runs on the card.

## Probe mechanics

On x86-64, `p40clock.c` registers a kprobe at `_nv000283rm`. At function entry it:

1. Treats the first argument in `regs->di` as the driver's GPU object.
2. Reads the BIOS object pointer at GPU object offset `0x20c8`.
3. Reads the writable image pointer at BIOS object offset `0x2e8`.
4. Reads the image size at BIOS object offset `0x2f0` and requires `0x45000` bytes.
5. Checks every expected original value, while also accepting the corresponding patched value for idempotence.
6. Aborts without writing if a pointer read fails, the size differs, or any value is unexpected.
7. Applies the edits and fixes the shadow checksum.

These offsets, the symbol name, and the object layouts are driver/version-specific. The original-value guard is a compatibility check, not a proof that a different configuration is supported.

## Included 2000 MHz candidate

Offsets are relative to the start of the host-RAM VBIOS shadow:

| Offset | Original | Candidate | Size | Purpose |
|---|---:|---:|---:|---|
| `0x2d711` | `0x02fd` | `0x03e8` | 2 bytes | Related performance-domain companion |
| `0x2d77d` | `0x02fd8bf6` | `0x03e88fa0` | 4 bytes | Maximum vP-state record 1 |
| `0x2d7cf` | `0x02fd8bf6` | `0x03e88fa0` | 4 bytes | Maximum vP-state record 3 |
| `0x37fff` | `0x3b` | `0x1b` | 1 byte | Preserve the additive shadow checksum |

The candidate vP-state raw value is `0x03e88fa0`, stored as `a0 8f e8 03` in little-endian order. Its companion value is `0x03e8`. A community VBIOS parser independently decoded the modified table as a 2000 MHz maximum and reported the companion as 2000 MHz.

The vP-state encoding is not a plain integer MHz field. Changes should be encoded and decoded with an appropriate parser rather than inferred from the final integer alone.

Parser agreement and checksum consistency do not establish hardware stability. The included 2000 MHz candidate still requires a separate initialization test, application-clock readback, loaded clock sampling, and correctness-checked workload.

## Previously verified configurations

### 1544 MHz loaded proof

A dedicated FP32 workload ran at 1544 MHz and checked every output value exactly. This established that the host-RAM approach could produce a real loaded clock above 1531 MHz rather than only a misleading policy readback.

A llama.cpp benchmark also reached 1544 MHz under a real model workload. These tests were functional demonstrations, not matched stock-versus-overclock performance comparisons.

### 1797 MHz application target

The earlier persistent configuration used:

| Offset | Patched value |
|---|---:|
| `0x2d711` | `0x0388` |
| `0x2d77d` | `0x03888e22` |
| `0x2d7cf` | `0x03888e22` |
| `0x37fff` | `0x39` |

Under varying real workloads, observed SM clocks approached the 1797 MHz target. Power draw approached the stock 250 W limit, and clock frequency varied with utilization and power management. This demonstrated a substantially expanded operating range, not a fixed all-workload frequency.

## Negative results that shaped the method

- Above-stock runtime API acceptance alone did not change the actual clock ceiling.
- Multiple direct physical VBIOS edits failed during initialization.
- Matching local data tags on one modified image did not prove that the complete initialization integrity path was satisfied.
- A benign edit in an erased region outside identified images initialized normally, arguing against whole-chip authentication and in favor of region-specific checks.
- No general Falcon signing bypass was discovered or needed for the host-RAM method.

No modified ROM or proprietary initialization firmware is included in this repository.

## Delivery constraint

The kprobe must be armed before `_nv000283rm` handles the initial driver-owned shadow. A normal late `insmod` does not rewind clock-policy construction.

The research boot used a separately instrumented proprietary NVIDIA module to request the probe as `p40init.ko` during initialization. That driver derivative is not part of this source snapshot. An early-boot delivery mechanism must therefore be reproduced separately, with a stock fallback and verification of the final initramfs contents.

## Validation standard for future targets

For each candidate, distinguish these milestones:

1. Table encoding and checksum validate offline.
2. The probe reports that it applied without a refused-value message.
3. GPU initialization completes.
4. The application-clock request succeeds and reads back as expected.
5. The reported maximum clock changes.
6. A real workload reaches a higher measured SM clock.
7. Computation remains correct and performance, power, temperature, and stability are recorded.

Only steps 6 and 7 establish a useful loaded overclock. Higher targets may remain limited by voltage, available clock bins, temperature, or power delivery.
