#!/bin/bash

INPUT_FILE="$1"
OUTPUT_EXE="${INPUT_FILE%.c}.exe"

docker build --platform=linux/amd64 -t mingw-wine-env .
docker run --rm -v "$(pwd)":/app -w /app mingw-wine-env