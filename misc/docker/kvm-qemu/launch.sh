#/bin/sh
# Launch a native EbbRT 'backend' with kvm-qemu
#
# Usage:
# ./launch.sh <elf32> 
#
if [ -n "$DEBUG" ]; then set -x; fi
if [ -n "$DRY_RUN" ]; then 
  echo "***DRY RUN***: Command will not be executed"
  LAUNCHER=echo
fi
if [ -n "$NO_NETWORK" ]; then 
  echo "***No Networking***"
fi
if [ -n "$GDB" ]; then 
  echo "***GDB Enabled @localhost:1234 ***"
fi

# configure virtual machine
VCPU=${VM_CPU:-2}
VMEM=${VM_MEM:-2G}
VNUMA=${VM_NUMA:-0} # numa
WAIT=${VM_WAIT:-false}
TAP=${TAP_IFACE:-tap0}
MAC=$VM_MAC
VQS=$VCPU
NETVEC=$(($((${VCPU}*2))+ 2))
if [ $VCPU -eq 1 ]
then
  VQS=2
fi
# configure networking
if [ -n "$NO_NETWORK" ]; then 
: ${KVM_NET_OPTS:=""}
else
: ${KVM_NET_OPTS:="-netdev tap,script=no,downscript=no,\
ifname=\$TAP,id=net0,vhost=on,queues=\$VQS \
-device virtio-net-pci,netdev=net0,mac=\$MAC,mq=on,\
vectors=\$NETVEC"}

# Generate random MAC address if one isn't set already
if [ -z "$MAC" ]; then
  echo "***No Mac Address Specified***"
  hexchars="0123456789ABCDEF"
  end=$( for i in {1..8} ; do echo -n ${hexchars:$(( $RANDOM % 16 )):1} ; done | sed -e 's/\(..\)/:\1/g' )
  MAC=`echo 06:FE$end`
fi
fi # end NETWORK
if [ -z "$KVM_ARGS" ]; then
  # No boot configuration passed in   
  BOOTIMG=${BOOT_IMG:-$1}
  # verify boot image exists 
  if [[ ! -f "$BOOTIMG" ]]; then echo "Boot image not found: $1"; exit 1; fi
  KVM_ARGS="-kernel $BOOTIMG"
fi
if [ -n "$GDB" ]; then 
  KVM_ARGS="-s $KVM_ARGS"
fi
if [ -n "$NO_NETWORK" ]; then 
KVM_ARGS="$KVM_ARGS -append 'nodhcp;'"
fi

# numa 
NUMA_CMD=""
if [ $VNUMA -ne 0 ]
then
  for i in $(seq 1 $VNUMA);
  do
    NUMA_CMD="$NUMA_CMD -numa node "
  done
fi

$LAUNCHER qemu-system-x86_64 -m $VMEM -smp cpus=$VCPU $NUMA_CMD -cpu host -serial stdio -display none -enable-kvm `eval echo $KVM_NET_OPTS` $KVM_ARGS

