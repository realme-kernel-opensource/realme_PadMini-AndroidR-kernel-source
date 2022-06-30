my_top_dirs=$PWD
echo "$my_top_dirs"
cross=/work1/TL_TL18617169050/opensoure/cross_tools/toolchain/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-
cc_path=/work1/TL_TL18617169050/opensoure/cross_tools/prebuilts/clang/host/linux-x86/clang-r383902/bin/clang
ld_path=/work1/TL_TL18617169050/opensoure/cross_tools/prebuilts/clang/host/linux-x86/clang-r383902/bin/ld.lld
export PATH=$PATH:/work1/TL_TL18617169050/opensoure/cross_tools/prebuilts/clang/host/linux-x86/clang-r383902/bin/
# source $my_top_dirs/config/common/kernel.cfg
# bsp\kernel\kernel4.14\arch\arm64\configs
# source $my_top_dirs/arch/arm64/configs/na500/na500_base/kernel.cfg
# source $my_top_dirs/arch/arm64/configs/na500/na500_Natv/kernel.cfg

export BSP_BUILD_FAMILY=qogirl6
export BSP_BUILD_ANDROID_OS=y
export BSP_BUILD_DT_OVERLAY=y
mkdir out/
kernel_out_dir=$my_top_dirs/out
# export JOURNEY_BUILD_SCRIPT=yes
export ARCH=arm64
export SUBARCH=arm64
export HEADER_ARCH=arm64
export CLANG_TRIPLE=aarch64-linux-gnu-
#cd kernel-4.19/

make O=out ARCH=arm64 CC=$cc_path LD=$ld_path na500_defconfig
make -j16 O=out ARCH=arm64 CC=$cc_path LD=$ld_path CROSS_COMPILE=$cross
