#!/bin/bash
if [ -n "$DEBUG" ]; then set -x; fi

if [ -n "$MANUAL_LAUNCH" ]; then 
	echo "Will wait for /tmp/launch before VM boots"
fi

# Configure network for VM 
# This script configured the container to boot a single EbbRT backend

# Ensure working directory is local to this script
cd "$(dirname "$0")"

export TAP_IFACE=tap0
export KVM_ARGS=$@
BRIDGE_IFACE=br0
IFACE=${IFACE_DEFAULT:-eth0}

atoi()
{
	#Returns the integer representation of an IP arg, passed in ascii dotted-decimal notation (x.x.x.x)
	IP=$1; IPNUM=0
	for (( i=0 ; i<4 ; ++i )); do
	((IPNUM+=${IP%%.*}*$((256**$((3-${i}))))))
	IP=${IP#*.}
	done
	echo $IPNUM
}

itoa()
{
	#returns the dotted-decimal ascii form of an IP arg passed in integer format
	echo -n $(($(($(($((${1}/256))/256))/256))%256)).
	echo -n $(($(($((${1}/256))/256))%256)).
	echo -n $(($((${1}/256))%256)).
	echo $((${1}%256))
}

cidr2mask() {
  local i mask=""
  local full_octets=$(($1/8))
  local partial_octet=$(($1%8))

  for ((i=0;i<4;i+=1)); do
    if [ $i -lt $full_octets ]; then
      mask+=255
    elif [ $i -eq $full_octets ]; then
      mask+=$((256 - 2**(8-$partial_octet)))
    else
      mask+=0
    fi
    test $i -lt 3 && mask+=.
  done

  echo $mask
}

setup_bridge_networking() {
    MAC=`ip addr show $IFACE | grep ether | sed -e 's/^[[:space:]]*//g' -e 's/[[:space:]]*\$//g' | cut -f2 -d ' '`
    IP=`ip addr show dev $IFACE | grep "inet $IP" | awk '{print $2}' | cut -f1 -d/`
    CIDR=`ip addr show dev $IFACE | grep "inet $IP" | awk '{print $2}' | cut -f2 -d/`
    NETMASK=`cidr2mask $CIDR`
    GATEWAY=`ip route get 8.8.8.8 | grep via | cut -f3 -d ' '`
    NAMESERVER=( `grep nameserver /etc/resolv.conf | grep -v "#" | cut -f2 -d ' '` )
    NAMESERVERS=`echo ${NAMESERVER[*]} | sed "s/ /,/"`

    # Generate random new MAC address
    hexchars="0123456789ABCDEF"
    end=$( for i in {1..8} ; do echo -n ${hexchars:$(( $RANDOM % 16 )):1} ; done | sed -e 's/\(..\)/:\1/g' )
    export VM_MAC=`echo 06:FE$end`

    let "NEWCIDR=$CIDR"

    i=`atoi $IP`
    let "j=(255-(($i&0xff)))" 
    let "i=$i&~((1<<8)-1)" # X.Y.Z.0
    let "k=($i+$j)"
    NEWIP=`itoa k`

    # bridge
    #ip link set dev $IFACE mtu 9000 
    ip link add name $BRIDGE_IFACE type bridge
    ip addr add $NEWIP/$CIDR dev $BRIDGE_IFACE
    #ip link set dev $BRIDGE_IFACE mtu 9000 
    ip link set dev $BRIDGE_IFACE up
    # tap
    ip tuntap add dev $TAP_IFACE mode tap multi_queue 
    ip link set $TAP_IFACE master $BRIDGE_IFACE 
    #ip link set dev $TAP_IFACE mtu 9000
    ip link set dev $TAP_IFACE up
    # iface
    ip link set dev $IFACE down
    ip addr flush $IFACE 
    ip link set $IFACE master $BRIDGE_IFACE 
    ip link set dev $IFACE up
    if [[ $? -ne 0 ]]; then
        echo "Failed to bring up network bridge"
        exit 4
    fi
    if [ -z "$NO_ETH_BRIDGE" ]; then
      ebtables -t nat -A POSTROUTING -o eth0 -j snat --to-source $MAC -s $VM_MAC --snat-arp
      ebtables -t nat -A PREROUTING -i eth0 -j dnat -d $MAC --to-destination $VM_MAC 
    fi
    if [ -z "$NO_DHCP" ]; then
cat > /etc/dnsmasq.conf << EOF
      user=root
      dhcp-range=$IP,$IP
      dhcp-host=$VM_MAC,$HOSTNAME,$IP,infinite
      dhcp-option=option:router,$GATEWAY
      dhcp-option=option:netmask,$NETMASK
      dhcp-option=option:dns-server,$NAMESERVERS
EOF
        dnsmasq 
    fi
}

# Spin until signaled to continue 
# This allows boot image to be migrated into the local filesystem
if [ -z "$NO_WAIT" ]; then
  echo "Waiting for creation of file /tmp/signal"
  until [ -a /tmp/signal ]; do sleep 1; done
fi

setup_bridge_networking

if [ -n "$MANUAL_LAUNCH" ]; then 
  echo "Waiting for creation of file /tmp/launch"
  until [ -a /tmp/launch ]; do sleep 1; done
fi

./launch.sh 
