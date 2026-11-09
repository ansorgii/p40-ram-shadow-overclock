# Tesla P40 host-RAM shadow overclock

An experimental Linux/NVIDIA driver research method for raising the clock ceiling of a Pascal Tesla P40 without flashing modified clock tables or initialization firmware onto the card.

The kernel probe changes four fields in the driver's writable VBIOS shadow before clock-policy construction. A small startup script then requests application clocks through `nvidia-smi`.

## Research status

- Actual loaded operation at **1544 MHz** was verified with an exact FP32 workload and a llama.cpp benchmark.
- A **1797 MHz** application target was subsequently used under real workloads, with live clocks approaching that target and power management remaining active.
- The included table encoding targets **2000 MHz** and its values were checked with a community VBIOS parser. This specific 2000 MHz configuration has **not yet been boot- or load-tested**.
- No physical VBIOS clock edit, Falcon authentication bypass, or power-limit modification is part of this method.

This is a research snapshot, not a turnkey or generally portable overclocking utility. See [METHOD.md](METHOD.md) for the technical details and evidence boundaries.

## Safety and compatibility

This code patches kernel memory owned by a proprietary driver during GPU initialization. It can prevent GPU initialization, crash the driver, or cause hardware instability if used with an incompatible driver, kernel, VBIOS, or GPU.

The current constants are specific to one Tesla P40 VBIOS layout and one x86-64 NVIDIA driver/kernel configuration. The original-value checks reduce the chance of patching an unexpected image, but they do not make the probe portable or guarantee safety. Keep a stock boot entry and do not use this on a production system.

Higher requested clocks do not guarantee higher sustained clocks. Voltage, clock-bin selection, temperature, and the stock power limit can reduce the actual frequency. No long-term stability or safe operating margin has been established.

## Repository contents

- `p40clock.c`: kprobe that verifies and patches the host-RAM VBIOS shadow.
- `Makefile`: external kernel-module build declaration.
- `p40-clock`: selects a Tesla P40 by GPU name, requests 3615/2000 MHz, and checks the application-clock readback.
- `p40-clock.service`: optional systemd oneshot service.
- `initramfs-hook.example`: template for embedding the probe as `p40init.ko` in an initramfs.

No proprietary driver, VBIOS image, signing material, account data, or machine-specific boot configuration is included.

## Build the probe

Install the headers for the running kernel, then run:

```bash
make -C "/lib/modules/$(uname -r)/build" M="$PWD" modules
```

The result is `p40clock.ko`. Building successfully only establishes compilation compatibility; it does not validate the probe against another GPU or driver.

## Application-clock service

Only install this service on a boot configuration where the raised clock policy is already known to work:

```bash
sudo install -m 0755 p40-clock /usr/local/sbin/p40-clock
sudo install -m 0644 p40-clock.service /etc/systemd/system/p40-clock.service
sudo systemctl daemon-reload
sudo systemctl enable p40-clock.service
```

The script locates the P40 dynamically, so no PCI address is hard-coded. After a successful boot, inspect it with:

```bash
systemctl status p40-clock.service --no-pager
nvidia-smi --query-gpu=name,clocks.applications.graphics,clocks.max.graphics,clocks.sm,power.draw,temperature.gpu --format=csv
journalctl -b -t kernel | grep P40CLOCK
```

Do not treat an accepted `nvidia-smi` request or a raised reported ceiling as proof of a loaded overclock. Measure the actual SM clock while running a correctness-checked workload.

## Early-boot integration

The probe must be registered before the driver reaches `_nv000283rm` during initial clock-policy construction. Loading it after normal driver initialization is too late.

The research setup used an instrumented NVIDIA module that requested `p40init` at the required initialization point. That proprietary derivative is not distributed here. A stock NVIDIA driver will not automatically load the probe merely because `initramfs-hook.example` copies it into an image.

For reproduction, a developer must provide an equivalent early-loading mechanism, adapt `initramfs-hook.example`, build a separate initramfs, and retain a known-good stock entry. The hook's `P40_CLOCK_MODULE` variable must point to the compiled module. Verify the module embedded in the final image byte-for-byte before rebooting.

## Return to stock application clocks

```bash
sudo nvidia-smi -i 0 -ac 3615,1303
```

Replace GPU index `0` if the P40 has another index. To stop applying the application target on future boots:

```bash
sudo systemctl disable p40-clock.service
```

Disabling the service does not reset clocks in the current boot. Booting the unmodified initramfs restores stock initialization and clock-policy construction.

## Privacy

This snapshot intentionally omits local absolute paths, usernames, account identifiers, hostnames, GPU UUIDs, disk UUIDs, PCI addresses, ROM backups, remote repository names, and details of unrelated workloads. The documented results are limited to the P40 method itself.
