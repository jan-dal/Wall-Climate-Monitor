.PHONY: build flash qemu

# Build production firmware
build:
	idf.py --preset default build

# Flash physical ESP32-S3
flash:
	idf.py --preset default flash monitor

# Run firmware interactively in QEMU
qemu:
	idf.py --preset qemu qemu \
		--qemu-extra-args="-m 8M -nic user,model=open_eth,hostfwd=tcp:127.0.0.1:8080-:80" \
		monitor