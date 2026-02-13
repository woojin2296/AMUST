# AMUST

## Raspberry Pi (Ubuntu 24.04) 빌드/실행 방법

```bash
# 1) 의존성 설치
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build git pkg-config \
  qt6-base-dev libqt6core6 libqt6gui6 libqt6widgets6 \
  libgpiod-dev

# 2) 프로젝트 클론/이동
cd /path/to/AMUST

# 3) 실행 권한 확인
chmod +x build.sh run.sh

# 4) 빌드
./build.sh

# 5) 실행
./run.sh
```

## GPIO 접근이 필요한 경우(선택)

```bash
sudo usermod -aG gpio $USER
```

재로그인 후 GPIO 제어 기능이 동작합니다.
