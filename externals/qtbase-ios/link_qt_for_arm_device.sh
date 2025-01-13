#!/bin/bash
rm upstream.patched.ios.clang || true
ln -s upstream.patched.ios.device.clang.static upstream.patched.ios.clang 
rm upstream.patched.ios.clang-fat.static || true
ln -s upstream.patched.ios.device.clang.static upstream.patched.ios.clang-fat.static
