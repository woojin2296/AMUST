#!/usr/bin/env python3
import argparse
import time
import signal
import sys

try:
    import VL53L1X
except ImportError:
    print("ERR: VL53L1X 파이썬 모듈이 필요합니다.", file=sys.stderr)
    sys.exit(1)


stop = False


def _handle_sigint(signum, frame):
    global stop
    stop = True


def parse_args():
    ap = argparse.ArgumentParser(description="VL53L1X distance reader")
    ap.add_argument("--bus", default="1", help="I2C bus index 또는 '/dev/i2c-<n>'")
    ap.add_argument("--addr", default="0x29", help="I2C address (hex e.g. 0x29)")
    ap.add_argument("--interval", type=float, default=0.5, help="Polling interval seconds")
    ap.add_argument("--duration", type=float, default=0.0, help="Total duration seconds (0=infinite)")
    return ap.parse_args()


def parse_bus(bus_arg: str) -> int:
    if isinstance(bus_arg, str) and bus_arg.startswith("/dev/i2c-"):
        try:
            return int(bus_arg.split("-")[-1])
        except Exception:
            raise ValueError(f"invalid bus: {bus_arg}")
    return int(bus_arg)


def main():
    args = parse_args()
    try:
        bus_no = parse_bus(args.bus)
        addr = int(args.addr, 0)
    except Exception as e:
        print(f"ERR: invalid args: {e}", file=sys.stderr)
        return 2

    tof = VL53L1X.VL53L1X(i2c_bus=bus_no, i2c_address=addr)
    tof.open()

    # Ranging 설정
    tof.start_ranging(1)  # 0=Unchanged, 1=Short, 2=Medium, 3=Long
    tof.set_timing(33000, 50)  # budget(us), inter-measure(ms)

    signal.signal(signal.SIGINT, _handle_sigint)
    signal.signal(signal.SIGTERM, _handle_sigint)

    start = time.time()
    try:
        while not stop:
            try:
                result = tof.get_distance()  # blocking
            except Exception as e:
                print(f"ERR: read failed: {e}", file=sys.stderr, flush=True)
                time.sleep(args.interval)
                continue

            print(result, flush=True)  # 숫자만 출력

            # 0은 유효하지 않은 측정일 수 있으니 계속 시도
            if args.duration > 0 and (time.time() - start) >= args.duration:
                break

            time.sleep(args.interval)
    except KeyboardInterrupt:
        pass
    finally:
        try:
            tof.stop_ranging()
        except Exception:
            pass
        try:
            if hasattr(tof, "close"):
                tof.close()
        except Exception:
            pass

    return 0


if __name__ == "__main__":
    sys.exit(main())

