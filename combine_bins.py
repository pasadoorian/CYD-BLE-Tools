import os
Import("env")

def combine_bins(source, target, env):
    firmware_dir = env.subst("$BUILD_DIR")
    firmware_bin = f"{firmware_dir}/firmware.bin"
    bootloader_bin = f"{firmware_dir}/bootloader.bin"
    partitions_bin = f"{firmware_dir}/partitions.bin"

    # Extract the last part of the build directory name
    build_dir_name = os.path.basename(firmware_dir)
    combined_bin = f"{firmware_dir}/{build_dir_name}.bin"

    # Merge the binaries into one
    env.Execute(
        f"esptool.py --chip esp32 merge_bin --output {combined_bin} "
        f"--flash_mode dio --flash_freq 40m --flash_size 4MB "
        f"0x1000 {bootloader_bin} "
        f"0x8000 {partitions_bin} "
        f"0x10000 {firmware_bin}"
    )
    print(f"Combined firmware created: {combined_bin}")

# Add the post-action to run after buildprog
env.AddPostAction("buildprog", combine_bins)
