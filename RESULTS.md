# Results through November 9, 2026

The tested stack is a Tesla P40 with NVIDIA 580.178.04, Linux 7.0.0-42-generic and reported VBIOS 86.02.23.00.01. Results below come from different configurations and workloads; they must not be combined into a stock-versus-overclock speedup claim.

## 1797 MHz target: passive workload observation

A read-only observer collected **277 samples over 1426.78 seconds (23 minutes 47 seconds)**, stopping when the existing run became paused. Application graphics clocks read 1797 MHz, memory 3615 MHz and the power limit 250 W.

| Measurement | Observed result |
|---|---|
| Actual SM clock, all samples | 1531–1771 MHz |
| Actual SM clock at >=90% utilization | 1670–1746 MHz; median 1695 MHz across 31 samples |
| Temperature | 42–56 C |
| Reported instantaneous power | 68.28–265.85 W |
| GPU utilization | 0–100% |
| VRAM used | 16153–16955 MiB |
| Status-query failures | 0 |
| NVIDIA Xid log check | No matching records in accessible current-boot logs during observation |

[Sanitized samples](results/1797-workload-samples.json) retain elapsed time and GPU measurements. The sample interval targeted five seconds; query overhead makes spacing slightly longer. These are discrete samples, not continuous dwell-time measurements. The minimum includes low-utilization periods. A sampled power peak above the configured limit does not establish sustained delivery at that level.

No inference requests or training/GPU setting changes were made by the observer. Native llama.cpp decode tok/s was not exposed by the embedding backend. Token grading was disabled for this run, so the alternate graded-token counter was unavailable; its zeros are not zero generation speed. No throughput result is claimed.

## 1911 MHz request at the 250 W cap

A later direct driver query confirmed application and maximum graphics clocks of **1911 MHz**, with actual SM clock **1759 MHz** and **96% GPU utilization**. The **software power cap was active**. Software thermal slowdown, hardware thermal slowdown and hardware power-brake slowdown were inactive; temperature was **54 C**.

The associated driver report contained a recent 2.36-second power sample window: **236.15 W average**, **144.79–258.59 W range**, with a **250 W** configured limit. Instantaneous readings and limiter flags need not reflect identical averaging windows. This supports power limiting at that observation, not a universal 1771 MHz hardware ceiling or a proof that power is the only possible limit.

The 2000 MHz table encoding produced a 1911 MHz driver graphics ceiling. Raising a reported/requested ceiling is distinct from reaching it under load. A matched same-workload comparison between the 1797 and 1911 settings has not been performed.

## Later 300 W variant: reported follow-up results

These results are transcribed from the updated local experiment passdown. Unlike the 277-sample observation above, the underlying raw follow-up series is not included in this repository. Treat them as handoff-reported measurements rather than independently re-collected measurements in this update.

- Initialization succeeded with a probe log containing `target=2000 power_mw=300000`. Current/default/maximum power limits reported 300 W, minimum 125 W.
- A short exact-FP32 workload completed **832 batches in approximately 5.0045 seconds**, checking **218,103,808 values** with `exact_fp32_match=true`. Across 97 samples, maxima were **1797 MHz SM**, **209.51 W**, and **41 C**.
- A separate approximately five-second, 20-sample workload series reported **18 samples above 250 W**, a **246.288–276.61 W** range, **1721–1746 MHz SM**, and **57 C** maximum.

The repeated samples above 250 W support increased measured draw beyond the former cap. They do not show continuous 300 W operation or safe long-term electrical/thermal margins. The FP32 check did not itself exercise draw above 250 W. Differences in batch count from the earlier test are not a controlled speed comparison.

The included probe remains clock-only. This update does not ship the power-limit variant as installation code.

## Memory candidate: not yet a measured success

The passdown describes a 3815 memory target, retaining the 2000 graphics table encoding and 300 W policy. An earlier memory variant booted but left supported, maximum and actual memory at 3615. A revised candidate additionally changes a flagged memory field and passes offline encoding/checksum checks; boot/load validation is pending in the passdown.

A direct runtime attempt showed application memory of 3815 while actual memory remained 3615, and graphics application clocks fell to 1303. That is a failed/unsupported request, not evidence of faster memory. No successful 3815 operating-clock, bandwidth, ECC-under-load or stability result is claimed.

## Earlier loaded proof

The saved 1544 MHz exact-FP32 test completed 736 batches in about 5.003 seconds, checked 192,937,984 values and passed exact comparison. A separate llama.cpp benchmark with eight GPU layers reached 1544 MHz and completed at about 24.31 prompt tokens/sec and 1.81 generated tokens/sec. It was a single repetition with partial GPU offload and competing VRAM use, not a benchmark of the card's maximum inference speed or a matched baseline comparison.

## Remaining validation

- Controlled same-workload stock/1797/1911 comparisons with repeated trials and measured achieved clocks.
- Independent numerical checks at each tested operating point, including sustained workloads and memory checks.
- Native generation counters/timing before reporting decode tok/s.
- Long-duration stability, power and thermal evaluation for the separate 300 W variant.
- Boot/load validation of the revised memory candidate and a reproduction from a byte-identical stock ROM.

These results establish experimental above-stock operation on the tested stack. They do not establish universal compatibility, general learning improvements, stable operation at the full requested ceiling or a performance gain.
