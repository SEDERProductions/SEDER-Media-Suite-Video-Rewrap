# Contributing

Thanks for helping improve Video Rewrap.

## Development

Required tools:

- Rust stable
- Node.js 20 or newer
- Qt 6.5 or newer
- CMake 3.24 or newer
- FFmpeg, FFprobe, and FFplay for manual media testing

Run checks before opening a pull request:

```sh
cargo fmt --check
cargo check
cargo test
cmake -S qt -B qt/build/release -DCMAKE_BUILD_TYPE=Release
cmake --build qt/build/release
ctest --test-dir qt/build/release
```

Keep changes focused. Avoid committing build output, generated packages, local screenshots, or machine-specific files.
