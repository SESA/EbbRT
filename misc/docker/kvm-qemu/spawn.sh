#/bin/sh
if [ -n "$DEBUG" ]; then set -x; fi
if [ -n "$DRYRUN" ]; then 
  echo "***DRY RUN***: Command will not be executed"
  LAUNCHER=echo
fi


# Spawn a native EbbRT 'backend' VM
#
# Usage:
# ./spawn.sh <image> 

# VM configuration 
VCPU=${VM_CPU:-2}
VMEM=${VM_MEM:-2G}
WAIT=${VM_WAIT:-false}
TAP=${TAP_IFACE:-tap0}
VQS=$VCPU
NETVEC=$(($((${VCPU}*2))+ 2))
if [ $VCPU -eq 1 ]
then
  VQS=2
fi

: ${KVM_NET_OPTS:="-netdev tap,script=no,downscript=no,\
ifname=\$TAP,id=net0,vhost=on,queues=\$VQS \
-device virtio-net-pci,netdev=net0,mac=\$MAC,mq=on,\
vectors=\$NETVEC"}

if [ -z "$KVM_ARGS" ]; then
  # No boot configuration passed in   
  BOOTIMG=${BOOT_IMG:-$1}
  # verify boot image exists 
  if [[ ! -f "$BOOTIMG" ]]; then echo "Boot image not found: $1"; exit 1; fi
  KVM_ARGS="-kernel $BOOTIMG"
fi

# Generate random MAC address if one isn't set already
if [ -z "$MAC" ]; then
  hexchars="0123456789ABCDEF"
  end=$( for i in {1..8} ; do echo -n ${hexchars:$(( $RANDOM % 16 )):1} ; done | sed -e 's/\(..\)/:\1/g' )
  MAC=`echo 06:FE$end`
fi

$LAUNCHER qemu-system-x86_64 -m $VMEM -smp cpus=$VCPU -cpu host -serial stdio -display none -enable-kvm `eval echo $KVM_NET_OPTS` $KVM_ARGS
